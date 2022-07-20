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

#include "../Engine/ConfigManager.h"
#include "../Engine/Engine.h"
#include "../IO/Log.h"
#include "../IO/FileSystem.h"
#include "../IO/ArchiveSerializationBasic.h"
#include "../Resource/JSONArchive.h"

namespace Urho3D
{

/// Construct.
ConfigFile::ConfigFile(Context* context)
    : Object(context)
{
}

/// Return whether the serialization is needed.
bool ConfigFile::IsSerializable()
{
    return true;
};
/// Return whether to show "reset to default" button.
bool ConfigFile::CanResetToDefault()
{
    return false;
}


/// Reset settings to default.
void ConfigFile::ResetToDefaults()
{
}
/// Load configuration file.
bool ConfigFile::Load()
{
    return GetSubsystem<ConfigManager>()->Load(this);
}

/// Save configuration file.
bool ConfigFile::Save() const
{
    return GetSubsystem<ConfigManager>()->Save(this);
}

ConfigManager::ConfigManager(Context* context)
    : Object(context)
{
}

ConfigManager::~ConfigManager()
{
}

void ConfigManager::SetConfigDir(const ea::string& dir)
{
    const ea::string trimmedPath = dir.trimmed();
    if (trimmedPath.length())
    {
        configurationDir_ = AddTrailingSlash(trimmedPath);

        const auto fileSystem = context_->GetSubsystem<FileSystem>();
        if (!fileSystem->DirExists(configurationDir_))
            fileSystem->CreateDirsRecursive(configurationDir_);
    }
}

/// Get configuration file.
ConfigFile* ConfigManager::Get(StringHash type)
{
    if (configurationDir_.empty())
    {
        URHO3D_LOGERROR("ConfigManager is not initialized yet");
        return nullptr;
    }

    const auto fileIt = files_.find(type);
    if (fileIt != files_.end())
    {
        return fileIt->second;
    }

    ObjectReflection* reflection = context_->GetReflection(type);
    if (!reflection)
    {
        URHO3D_LOGERROR("Can't find configuration type");
        return nullptr;
    }
    SharedPtr<ConfigFile> configFile;
    configFile.DynamicCast(reflection->CreateObject());
    if (!configFile)
    {
        URHO3D_LOGERROR("Can't create object of type {} or it is not ConfigFile", reflection->GetTypeName());
        return nullptr;
    }

    files_[type] = configFile;
    Load(configFile);
    return configFile;
}

/// Load configuration file.
bool ConfigManager::Load(ConfigFile* configFile)
{
    if (!configFile)
        return false;

    if (configurationDir_.empty())
    {
        URHO3D_LOGERROR("ConfigManager is not initialized yet");
        return nullptr;
    }

    const auto fileSystem = context_->GetSubsystem<FileSystem>();
    const ea::string fileName = configurationDir_ + configFile->GetTypeName() + ".json";

    if (fileSystem->Exists(fileName))
    {
        const auto jsonFile = MakeShared<JSONFile>(context_);

        if (!jsonFile->LoadFile(fileName))
        {
            URHO3D_LOGERROR("Can't load file {}", fileName);
            return false;
        }
        if (!jsonFile->LoadObject(configFile->GetTypeName().c_str(), *configFile))
        {
            URHO3D_LOGERROR("Can't deserialize file {}", fileName);
            return false;
        }
        return true;
    }
    return false;
}

/// Save configuration file.
bool ConfigManager::SaveAll()
{
    bool res = true;
    for (auto configIt: files_)
    {
        res &= Save(configIt.second);
    }
    return res;
}

/// Save configuration file.
bool ConfigManager::Save(const ConfigFile* configFile)
{
    if (!configFile)
        return false;

    if (configurationDir_.empty())
    {
        URHO3D_LOGERROR("ConfigManager is not initialized yet");
        return nullptr;
    }

    const auto fileSystem = context_->GetSubsystem<FileSystem>();
    if (!fileSystem->DirExists(configurationDir_))
    {
        fileSystem->CreateDirsRecursive(configurationDir_);
    }
    const ea::string fileName = configurationDir_ + configFile->GetTypeName() + ".json";
    const auto jsonFile = MakeShared<JSONFile>(context_);
    if (!jsonFile->SaveObject(configFile->GetTypeName().c_str(), *configFile))
    {
        URHO3D_LOGERROR("Can't serialize file {}", configFile->GetTypeName());
        return false;
    }
    if (!jsonFile->SaveFile(fileName))
    {
        URHO3D_LOGERROR("Can't save file {}", fileName);
        return false;
    }
    return true;
}

} // namespace Urho3D
