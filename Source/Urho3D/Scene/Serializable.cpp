//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/Archive.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/Serializer.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLFile.h"
#include "../Resource/JSONFile.h"
#include "../Resource/JSONValue.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/SceneEvents.h"
#include "../Scene/NodePrefab.h"
#include "../Scene/Serializable.h"

#include <EASTL/fixed_vector.h>

#include "../DebugNew.h"

namespace Urho3D
{

static const unsigned MAX_STACK_ATTRIBUTE_COUNT = 128;

static unsigned RemapAttributeIndex(const ea::vector<AttributeInfo>* attributes, const AttributeInfo& netAttr, unsigned netAttrIndex)
{
    if (!attributes)
        return netAttrIndex; // Could not remap

    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const AttributeInfo& attr = attributes->at(i);
        // Compare accessor to avoid name string compare
        if (attr.accessor_ && attr.accessor_ == netAttr.accessor_.Get())
            return i;
    }

    return netAttrIndex; // Could not remap
}

Serializable::Serializable(Context* context) :
    Object(context),
    setInstanceDefault_(false),
    temporary_(false)
{
}

Serializable::~Serializable() = default;

void Serializable::OnSetAttribute(const AttributeInfo& attr, const Variant& src)
{
    // TODO: may be could use an observer pattern here
    if (setInstanceDefault_)
        SetInstanceDefault(attr.name_, src);

    attr.accessor_->Set(this, src);
}

void Serializable::OnGetAttribute(const AttributeInfo& attr, Variant& dest) const
{
    attr.accessor_->Get(this, dest);
}

ObjectReflection* Serializable::GetReflection() const
{
    return context_->GetReflection(GetType());
}

const ea::vector<AttributeInfo>* Serializable::GetAttributes() const
{
    return context_->GetAttributes(GetType());
}

bool Serializable::Load(Deserializer& source)
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
        return true;

    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const AttributeInfo& attr = attributes->at(i);
        if (!attr.ShouldLoad())
            continue;

        if (source.IsEof())
        {
            URHO3D_LOGERROR("Could not load " + GetTypeName() + ", stream not open or at end");
            return false;
        }

        Variant varValue = source.ReadVariant(attr.type_, context_);
        OnSetAttribute(attr, varValue);
    }

    return true;
}

bool Serializable::Save(Serializer& dest) const
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
        return true;

    Variant value;

    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const AttributeInfo& attr = attributes->at(i);
        if (!attr.ShouldSave())
            continue;

        OnGetAttribute(attr, value);

        if (!dest.WriteVariantData(value))
        {
            URHO3D_LOGERROR("Could not save " + GetTypeName() + ", writing to stream failed");
            return false;
        }
    }

    return true;
}

bool Serializable::LoadXML(const XMLElement& source)
{
    if (source.IsNull())
    {
        URHO3D_LOGERROR("Could not load " + GetTypeName() + ", null source element");
        return false;
    }

    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
        return true;

    XMLElement attrElem = source.GetChild("attribute");
    unsigned startIndex = 0;

    while (attrElem)
    {
        ea::string name = attrElem.GetAttribute("name");
        unsigned i = startIndex;
        unsigned attempts = attributes->size();

        while (attempts)
        {
            const AttributeInfo& attr = attributes->at(i);
            if (attr.ShouldLoad() && !attr.name_.compare(name))
            {
                Variant varValue;

                // If enums specified, do enum lookup and int assignment. Otherwise assign the variant directly
                if (!attr.enumNames_.empty() && attr.type_ == VAR_INT)
                {
                    ea::string value = attrElem.GetAttribute("value");
                    const unsigned enumValue = attr.ConvertEnumToUInt(value);
                    if (enumValue != M_MAX_UNSIGNED)
                        varValue = enumValue;
                    else
                        URHO3D_LOGWARNING("Unknown enum value " + value + " in attribute " + attr.name_);
                }
                else
                    varValue = attrElem.GetVariantValue(attr.type_, context_);

                if (!varValue.IsEmpty())
                    OnSetAttribute(attr, varValue);

                startIndex = (i + 1) % attributes->size();
                break;
            }
            else
            {
                i = (i + 1) % attributes->size();
                --attempts;
            }
        }

        if (!attempts)
            URHO3D_LOGWARNING("Unknown attribute " + name + " in XML data");

        attrElem = attrElem.GetNext("attribute");
    }

    return true;
}

