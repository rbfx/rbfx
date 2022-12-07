//
// Copyright (c) 2022-2022 the Urho3D project.
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

#include "../IO/ConfigFile.h"

#include "BinaryArchive.h"
#include "../IO/FileIdentifier.h"
#include "../IO/FileSystem.h"
#include "../IO/VirtualFileSystem.h"
#include "../Resource/XMLFile.h"
#include "../Resource/JSONFile.h"
#include "../Resource/XMLArchive.h"
#include "../Resource/JSONArchive.h"

namespace Urho3D
{
namespace
{
template <typename T>
void SerializeConfigValue(Archive& archive, const char* key, Variant& defaultValue, StringVariantMap& values)
{
    if (archive.IsInput())
    {
        if (archive.HasElementOrBlock(key))
        {
            T actualValue = defaultValue.Get<T>();
            SerializeValue(archive, key, actualValue);
            values[key] = actualValue;
        }
    }
    else
    {
        const auto valueIterator = values.find(key);
        if (valueIterator != values.end())
        {
            T actualValue = valueIterator->second.Get<T>();
            if (defaultValue.Get<T>() != actualValue)
            {
                SerializeValue(archive, key, actualValue);
            }
        }
    }
}
void SerializeConfigVariant(Archive& archive, const char* key, Variant& defaultValue, StringVariantMap& values)
{
    switch (defaultValue.GetType())
    {
    case VAR_BOOL: SerializeConfigValue<bool>(archive, key, defaultValue, values); break;
    case VAR_STRING: SerializeConfigValue<ea::string>(archive, key, defaultValue, values); break;
    case VAR_INT: SerializeConfigValue<int>(archive, key, defaultValue, values); break;
    case VAR_INT64: SerializeConfigValue<long long>(archive, key, defaultValue, values); break;
    case VAR_FLOAT: SerializeConfigValue<float>(archive, key, defaultValue, values); break;
    case VAR_DOUBLE: SerializeConfigValue<double>(archive, key, defaultValue, values); break;
    case VAR_VECTOR2: SerializeConfigValue<Vector2>(archive, key, defaultValue, values); break;
    case VAR_VECTOR3: SerializeConfigValue<Vector3>(archive, key, defaultValue, values); break;
    case VAR_VECTOR4: SerializeConfigValue<Vector4>(archive, key, defaultValue, values); break;
    default: URHO3D_LOGERROR("Config value serialization for key {} not implemented", key);
    }
}

}

ConfigFileBase::ConfigFileBase(Context* context)
    : BaseClassName(context)
{
}

ConfigFileBase::~ConfigFileBase() = default;

void ConfigFileBase::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
}

ConfigFile::ConfigFile(Context* context)
    : BaseClassName(context)
{
}

ConfigFile::~ConfigFile() = default;

void ConfigFile::Clear()
{
    values_.clear();
}

bool ConfigFileBase::MergeFile(const ea::string& fileName)
{
    const auto* vfs = context_->GetSubsystem<VirtualFileSystem>();
    const auto settingsFileId = FileIdentifier("", fileName);

    // Load config files from least to most prioritized.
    for (unsigned i = 0; i < vfs->NumMountPoints(); ++i)
    {
        const auto mountPoint = vfs->GetMountPoint(i);
        if (const auto file = mountPoint->OpenFile(settingsFileId, FILE_READ))
            LoadImpl(file);
    }
    if (const auto file = vfs->OpenFile(FileIdentifier("conf", fileName), FILE_READ))
    {
        LoadImpl(file);
    }

    return true;
}

bool ConfigFileBase::LoadFile(const ea::string& fileName)
{
    const auto* vfs = context_->GetSubsystem<VirtualFileSystem>();
    if (const auto file = vfs->OpenFile(FileIdentifier("conf", fileName), FILE_READ))
    {
        return LoadImpl(file);
    }
    return false;
}

/// Load from binary resource.
bool ConfigFileBase::Load(const ea::string& resourceName)
{
    const auto* vfs = context_->GetSubsystem<VirtualFileSystem>();
    if (const auto file = vfs->OpenFile(FileIdentifier("conf", resourceName), FILE_READ))
    {
        return Load(*file);
    }
    return false;
}

/// Load from XML resource.
bool ConfigFileBase::LoadXML(const ea::string& resourceName)
{
    const auto* vfs = context_->GetSubsystem<VirtualFileSystem>();
    if (const auto file = vfs->OpenFile(FileIdentifier("conf", resourceName), FILE_READ))
    {
        XMLFile xmlFile(context_);
        if (!xmlFile.Load(*file))
            return false;
        return LoadXML(xmlFile.GetRoot());
    }
    return false;
}

/// Load from JSON resource.
bool ConfigFileBase::LoadJSON(const ea::string& resourceName)
{
    const auto* vfs = context_->GetSubsystem<VirtualFileSystem>();
    if (const auto file = vfs->OpenFile(FileIdentifier("conf", resourceName), FILE_READ))
    {
        JSONFile jsonFile(context_);
        if (!jsonFile.Load(*file))
            return false;
        return LoadJSON(jsonFile.GetRoot());
    }
    return false;
}

/// Load from file. Return true if successful.
bool ConfigFileBase::LoadImpl(const AbstractFilePtr& source)
{
    if (!source)
        return false;

    ea::string extension = GetExtension(source->GetName());
    if (extension == ".xml")
    {
        XMLFile xmlFile(context_);
        xmlFile.Load(*source);
        return LoadXML(xmlFile.GetRoot());

    }
    if (extension == ".json")
    {
        JSONFile jsonFile(context_);
        jsonFile.Load(*source);
        return LoadJSON(&jsonFile.GetRoot());
    }
    return Load(*source);
}

