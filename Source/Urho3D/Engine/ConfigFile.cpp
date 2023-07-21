//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Engine/ConfigFile.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/VirtualFileSystem.h"
#include "../IO/Log.h"
#include "../Resource/JSONFile.h"

#include <EASTL/sort.h>

namespace Urho3D
{

ConfigVariableDefinition& ConfigVariableDefinition::SetDefault(const Variant& value)
{
    defaultValue_ = value;
    type_ = value.GetType();
    return *this;
}

ConfigVariableDefinition& ConfigVariableDefinition::SetOptional(VariantType type)
{
    type_ = type;
    return *this;
}

ConfigVariableDefinition& ConfigVariableDefinition::Overridable()
{
    overridable_ = true;
    return *this;
}

ConfigVariableDefinition& ConfigVariableDefinition::CommandLinePriority()
{
    commandLinePriority_ = true;
    return *this;
}

void ConfigFile::ConfigFlavor::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Flavor", flavor_.components_);
    SerializeOptionalValue(archive, "Variables", variables_);
}

ConfigFile::ConfigFile(Context* context)
    : Object(context)
{
}

ConfigVariableDefinition& ConfigFile::DefineVariable(const ea::string& name, const Variant& defaultValue)
{
    ConfigVariableDefinition& desc = definitions_[name];
    return desc.SetDefault(defaultValue);
}

void ConfigFile::DefineVariables(const StringVariantMap& defaults)
{
    for (const auto& [name, value] : defaults)
        DefineVariable(name, value);
}

void ConfigFile::UpdatePriorityVariables(const StringVariantMap& defaults)
{
    for (const auto& [name, value] : defaults)
    {
        const auto iter = definitions_.find(name);
        if (iter == definitions_.end())
            continue;

        ConfigVariableDefinition& desc = iter->second;
        if (desc.commandLinePriority_)
            desc.SetDefault(value);
    }
}

void ConfigFile::SetVariable(const ea::string& name, const Variant& value)
{
    if (!value.IsEmpty())
        variables_[name] = value;
    else
        variables_.erase(name);
}

bool ConfigFile::HasVariable(const ea::string& name) const
{
    return variables_.find(name) != variables_.end();
}

const ConfigVariableDefinition* ConfigFile::GetVariableDefinition(const ea::string& name) const
{
    auto iter = definitions_.find(name);
    return iter != definitions_.end() ? &iter->second : nullptr;
}

const Variant& ConfigFile::GetVariable(const ea::string& name) const
{
    const auto iter = variables_.find(name);
    if (iter != variables_.end() && !iter->second.IsEmpty())
        return iter->second;

    const auto definition = GetVariableDefinition(name);
    return definition ? definition->defaultValue_ : Variant::EMPTY;
}

void ConfigFile::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Default", variablesPerFlavor_);
}

bool ConfigFile::LoadDefaults(const ea::string& fileName, const ApplicationFlavor& flavor)
{
    auto vfs = GetSubsystem<VirtualFileSystem>();
    const auto file = vfs->OpenFile(fileName, FILE_READ);

    // If file does not exist, it is not an error.
    if (!file)
    {
        URHO3D_LOGINFO("ConfigFile '{}' is not found", fileName);
        return false;
    }

    JSONFile jsonFile(context_);
    if (!jsonFile.Load(*file))
    {
        URHO3D_LOGERROR("Failed to load JSON settings file '{}'", fileName);
        return false;
    }

    if (!jsonFile.LoadObject(*this))
    {
        URHO3D_LOGERROR("Failed to parse settings file '{}'", fileName);
        return false;
    }

    const StringVariantMap& flavorVariables = GetFlavorVariables(flavor);
    for (const auto& [name, value] : flavorVariables)
        SetVariable(name, value);

    URHO3D_LOGINFO("ConfigFile '{}' is loaded with flavor '{}'", fileName, flavor.ToString());
    return true;
}

bool ConfigFile::LoadOverrides(const ea::string& fileName)
{
    auto vfs = GetSubsystem<VirtualFileSystem>();
    const auto file = vfs->OpenFile(fileName, FILE_READ);

    // If file does not exist, it is not an error.
    if (!file)
    {
        URHO3D_LOGINFO("ConfigFile overrides '{}' are not found", fileName);
        return false;
    }

    JSONFile jsonFile(context_);
    if (!jsonFile.Load(*file))
    {
        URHO3D_LOGERROR("Failed to load JSON settings file '{}'", fileName);
        return false;
    }

    const StringVariantMap& overrides = jsonFile.GetRoot().GetStringVariantMap();
    for (const auto& [name, value] : overrides)
    {
        const auto defintion = GetVariableDefinition(name);
        if (!defintion)
        {
            URHO3D_LOGWARNING("Ignoring override for unknown variable '{}'", name);
            continue;
        }
        if (!defintion->overridable_)
        {
            URHO3D_LOGWARNING("Ignoring override for non-overridable variable '{}'", name);
            continue;
        }
        if (defintion->type_ != value.GetType())
        {
            URHO3D_LOGWARNING("Ignoring override for variable '{}' with invalid type {} ({} was expected)",
                name, Variant::GetTypeName(value.GetType()), Variant::GetTypeName(defintion->type_));
            continue;
        }

        SetVariable(name, value);
    }

    URHO3D_LOGINFO("ConfigFile overrides '{}' are loaded", fileName);
    return true;
}

bool ConfigFile::SaveOverrides(const ea::string& fileName, const ApplicationFlavor& flavor) const
{
    auto vfs = GetSubsystem<VirtualFileSystem>();
    const auto file = vfs->OpenFile(fileName, FILE_WRITE);
    if (!file)
    {
        URHO3D_LOGERROR("Failed to write JSON settings to file '{}'", fileName);
        return false;
    }

    const StringVariantMap overrides = GetChangedVariables(flavor);

    JSONFile jsonFile(context_);
    jsonFile.GetRoot().SetStringVariantMap(overrides);
    if (!jsonFile.Save(*file))
    {
        URHO3D_LOGERROR("Failed to save JSON settings to file '{}'", fileName);
        return false;
    }

    return true;
}

StringVariantMap ConfigFile::GetFlavorVariables(const ApplicationFlavor& flavor) const
{
    ea::vector<ea::pair<unsigned, const ConfigFlavor*>> filteredVariables;
    for (const ConfigFlavor& variablesPerFlavor : variablesPerFlavor_)
    {
        if (const auto penalty = flavor.Matches(variablesPerFlavor.flavor_))
            filteredVariables.emplace_back(*penalty, &variablesPerFlavor);
    }

    ea::stable_sort(filteredVariables.begin(), filteredVariables.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.first > rhs.first; });

    StringVariantMap result;
    for (const auto& [_, variablesPerFlavor] : filteredVariables)
    {
        for (const auto& [key, value] : variablesPerFlavor->variables_)
            result[key] = value;
    }
    return result;
}

StringVariantMap ConfigFile::GetChangedVariables(const ApplicationFlavor& flavor) const
{
    StringVariantMap defaultValues;
    for (const auto& [name, defintion] : definitions_)
        defaultValues[name] = defintion.defaultValue_;

    const StringVariantMap& flavorVariables = GetFlavorVariables(flavor);
    for (const auto& [name, value] : flavorVariables)
        defaultValues[name] = value;

    StringVariantMap overrides;
    for (const auto& [name, value] : variables_)
    {
        const auto iter = defaultValues.find(name);
        if (iter == defaultValues.end())
            continue;

        if (iter->second != value)
            overrides[name] = value;
    }
    return overrides;
}

}