bool Serializable::LoadJSON(const JSONValue& source)
{
    if (source.IsNull())
    {
        URHO3D_LOGERROR("Could not load " + GetTypeName() + ", null JSON source element");
        return false;
    }

    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
        return true;

    // Get attributes value
    JSONValue attributesValue = source.Get("attributes");
    if (attributesValue.IsNull())
        return true;
    // Warn if the attributes value isn't an object
    if (!attributesValue.IsObject())
    {
        URHO3D_LOGWARNING("'attributes' object is present in " + GetTypeName() + " but is not a JSON object; skipping load");
        return true;
    }

    const JSONObject& attributesObject = attributesValue.GetObject();

    for (const AttributeInfo& attr : *attributes)
    {
        if (attr.ShouldLoad())
        {
            const JSONValue& value = attributesValue[attr.name_];
            if (value.GetValueType() == JSON_NULL)
                continue;

            Variant varValue;
            // If enums specified, do enum lookup ad int assignment. Otherwise assign variant directly
            if (!attr.enumNames_.empty() && attr.type_ == VAR_INT)
            {
                const ea::string& valueStr = value.GetString();
                const unsigned enumValue = attr.ConvertEnumToUInt(valueStr);
                if (enumValue != M_MAX_UNSIGNED)
                    varValue = enumValue;
                else
                    URHO3D_LOGWARNING("Unknown enum value " + valueStr + " in attribute " + attr.name_);
            }
            else
                varValue = value.GetVariantValue(attr.type_, context_);

            if (!varValue.IsEmpty())
                OnSetAttribute(attr, varValue);
        }
    }

    // Report missing attributes.
    for (const auto& pair : attributesObject)
    {
        bool found = false;
        for (int i = 0; i < attributes->size() && !found; i++)
            found |= attributes->at(i).name_ == pair.first;
        if (!found)
            URHO3D_LOGWARNING("Unknown attribute {} in JSON data", pair.first);
    }

    return true;
}

bool Serializable::SaveXML(XMLElement& dest) const
{
    if (dest.IsNull())
    {
        URHO3D_LOGERROR("Could not save " + GetTypeName() + ", null destination element");
        return false;
    }

    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
        return true;

    Variant value;

    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const AttributeInfo& attr = attributes->at(i);
        if (!attr.ShouldSave())
            continue;

        OnGetAttribute(attr, value);
        Variant defaultValue(GetAttributeDefault(i));

        // In XML serialization default values can be skipped. This will make the file easier to read or edit manually
        if (value == defaultValue && !SaveDefaultAttributes(attr))
            continue;

        XMLElement attrElem = dest.CreateChild("attribute");
        attrElem.SetAttribute("name", attr.name_);
        // If enums specified, set as an enum string. Otherwise set directly as a Variant
        if (!attr.enumNames_.empty() && attr.type_ == VAR_INT)
        {
            int enumValue = value.GetInt();
            attrElem.SetAttribute("value", attr.enumNames_[enumValue]);
        }
        else
            attrElem.SetVariantValue(value);
    }

    return true;
}

