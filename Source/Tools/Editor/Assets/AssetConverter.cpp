//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include <regex>

#include "Editor.h"
#include "AssetConverter.h"

namespace Urho3D
{

AssetConverter::AssetConverter(Context* context)
    : Object(context)
{
    GetFileSystem()->CreateDirsRecursive(GetCachePath());
    SubscribeToEvent(E_UPDATE, std::bind(&AssetConverter::DispatchChangedAssets, this));
}

void AssetConverter::AddAssetDirectory(const String& path)
{
    SharedPtr<FileWatcher> watcher(new FileWatcher(context_));
    watcher->StartWatching(path, true);
    watchers_.Push(watcher);
}

String AssetConverter::GetCachePath()
{
    auto resourceCacheDir = ((SystemUI*)ui::GetIO().UserData)->GetFileSystem()->GetProgramDir();
    if (resourceCacheDir.EndsWith("/tools/"))
        resourceCacheDir = GetParentPath(resourceCacheDir);
    return resourceCacheDir + "Cache/";
}

void AssetConverter::VerifyCacheAsync()
{
    auto* rules_ = GetCache()->GetResource<XMLFile>("Settings/AssetConversionRules.xml");
    GetWorkQueue()->AddWorkItem([=]() {
        SharedPtr<XMLFile> rules(rules_);
        for (const auto& watcher : watchers_)
        {
            Vector<String> files;
            GetFileSystem()->ScanDir(files, watcher->GetPath(), "*", SCAN_FILES, true);

            for (const auto& file : files)
                ConvertAsset(file, rules);
        }
    });
}

void AssetConverter::ConvertAssetAsync(const String& resourceName)
{
    SharedPtr<XMLFile> rules(GetCache()->GetResource<XMLFile>("Settings/AssetConversionRules.xml"));
    GetWorkQueue()->AddWorkItem(std::bind(&AssetConverter::ConvertAsset, this, resourceName, rules));
}

bool AssetConverter::ConvertAsset(const String& resourceName, const SharedPtr<XMLFile>& rules)
{
    if (!IsCacheOutOfDate(resourceName))
        return true;

    // Ensure that no resources are left over from previous version
    GetFileSystem()->RemoveDir(GetCachePath() + resourceName, true);

    XMLElement converters = rules->GetRoot("converters");
    for (auto converter = converters.GetChild("converter"); converter.NotNull();
         converter = converter.GetNext("converter"))
    {
        String regex = converter.GetAttribute("regex");
        if (regex.Empty())
        {
            // Wildcard is converted to regex
            regex = converter.GetAttribute("wildcard").CString();

            // Escape regex characters except for *
            const char* special = "\\.^$|()[]{}+?";
            for (char c = *special; c != 0; c = *(++special))
                regex.Replace(String(c), ToString("\\%c", c));

            // Replace wildcard characters
            regex.Replace("**", ".+");
            regex.Replace("*", "[^/]+");
        }

        std::regex re(regex.CString());
        if (!std::regex_match(resourceName.CString(), re))
            continue;

        for (auto method = converter.GetChild(); method.NotNull(); method = method.GetNext())
        {
            if (method.GetName() == "popen")
            {
                if (ExecuteConverterPOpen(method, resourceName) != 0)
                    return false;
            }
        }
    }

    auto convertedAssets = GetCacheAssets(resourceName);
    if (!convertedAssets.Empty())
    {
        unsigned mtime = GetFileSystem()->GetLastModifiedTime(GetCache()->GetResourceFileName(resourceName));
        for (const auto& path : GetCacheAssets(resourceName))
        {
            GetFileSystem()->SetLastModifiedTime(path, mtime);
            URHO3D_LOGINFOF("Imported %s", path.CString());
        }
        return true;
    }

    return false;
}

int AssetConverter::ExecuteConverterPOpen(const XMLElement& popen, const String& resourceName)
{
    auto* fs = context_->GetSubsystem<FileSystem>();
    String executable = popen.GetAttribute("executable");

    if (!fs->FileExists(executable))
    {
        PrintLine(ToString("Executable \"%s\" not found.", executable.CString()).CString(), true);
        return -1;
    }

    Vector<String> args;
    for (XMLElement arg = popen.GetChild("arg"); arg.NotNull(); arg = arg.GetNext("arg"))
    {
        String argument = arg.GetValue();
        InsertVariables(resourceName, argument);
        args.Push(argument);

        if (ToBool(arg.GetAttribute("output")))
            GetFileSystem()->CreateDirsRecursive(GetPath(argument));
    }

    Process process(executable, args);
    return process.Run();
}

void AssetConverter::DispatchChangedAssets()
{
    if (checkTimer_.GetMSec(false) < 3000)
        return;
    checkTimer_.Reset();

    for (auto& watcher : watchers_)
    {
        String path;
        while (watcher->GetNextChange(path))
            ConvertAssetAsync(path);
    }
}

bool AssetConverter::IsCacheOutOfDate(const String& resourceName)
{
    unsigned mtime = GetFileSystem()->GetLastModifiedTime(GetCache()->GetResourceFileName(resourceName));

    auto files = GetCacheAssets(resourceName);
    for (const auto& path : files)
    {
        if (GetFileSystem()->GetLastModifiedTime(path) != mtime)
            return true;
    }

    return files.Empty();
}

Vector<String> AssetConverter::GetCacheAssets(const String& resourceName)
{
    Vector<String> files;
    String assetCacheDirectory = GetCachePath() + resourceName;
    if (GetFileSystem()->DirExists(assetCacheDirectory))
        GetFileSystem()->ScanDir(files, assetCacheDirectory, "", SCAN_FILES, true);
    for (auto& fileName : files)
        fileName = AddTrailingSlash(assetCacheDirectory) + fileName;
    return files;
}

void AssetConverter::InsertVariables(const String& resourceName, String& value)
{
    value.Replace("${resourceName}", resourceName);
    value.Replace("${resourcePath}", GetCache()->GetResourceFileName(resourceName));
    value.Replace("${resourceCacheDir}", GetCachePath());
    value.Replace("${resourceCachePath}", GetCachePath() + resourceName);
}

void AssetConverter::RemoveAssetDirectory(const String& path)
{
    for (auto it = watchers_.Begin(); it != watchers_.End();)
    {
        if ((*it)->GetPath() == path)
            it = watchers_.Erase(it);
        else
            ++it;
    }
}

}
