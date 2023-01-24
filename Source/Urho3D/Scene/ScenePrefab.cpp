//
// Copyright (c) 2022-2022 the rbfx project.
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

#include <Urho3D/Precompiled.h>

#include <Urho3D/Core/ObjectReflection.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/ScenePrefab.h>

namespace Urho3D
{

namespace
{

const unsigned char IdentifierTypeOffset = 6;

} // namespace

AttributePrefab::AttributePrefab(AttributeId id)
    : id_(id)
{
}

AttributePrefab::AttributePrefab(const ea::string& name)
    : name_(name)
    , nameHash_(name)
{
}

AttributePrefab::AttributePrefab(const char* name)
    : name_(name)
    , nameHash_(name)
{
}

AttributePrefab::AttributePrefab(StringHash nameHash)
    : nameHash_(nameHash)
{
}

AttributePrefab::IdentifierType AttributePrefab::GetIdentifierType() const
{
    if (id_ != AttributeId::None)
        return IdentifierType::Id;
    else if (!name_.empty())
        return IdentifierType::Name;
    else
        return IdentifierType::NameHash;
}

void AttributePrefab::SerializeInBlock(Archive& archive, bool compactSave)
{
    if (archive.IsHumanReadable())
    {
        SerializeOptionalValue(archive, "id", reinterpret_cast<unsigned&>(id_));
        SerializeOptionalValue(archive, "name", name_);
        if (name_.empty())
            SerializeOptionalValue(archive, "nameHash", nameHash_);

        VariantType type = value_.GetType();
        SerializeOptionalValue(archive, "type", type);
        SerializeVariantAsType(archive, "value", value_, type);
    }
    else
    {
        unsigned char descriptor{};
        IdentifierType identifierType{};
        VariantType type = value_.GetType();

        if (!archive.IsInput())
        {
            identifierType = GetIdentifierType();

            if (compactSave && identifierType == IdentifierType::Name)
                identifierType = IdentifierType::NameHash;

            descriptor = static_cast<unsigned char>(type & MAX_VAR_MASK)
                | (static_cast<unsigned char>(identifierType) << IdentifierTypeOffset);
        }

        SerializeValue(archive, "descriptor", descriptor);

        if (archive.IsInput())
        {
            type = static_cast<VariantType>(descriptor & MAX_VAR_MASK);
            identifierType = static_cast<IdentifierType>(descriptor >> IdentifierTypeOffset);
        }

        switch (identifierType)
        {
        case IdentifierType::Id: archive.SerializeVLE("id", reinterpret_cast<unsigned&>(id_)); break;

        case IdentifierType::Name: archive.Serialize("name", name_); break;

        case IdentifierType::NameHash: archive.Serialize("nameHash", nameHash_.MutableValue()); break;

        default: URHO3D_ASSERT(false); break;
        }

        SerializeVariantAsType(archive, "value", value_, type);
    }

    if (archive.IsInput() && !name_.empty())
        nameHash_ = StringHash{name_};
}

bool AttributePrefab::operator==(const AttributePrefab& rhs) const
{
    return ea::tie(id_, name_, nameHash_, value_) == ea::tie(rhs.id_, rhs.name_, rhs.nameHash_, rhs.value_);
}

void SerializeValue(Archive& archive, const char* name, AttributePrefab& value, bool compactSave)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    value.SerializeInBlock(archive, compactSave);
}

void SerializablePrefab::SetType(const ea::string& typeName)
{
    typeName_ = typeName;
    typeNameHash_ = StringHash{typeName};
}

void SerializablePrefab::SetType(const char* typeName)
{
    typeName_ = typeName;
    typeNameHash_ = StringHash{typeName};
}

void SerializablePrefab::SetType(StringHash typeNameHash)
{
    typeName_ = EMPTY_STRING;
    typeNameHash_ = typeNameHash;
}

