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

#include "../Resource/XMLFile.h"
#include "../Resource/JSONFile.h"
#include "../Resource/XMLArchive.h"
#include "Urho3D/Resource/JSONArchive.h"

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

void ConfigFile::Clear()
{
    values_.clear();
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

bool ConfigFile::LoadXML(const XMLFile* xmlFile)
{
    if (!xmlFile)
        return false;

    XMLInputArchive archive(xmlFile->GetContext(), xmlFile->GetRoot());
    auto block = archive.OpenUnorderedBlock(xmlFile->GetRoot().GetName().c_str());
    this->SerializeInBlock(archive);
    return true;
}

bool ConfigFile::SaveXML(XMLFile* xmlFile)
{
    if (!xmlFile)
        return false;

    if (!xmlFile->GetRoot())
    {
        xmlFile->CreateRoot("Values");
    }

    XMLOutputArchive archive(xmlFile->GetContext(), xmlFile->GetRoot());
    auto block = archive.OpenUnorderedBlock(xmlFile->GetRoot().GetName().c_str());
    this->SerializeInBlock(archive);
    return true;
}

bool ConfigFile::LoadJSON(const JSONFile* jsonFile)
{
    if (!jsonFile)
        return false;

    JSONInputArchive archive(jsonFile->GetContext(), jsonFile->GetRoot());
    auto block = archive.OpenUnorderedBlock("Values");
    this->SerializeInBlock(archive);
    return true;
}

bool ConfigFile::SaveJSON(JSONFile* jsonFile)
{
    if (!jsonFile)
        return false;

    JSONOutputArchive archive(jsonFile->GetContext(), jsonFile->GetRoot());
    auto block = archive.OpenUnorderedBlock("Values");
    this->SerializeInBlock(archive);
    return true;
}

void ConfigFile::SetDefaultValue(const ea::string key, const Variant& vault)
{
    default_[key] = vault;
}

bool ConfigFile::SetValue(const ea::string& name, const Variant& value)
{
    const auto iter = default_.find(name);
    if (iter == default_.end())
    {
        URHO3D_LOGERROR("Unkown config file value {}", name);
        return false;
    }

    if (value.GetType() != iter->second.GetType())
    {
        URHO3D_LOGERROR("Type of {} doesn't match default value type", name);
        return false;
    }

    values_[name] = value;
    return true;
}

const Variant& ConfigFile::GetValue(const ea::string& name) const
{
    auto iter = values_.find(name);
    if (iter != values_.end() && !iter->second.IsEmpty())
        return iter->second;
    iter = default_.find(name);
    return iter != default_.end() ? iter->second : Variant::EMPTY;
}

} // namespace Urho3D
