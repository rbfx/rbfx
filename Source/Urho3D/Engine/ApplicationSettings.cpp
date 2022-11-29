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

#include "../Engine/ApplicationSettings.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/FileSystemFile.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/PackageFile.h"
#include "../Resource/JSONFile.h"

#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

const ea::string defaultFileName = "Settings.json";

ea::string GetProgramFolder(Context* context)
{
    auto fs = context->GetSubsystem<FileSystem>();
    return AddTrailingSlash(fs->GetProgramDir());
}

ea::string FindSettingsFileInFileSystem(Context* context)
{
    auto fs = context->GetSubsystem<FileSystem>();
    const ea::string programFolder = GetProgramFolder(context);

    for (const ea::string scanFolder : {programFolder, fs->GetCurrentDir()})
    {
        for (const ea::string& prefix : {"", "Data/", "CoreData/"})
        {
            const ea::string filePath = Format("{}{}{}", scanFolder, prefix, defaultFileName);
            if (fs->FileExists(filePath))
                return filePath;
        }
    }

    return EMPTY_STRING;
}

ea::string FindPackageWithSettings(Context* context)
{
    auto fs = context->GetSubsystem<FileSystem>();
    const ea::string programFolder = GetProgramFolder(context);

    StringVector packageFiles;
    for (const ea::string scanFolder : {programFolder, fs->GetCurrentDir()})
        fs->ScanDirAdd(packageFiles, scanFolder, "*.pak", SCAN_FILES, false);

    for (const ea::string& packageFile : packageFiles)
    {
        auto package = MakeShared<PackageFile>(context);
        if (!package->Open(APK + packageFile))
        {
            URHO3D_LOGERROR("Failed to open {}", packageFile);
            continue;
        }

        if (package->Exists(defaultFileName))
            return APK + packageFile;
    }

    return EMPTY_STRING;
}

SharedPtr<JSONFile> LoadSettingsFile(Context* context)
{
    if (const ea::string fileName = FindSettingsFileInFileSystem(context); !fileName.empty())
    {
        auto jsonFile = MakeShared<JSONFile>(context);
        if (jsonFile->LoadFile(fileName))
            return jsonFile;
    }

    if (const ea::string packageFile = FindPackageWithSettings(context); !packageFile.empty())
    {
        auto package = MakeShared<PackageFile>(context);
        if (package->Open(APK + packageFile))
        {
            if (const auto file = package->OpenFile(defaultFileName))
            {
                auto jsonFile = MakeShared<JSONFile>(context);
                if (jsonFile->Load(*file))
                    return jsonFile;
            }
        }
    }

    return nullptr;
}

}

void ApplicationSettings::FlavoredSettings::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Flavor", flavor_.components_);
    SerializeOptionalValue(archive, "EngineParameters", engineParameters_);
}

ApplicationSettings::ApplicationSettings(Context* context)
    : Object(context)
{
}

void ApplicationSettings::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Settings", settings_);
}

StringVariantMap ApplicationSettings::GetParameters(const ApplicationFlavor& flavor) const
{
    ea::vector<ea::pair<unsigned, const FlavoredSettings*>> filteredSettings;
    for (const FlavoredSettings& flavoredSettings : settings_)
    {
        if (const auto penalty = flavor.Matches(flavoredSettings.flavor_))
            filteredSettings.emplace_back(*penalty, &flavoredSettings);
    }

    ea::stable_sort(filteredSettings.begin(), filteredSettings.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.first > rhs.first; });

    StringVariantMap result;
    for (const auto& [_, flavoredSettings] : filteredSettings)
    {
        for (const auto& [key, value] : flavoredSettings->engineParameters_)
            result[key] = value;
    }
    return result;
}

StringVariantMap ApplicationSettings::GetParametersForCurrentFlavor() const
{
    // TODO(editor): Handle platforms
    return GetParameters(ApplicationFlavor::Empty);
}

SharedPtr<ApplicationSettings> ApplicationSettings::LoadForCurrentApplication(Context* context)
{
    auto result = MakeShared<ApplicationSettings>(context);
    if (auto jsonFile = LoadSettingsFile(context))
        jsonFile->LoadObject(*result);
    return result;
}

}