bool Serializable::SaveJSON(JSONValue& dest) const
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
        return true;

    Variant value;
    JSONValue attributesValue;

    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const AttributeInfo& attr = attributes->at(i);
        if (!attr.ShouldSave())
            continue;

        OnGetAttribute(attr, value);
        Variant defaultValue(GetAttributeDefault(i));

        // In JSON serialization default values can be skipped. This will make the file easier to read or edit manually
        if (value == defaultValue && !SaveDefaultAttributes(attr))
            continue;

        JSONValue attrVal;
        // If enums specified, set as an enum string. Otherwise set directly as a Variant
        if (!attr.enumNames_.empty() && attr.type_ == VAR_INT)
        {
            int enumValue = value.GetInt();
            attrVal = attr.enumNames_[enumValue];
        }
        else
            attrVal.SetVariantValue(value, context_);

        attributesValue.Set(attr.name_, attrVal);
    }
    dest.Set("attributes", attributesValue);

    return true;
}

bool Serializable::Load(const ea::string& resourceName)
{
    AbstractFilePtr file(GetSubsystem<ResourceCache>()->GetFile(resourceName, false));
    if (file)
        return Load(*file);
    return false;
}

bool Serializable::LoadXML(const ea::string& resourceName)
{
    SharedPtr<XMLFile> file(GetSubsystem<ResourceCache>()->GetResource<XMLFile>(resourceName, false));
    if (file)
        return LoadXML(file->GetRoot());
    return false;
}

bool Serializable::LoadJSON(const ea::string& resourceName)
{
    SharedPtr<JSONFile> file(GetSubsystem<ResourceCache>()->GetResource<JSONFile>(resourceName, false));
    if (file)
        return LoadJSON(file->GetRoot());
    return false;
}

bool Serializable::LoadFile(const ea::string& resourceName)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    // Router may redirect to different file.
    ea::string realResourceName = resourceName;
    cache->RouteResourceName(realResourceName, RESOURCE_CHECKEXISTS);
    ea::string extension = GetExtension(realResourceName);

    if (extension == ".xml")
        return LoadXML(realResourceName);
    if (extension == ".json")
        return LoadJSON(realResourceName);
    return Load(realResourceName);
}

void Serializable::SerializeInBlock(Archive& archive)
{
    SerializeInBlock(archive, false);
}

void Serializable::SerializeInBlock(Archive& archive, bool serializeTemporary)
{
    const bool compactSave = !archive.IsHumanReadable();
    const PrefabArchiveFlags flags = PrefabArchiveFlag::IgnoreSerializableId | PrefabArchiveFlag::IgnoreSerializableType
        | (serializeTemporary ? PrefabArchiveFlag::SerializeTemporary : PrefabArchiveFlag::None);

    SerializablePrefab prefab;

    if (!archive.IsInput())
        prefab.Import(this);

    prefab.SerializeInBlock(archive, flags, compactSave);

    if (archive.IsInput())
        prefab.Export(this);

    ApplyAttributes();
}

bool Serializable::SetAttribute(unsigned index, const Variant& value)
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
    {
        URHO3D_LOGERROR(GetTypeName() + " has no attributes");
        return false;
    }
    if (index >= attributes->size())
    {
        URHO3D_LOGERROR("Attribute index out of bounds");
        return false;
    }

    const AttributeInfo& attr = attributes->at(index);

    // Check that the new value's type matches the attribute type
    if (value.GetType() == attr.type_)
    {
        OnSetAttribute(attr, value);
        return true;
    }
    else
    {
        URHO3D_LOGERROR("Could not set attribute " + attr.name_ + ": expected type " + Variant::GetTypeName(attr.type_) +
                 " but got " + value.GetTypeName());
        return false;
    }
}

bool Serializable::SetAttribute(const ea::string& name, const Variant& value)
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
    {
        URHO3D_LOGERROR(GetTypeName() + " has no attributes");
        return false;
    }

    for (auto i = attributes->begin(); i != attributes->end(); ++i)
    {
        if (!i->name_.compare(name))
        {
            // Check that the new value's type matches the attribute type
            if (value.GetType() == i->type_)
            {
                OnSetAttribute(*i, value);
                return true;
            }
            else
            {
                URHO3D_LOGERROR("Could not set attribute " + i->name_ + ": expected type " + Variant::GetTypeName(i->type_)
                         + " but got " + value.GetTypeName());
                return false;
            }
        }
    }

    URHO3D_LOGERROR("Could not find attribute " + name + " in " + GetTypeName());
    return false;
}