bool ConfigFileBase::SaveFile(const ea::string& fileName)
{
    const auto* vfs = context_->GetSubsystem<VirtualFileSystem>();
    auto fileId = FileIdentifier("conf", fileName);
    for (unsigned i = vfs->NumMountPoints(); i-- > 0; )
    {
        const auto mountPoint = vfs->GetMountPoint(i);
        if (mountPoint->AcceptsScheme(fileId.scheme_))
        {
            if (const auto file = mountPoint->OpenFile(fileId, FILE_WRITE))
            {
                ea::string extension = GetExtension(fileName);
                if (extension == ".xml")
                {
                    XMLFile xmlFile(context_);
                    if (!SaveXML(xmlFile.CreateRoot("Settings")))
                    {
                        return false;
                    }
                    return xmlFile.Save(*file);
                }
                if (extension == ".json")
                {
                    JSONValue value;
                    if (!SaveJSON(value))
                    {
                        return false;
                    }
                    JSONFile jsonFile(context_);
                    jsonFile.GetRoot() = value;
                    return jsonFile.Save(*file);
                }
                return Save(*file);
            }
        }
    }
    return false;
}

bool ConfigFileBase::Load(Deserializer& source)
{
    BinaryInputArchive archive(context_, source);
    auto block = archive.OpenUnorderedBlock("Settings");
    SerializeInBlock(archive);
    return true;
}

bool ConfigFileBase::Save(Serializer& dest) const
{
    BinaryOutputArchive archive(context_, dest);
    auto block = archive.OpenUnorderedBlock("Settings");
    const_cast<ConfigFileBase*>(this)->SerializeInBlock(archive);
    return true;
}

bool ConfigFileBase::LoadXML(const XMLElement& xmlFile)
{
    XMLInputArchive archive(context_, xmlFile);
    auto block = archive.OpenUnorderedBlock(xmlFile.GetName().c_str());
    this->SerializeInBlock(archive);
    return true;
}

bool ConfigFileBase::SaveXML(XMLElement& xmlFile) const
{
    if (xmlFile.IsNull())
    {
        URHO3D_LOGERROR("Could not save " + GetTypeName() + ", null destination element");
        return false;
    }

    XMLOutputArchive archive(context_, xmlFile);
    auto block = archive.OpenUnorderedBlock(xmlFile.GetName().c_str());
    const_cast<ConfigFileBase*>(this)->SerializeInBlock(archive);
    return true;
}

bool ConfigFileBase::LoadJSON(const JSONValue& source)
{
    if (source.IsNull())
        return false;

    JSONInputArchive archive(context_, source);
    auto block = archive.OpenUnorderedBlock("Settings");
    this->SerializeInBlock(archive);
    return true;
}

bool ConfigFileBase::SaveJSON(JSONValue& dest) const
{
    JSONOutputArchive archive(context_, dest);
    auto block = archive.OpenUnorderedBlock("Settings");
    const_cast<ConfigFileBase*>(this)->SerializeInBlock(archive);
    return true;
}

bool ConfigFile::SaveDiffFile(const ea::string& fileName)
{
    const auto* vfs = context_->GetSubsystem<VirtualFileSystem>();
    auto settingsFileId = FileIdentifier("", fileName);

    ConfigFile mergedConfig(context_);

    // Copy default values.
    for (auto& keyValue : default_)
    {
        mergedConfig.SetDefaultValue(keyValue.first, keyValue.second);
    }

    // Load config files from least to most prioritized.
    for (unsigned i = 0; i < vfs->NumMountPoints(); ++i)
    {
         const auto mountPoint = vfs->GetMountPoint(i);
         if (mountPoint->Exists(settingsFileId))
         {
             mergedConfig.LoadImpl(mountPoint->OpenFile(settingsFileId, FILE_READ));
         }
     }

    ConfigFile diffConfig(context_);
    // Set values from parent files as default values.
    for (auto& keyValue : mergedConfig.default_)
    {
         diffConfig.SetDefaultValue(keyValue.first, mergedConfig.GetValue(keyValue.first));
    }

    for (auto& keyValue : values_)
    {
         diffConfig.SetValue(keyValue.first, keyValue.second);
    }

    return diffConfig.SaveFile(fileName);
}

void ConfigFile::SerializeInBlock(Archive& archive)
{
    for (auto& keyValue : default_)
    {
        auto& key = keyValue.first;
        auto& defaultValue = keyValue.second;

        SerializeConfigVariant(archive, key.c_str(), defaultValue, values_);
    }
}

void ConfigFile::SetDefaultValue(const ea::string key, const Variant& vault)
{
    default_[key] = vault;
}

bool ConfigFile::SetValue(const ea::string& name, const Variant& value)
{
    const auto iterator = default_.find(name);
    if (iterator == default_.end())
    {
        URHO3D_LOGERROR("Unkown config file value {}", name);
        return false;
    }

    if (value.GetType() != iterator->second.GetType())
    {
        URHO3D_LOGERROR("Type of {} doesn't match default value type", name);
        return false;
    }

    values_[name] = value;
    return true;
}

const Variant& ConfigFile::GetValue(const ea::string& name) const
{
    auto iterator = values_.find(name);
    if (iterator != values_.end() && !iterator->second.IsEmpty())
        return iterator->second;
    iterator = default_.find(name);
    return iterator != default_.end() ? iterator->second : Variant::EMPTY;
}

} // namespace Urho3D