void SerializablePrefab::Import(const Serializable* serializable, PrefabSaveFlags flags)
{
    const ObjectReflection* reflection = serializable->GetReflection();
    if (!reflection)
    {
        URHO3D_LOGERROR("Serializable '{}' is not reflected and cannot be serialized", reflection->GetTypeName());
        return;
    }

    const auto& objectAttributes = reflection->GetAttributes();
    const unsigned numObjectAttributes = objectAttributes.size();

    typeName_ = reflection->GetTypeName();
    typeNameHash_ = reflection->GetTypeNameHash();
    temporary_ = serializable->IsTemporary();

    attributes_.clear();
    attributes_.reserve(numObjectAttributes);
    for (unsigned attributeIndex = 0; attributeIndex < numObjectAttributes; ++attributeIndex)
    {
        const AttributeInfo& attr = objectAttributes[attributeIndex];
        if (!attr.ShouldSave())
            continue;

        if (!(attr.mode_ & AM_PREFAB) && flags.Test(PrefabSaveFlag::Prefab))
            continue;

        Variant value;
        serializable->OnGetAttribute(attr, value);

        // Skip defaults if allowed
        if (!flags.Test(PrefabSaveFlag::SaveDefaultValues))
        {
            const Variant defaultValue = serializable->GetAttributeDefault(attributeIndex);
            if (value == defaultValue)
                continue;
        }

        AttributePrefab& attributePrefab = flags.Test(PrefabSaveFlag::CompactAttributeNames)
            ? attributes_.emplace_back(attr.nameHash_)
            : attributes_.emplace_back(attr.name_);

        if (flags.Test(PrefabSaveFlag::EnumsAsStrings) && !attr.enumNames_.empty())
            value = attr.ConvertEnumToString(value.GetUInt());

        attributePrefab.SetValue(ea::move(value));
    }
}

void SerializablePrefab::Export(Serializable* serializable, PrefabLoadFlags flags) const
{
    const ObjectReflection* reflection = serializable->GetReflection();
    if (!reflection)
    {
        URHO3D_LOGERROR("Serializable '{}' is not reflected and cannot be serialized", reflection->GetTypeName());
        return;
    }

    if (flags.Test(PrefabLoadFlag::CheckSerializableType) && reflection->GetTypeNameHash() != typeNameHash_)
    {
        URHO3D_LOGERROR("Serializable '{}' is not of type '{}'", reflection->GetTypeName(),
            !typeName_.empty() ? typeName_ : typeNameHash_.ToString());
        return;
    }

    if (!flags.Test(PrefabLoadFlag::KeepTemporaryState))
        serializable->SetTemporary(temporary_);

    const auto& objectAttributes = reflection->GetAttributes();
    const unsigned numObjectAttributes = objectAttributes.size();

    for (const AttributePrefab& attributePrefab : attributes_)
    {
        // Not supported
        if (attributePrefab.GetId() != AttributeId::None)
            continue;

        const unsigned attributeIndex = reflection->GetAttributeIndex(attributePrefab.GetNameHash());
        if (attributeIndex == M_MAX_UNSIGNED)
            continue;

        const AttributeInfo& attr = objectAttributes[attributeIndex];
        if (!attr.ShouldLoad())
            continue;

        const Variant& value = attributePrefab.GetValue();

        if (value.GetType() == VAR_STRING && !attr.enumNames_.empty())
        {
            const unsigned enumValue = attr.ConvertEnumToUInt(value.GetString());
            if (enumValue != M_MAX_UNSIGNED)
                serializable->OnSetAttribute(attr, enumValue);
            else
            {
                URHO3D_LOGWARNING("Attribute '{}' of Serializable '{}' has unknown enum value '{}'",
                    attr.name_, reflection->GetTypeName(), value.GetString());
            }
        }
        else
            serializable->OnSetAttribute(attr, value);
    }
}