void Serializable::ResetToDefault()
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
        return;

    for (unsigned i = 0; i < attributes->size(); ++i)
    {
        const AttributeInfo& attr = attributes->at(i);
        if (attr.mode_ & (AM_NOEDIT | AM_NODEID | AM_COMPONENTID | AM_NODEIDVECTOR))
            continue;

        Variant defaultValue = GetInstanceDefault(attr.name_);
        if (defaultValue.IsEmpty())
            defaultValue = attr.defaultValue_;

        OnSetAttribute(attr, defaultValue);
    }
}

void Serializable::RemoveInstanceDefault()
{
    instanceDefaultValues_.reset();
}

void Serializable::SetTemporary(bool enable)
{
    if (enable != temporary_)
    {
        temporary_ = enable;

        using namespace TemporaryChanged;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_SERIALIZABLE] = this;

        SendEvent(E_TEMPORARYCHANGED, eventData);
    }
}

Variant Serializable::GetAttribute(unsigned index) const
{
    Variant ret;

    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
    {
        URHO3D_LOGERROR(GetTypeName() + " has no attributes");
        return ret;
    }
    if (index >= attributes->size())
    {
        URHO3D_LOGERROR("Attribute index out of bounds");
        return ret;
    }

    OnGetAttribute(attributes->at(index), ret);
    return ret;
}

Variant Serializable::GetAttribute(const ea::string& name) const
{
    Variant ret;

    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
    {
        URHO3D_LOGERROR(GetTypeName() + " has no attributes");
        return ret;
    }

    for (auto i = attributes->begin(); i != attributes->end(); ++i)
    {
        if (!i->name_.compare(name))
        {
            OnGetAttribute(*i, ret);
            return ret;
        }
    }

    URHO3D_LOGERROR("Could not find attribute " + name + " in " + GetTypeName());
    return ret;
}

Variant Serializable::GetAttributeDefault(unsigned index) const
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
    {
        URHO3D_LOGERROR(GetTypeName() + " has no attributes");
        return Variant::EMPTY;
    }
    if (index >= attributes->size())
    {
        URHO3D_LOGERROR("Attribute index out of bounds");
        return Variant::EMPTY;
    }

    AttributeInfo attr = attributes->at(index);
    Variant defaultValue = GetInstanceDefault(attr.name_);
    return defaultValue.IsEmpty() ? attr.defaultValue_ : defaultValue;
}

Variant Serializable::GetAttributeDefault(const ea::string& name) const
{
    Variant defaultValue = GetInstanceDefault(name);
    if (!defaultValue.IsEmpty())
        return defaultValue;

    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    if (!attributes)
    {
        URHO3D_LOGERROR(GetTypeName() + " has no attributes");
        return Variant::EMPTY;
    }

    for (auto i = attributes->begin(); i != attributes->end(); ++i)
    {
        if (!i->name_.compare(name))
            return i->defaultValue_;
    }

    URHO3D_LOGERROR("Could not find attribute " + name + " in " + GetTypeName());
    return Variant::EMPTY;
}

unsigned Serializable::GetNumAttributes() const
{
    const ea::vector<AttributeInfo>* attributes = GetAttributes();
    return attributes ? attributes->size() : 0;
}

void Serializable::SetInstanceDefault(const ea::string& name, const Variant& defaultValue)
{
    // Allocate the instance level default value
    if (!instanceDefaultValues_)
        instanceDefaultValues_ = ea::make_unique<VariantMap>();
    instanceDefaultValues_->operator [](name) = defaultValue;
}

Variant Serializable::GetInstanceDefault(const ea::string& name) const
{
    if (instanceDefaultValues_)
    {
        auto i = instanceDefaultValues_->find(name);
        if (i != instanceDefaultValues_->end())
            return i->second;
    }

    return Variant::EMPTY;
}

}
