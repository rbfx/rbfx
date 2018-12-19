//
// Copyright (c) 2018 Rokas Kupstys
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
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Toolbox/IO/ContentUtilities.h>
#include "Editor.h"
#include "AssetConverter.h"
#include "ImportAssimp.h"
#include "CookScene.h"


namespace Urho3D
{

AssetConverter::AssetConverter(Context* context)
    : Object(context)
{
    assetImporters_.Push(SharedPtr<ImportAssimp>(new ImportAssimp(context_)));
    assetImporters_.Push(SharedPtr<CookScene>(new CookScene(context_)));

    SubscribeToEvent(E_ENDFRAME, std::bind(&AssetConverter::DispatchChangedAssets, this));
    SubscribeToEvent(E_CONSOLECOMMAND, std::bind(&AssetConverter::OnConsoleCommand, this, _2));
}

AssetConverter::~AssetConverter()
{
    for (auto& watcher : watchers_)
        watcher->StopWatching();
}

void AssetConverter::AddAssetDirectory(const String& path)
{
    SharedPtr<FileWatcher> watcher(new FileWatcher(context_));
    watcher->StartWatching(path, true);
    watchers_.Push(watcher);
}

void AssetConverter::RemoveAssetDirectory(const String& path)
{
    String realPath = AddTrailingSlash(path);
    for (auto it = watchers_.Begin(); it != watchers_.End();)
    {
        if ((*it)->GetPath() == realPath)
        {
            (*it)->StopWatching();
            it = watchers_.Erase(it);
        }
        else
            ++it;
    }
}

void AssetConverter::SetCachePath(const String& cachePath)
{
    GetFileSystem()->CreateDirsRecursive(cachePath);
    cachePath_ = cachePath;
}

String AssetConverter::GetCachePath() const
{
    return cachePath_;
}

void AssetConverter::VerifyCacheAsync()
{
    for (const auto& watcher : watchers_)
    {
        Vector<String> files;
        GetFileSystem()->ScanDir(files, watcher->GetPath(), "*", SCAN_FILES, true);

        for (const auto& file : files)
            ConvertAssetAsync(file);
    }
}

void AssetConverter::ConvertAssetAsync(const String& resourceName)
{
    ContentType type = GetContentType(resourceName);
    GetWorkQueue()->AddWorkItem([this, resourceName, type]() { ConvertAsset(resourceName, type); });
}

bool AssetConverter::ConvertAsset(const String& resourceName, ContentType type)
{
    if (!IsCacheOutOfDate(resourceName))
        return true;

    // Ensure that no resources are left over from previous version
    GetFileSystem()->RemoveDir(cachePath_ + resourceName, true);
    String resourceFileName = GetCache()->GetResourceFileName(resourceName);
    bool convertedAny = false;

    for (auto& importer : assetImporters_)
    {
        if (importer->Accepts(resourceFileName, type))
        {
            if (importer->Convert(resourceFileName))
                convertedAny = true;
        }
    }

    auto convertedAssets = GetCacheAssets(resourceName);
    if (!convertedAssets.Empty())
    {
        unsigned mtime = GetFileSystem()->GetLastModifiedTime(resourceFileName);
        for (const auto& path : GetCacheAssets(resourceName))
        {
            GetFileSystem()->SetLastModifiedTime(path, mtime);
            URHO3D_LOGINFOF("Imported %s", path.CString());
        }
    }

    return convertedAny;
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
    String assetCacheDirectory = cachePath_ + resourceName;
    if (GetFileSystem()->DirExists(assetCacheDirectory))
        GetFileSystem()->ScanDir(files, assetCacheDirectory, "", SCAN_FILES, true);
    for (auto& fileName : files)
        fileName = AddTrailingSlash(assetCacheDirectory) + fileName;
    return files;
}

void AssetConverter::OnConsoleCommand(VariantMap& args)
{
    using namespace ConsoleCommand;
    if (args[P_COMMAND].GetString() == "cache.sync")
        VerifyCacheAsync();
}

}