void SerializablePrefab::SerializeInBlock(Archive& archive, PrefabArchiveFlags flags, bool compactSave)
{
    // Serialize ID
    if (flags.Test(PrefabArchiveFlag::IgnoreSerializableId))
    {
        if (archive.IsInput())
            id_ = SerializableId::None;
    }
    else
    {
        SerializeOptionalValue(archive, "_id", reinterpret_cast<unsigned&>(id_), {},
            [](Archive& archive, const char* name, unsigned& value) { archive.SerializeVLE(name, value); });
    }

    // Serialize type
    if (flags.Test(PrefabArchiveFlag::IgnoreSerializableType))
    {
        if (archive.IsInput())
        {
            typeName_ = {};
            typeNameHash_ = StringHash::Empty;
        }
    }
    else
    {
        if (archive.IsUnorderedAccessSupportedInCurrentBlock())
        {
            // If loading from archive with unordered blocks, always try to serialize both
            SerializeOptionalValue(archive, "_typeName", typeName_);
            if (typeName_.empty())
                SerializeOptionalValue(archive, "_typeHash", typeNameHash_);
        }
        else if (flags.Test(PrefabArchiveFlag::CompactTypeNames))
            SerializeOptionalValue(archive, "_typeHash", typeNameHash_);
        else
            SerializeOptionalValue(archive, "_typeName", typeName_);
    }

    if (archive.IsInput() && !typeName_.empty())
        typeNameHash_ = StringHash{typeName_};

    // Serialize temporary
    if (flags.Test(PrefabArchiveFlag::SerializeTemporary))
        SerializeOptionalValue(archive, "_temporary", temporary_, false);
    else if (archive.IsInput())
        temporary_ = false;

    // Serialize attributes
    SerializeOptionalValue(archive, "attributes", attributes_, {},
        [&](Archive& archive, const char* name, auto& value)
    {
        SerializeVectorAsObjects(archive, name, attributes_, "attribute",
            [=](Archive& archive, const char* name, AttributePrefab& value)
            { SerializeValue(archive, name, value, compactSave); });
    });
}

AttributeScopeHint SerializablePrefab::GetEffectiveScopeHint(Context* context) const
{
    if (typeNameHash_ == StringHash::Empty)
        return AttributeScopeHint::Attribute;

    const ObjectReflection* reflection = context->GetReflection(typeNameHash_);
    return reflection ? reflection->GetEffectiveScopeHint() : AttributeScopeHint::Attribute;
}

bool SerializablePrefab::operator==(const SerializablePrefab& rhs) const
{
    return ea::tie(id_, typeNameHash_, typeName_, attributes_)
        == ea::tie(rhs.id_, rhs.typeNameHash_, rhs.typeName_, rhs.attributes_);
}

void SerializeValue(
    Archive& archive, const char* name, SerializablePrefab& value, PrefabArchiveFlags flags, bool compactSave)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    value.SerializeInBlock(archive, flags, compactSave);
}

const ScenePrefab ScenePrefab::Empty;

void ScenePrefab::SerializeInBlock(Archive& archive, PrefabArchiveFlags flags, bool compactSave)
{
    node_.SerializeInBlock(archive, ToNodeFlags(flags));

    SerializeOptionalValue(archive, "components", components_, {},
        [&](Archive& archive, const char* name, auto& value)
    {
        SerializeVectorAsObjects(archive, name, value, "component",
            [=](Archive& archive, const char* name, SerializablePrefab& value)
            { SerializeValue(archive, name, value, ToComponentFlags(flags), compactSave); });
    });

    SerializeOptionalValue(archive, "nodes", children_, {},
        [&](Archive& archive, const char* name, auto& value)
    {
        SerializeVectorAsObjects(archive, name, value, "node",
            [=](Archive& archive, const char* name, ScenePrefab& value)
            { SerializeValue(archive, name, value, flags, compactSave); });
    });
}

AttributeScopeHint ScenePrefab::GetEffectiveScopeHint(Context* context) const
{
    AttributeScopeHint result = AttributeScopeHint::Attribute;
    for (const SerializablePrefab& component : components_)
        result = ea::max(result, component.GetEffectiveScopeHint(context));
    for (const ScenePrefab& child : children_)
        result = ea::max(result, child.GetEffectiveScopeHint(context));
    return result;
}

void ScenePrefab::Clear()
{
    node_ = {};
    components_.clear();
    children_.clear();
}

bool ScenePrefab::IsEmpty() const
{
    return node_.GetAttributes().empty() && components_.empty() && children_.empty();
}

bool ScenePrefab::operator==(const ScenePrefab& rhs) const
{
    return ea::tie(node_, components_, children_) == ea::tie(rhs.node_, rhs.components_, rhs.children_);
}

void SerializeValue(Archive& archive, const char* name, ScenePrefab& value, PrefabArchiveFlags flags, bool compactSave)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    value.SerializeInBlock(archive, flags, compactSave);
}

} // namespace Urho3D
