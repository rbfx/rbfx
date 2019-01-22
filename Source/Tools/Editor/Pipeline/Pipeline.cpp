//
// Copyright (c) 2017-2019 Rokas Kupstys.
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONFile.h>
#include "Project.h"
#include "Converter.h"
#include "Pipeline.h"

namespace Urho3D
{

static const char* AssetDatabaseFile = "AssetDatabase.json";

Pipeline::Pipeline(Context* context)
    : Serializable(context)
    , watcher_(context)
{
    if (!GetEngine()->IsHeadless())
        SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Pipeline, HandleEndFrame));
}

Pipeline::~Pipeline()
{
}

bool Pipeline::LoadJSON(const JSONValue& source)
{
    if (source.GetValueType() != JSON_ARRAY)
        return false;

    for (auto i = 0U; i < source.Size(); i++)
    {
        const JSONValue& converterData = source[i];
        StringHash type = Converter::GetSerializedType(converterData);
        if (type == StringHash::ZERO)
            return false;

        SharedPtr<Converter> converter = DynamicCast<Converter>(context_->CreateObject(type));
        if (converter.Null() || !converter->LoadJSON(converterData))
            return false;

        converters_.Push(converter);
    }

    // CacheInfo.json
    {
        cacheInfo_.Clear();
        /*
         * Expected cache format:
         * {
         *   "resource/name.fbx": {
         *     "mtime": 123456,                   // Modification time of source resource file
         *     "files": [                         // List of files that belong to this resource
         *       "resource/name.fbx/model.mdl",
         *       "resource/name.fbx/idle.ani",
         *     ]
         *   }
         * }
         */

        JSONFile file(context_);
        auto* project = GetSubsystem<Project>();
        if (!file.LoadFile(project->GetCachePath() + "CacheInfo.json"))
        {
            // Empty cache.
            GetFileSystem()->RemoveDir(project->GetCachePath(), true);
            GetFileSystem()->CreateDir(project->GetCachePath());
            return true;
        }

        const auto& root = file.GetRoot().GetObject();
        for (auto it = root.Begin(); it != root.End(); it++)
        {
            if (!it->second_.Contains("mtime") || !it->second_.Contains("files"))
                continue;

            cacheInfo_[it->first_] = {};
            auto& entry = cacheInfo_[it->first_];

            entry.mtime_ = it->second_["mtime"].GetUInt();
            const auto& files = it->second_["files"];
            for (unsigned i = 0, size = files.Size(); i < size; i++)
                entry.files_.Insert(files[i].GetString());
        }
    }

    return true;
}

void Pipeline::EnableWatcher()
{
    auto* project = GetSubsystem<Project>();
    GetFileSystem()->CreateDirsRecursive(project->GetCachePath());
    watcher_.StartWatching(project->GetResourcePath(), true);
}

void Pipeline::BuildCache(ConverterKinds converterKinds, const StringVector& files, bool complete)
{
    auto* project = GetSubsystem<Project>();

    // Schedule asset conversion
    StringVector resourcePaths{files};

    // If we are not processing explicit list of files - process all resources.
    if (resourcePaths.Empty())
    {
        // Clean stale cache items
        StringVector resourceFiles, cacheFiles;
        GetFileSystem()->ScanDir(resourceFiles, project->GetResourcePath(), "*", SCAN_FILES, true);
        GetFileSystem()->ScanDir(cacheFiles, project->GetCachePath(), "*", SCAN_FILES, true);

        // Remove cache items that no longer correspond to existing resource
        for (const String& resourceName : cacheInfo_.Keys())
        {
            // Source asset may be in resources dir or in cache dir.
            if (!resourceFiles.Contains(resourceName) && !cacheFiles.Contains(resourceName))
                ClearCache(resourceName);
        }

        resourcePaths = resourceFiles;
    }

    // Remove up to date entries from processing
    if (skipUpToDateAssets_)
    {
        for (auto it = resourcePaths.Begin(); it != resourcePaths.End();)
        {
            if (IsCacheOutOfDate(*it))
                // Pass to converter
                ++it;
            else
                // Ignore
                it = resourcePaths.Erase(it);
        }
    }

    if (resourcePaths.Empty())
        return;

    cacheInfoOutOfDate_ = true;
    StartWorkItems(converterKinds, resourcePaths);

    while (complete && !GetWorkQueue()->IsCompleted(0))
    {
        Time::Sleep(100);
        {
            MutexLock lock(lock_);
            if (!reschedule_.Empty())
            {
                StartWorkItems(converterKinds, reschedule_);
                reschedule_.Clear();
            }
        }
        if (GetEngine()->IsHeadless())
        {
            // Give a chance to logging system to flush it's logs.
            SendEvent(E_ENDFRAME);
        }
    }
}

void Pipeline::DispatchChangedAssets()
{
    FileChange entry;
    bool modified = false;
    while (watcher_.GetNextChange(entry))
    {
        if (entry.fileName_ == AssetDatabaseFile)
            continue;

        modified = true;
    }

    if (modified)
        BuildCache(CONVERTER_ALWAYS);
}

bool Pipeline::IsCacheOutOfDate(const String& resourceName) const
{
    auto it = cacheInfo_.Find(resourceName);
    if (it == cacheInfo_.End())
        return true;

    auto* project = GetSubsystem<Project>();
    unsigned mtime = GetFileSystem()->GetLastModifiedTime(project->GetResourcePath() + resourceName);
    if (mtime == 0)
        mtime = GetFileSystem()->GetLastModifiedTime(project->GetCachePath() + resourceName);

    for (const String& cachedFile : it->second_.files_)
    {
        if (!GetFileSystem()->Exists(project->GetCachePath() + cachedFile))
            return true;
    }

    return it->second_.mtime_ < mtime;
}

void Pipeline::ClearCache(const String& resourceName)
{
    auto it = cacheInfo_.Find(resourceName);
    if (it == cacheInfo_.End())
        return;

    auto* project = GetSubsystem<Project>();
    const CacheEntry& cacheEntry = it->second_;
    for (const String& cacheFile : cacheEntry.files_)
    {
        const String& fullPath = project->GetCachePath() + cacheFile;
        GetFileSystem()->Delete(fullPath);

        for (String parentPath = GetParentPath(fullPath);; parentPath = GetParentPath(parentPath))
        {
            if (!GetFileSystem()->DirExists(parentPath))
                continue;
            StringVector result;
            GetFileSystem()->ScanDir(result, parentPath, "*", SCAN_FILES|SCAN_DIRS|SCAN_HIDDEN, false);
            result.Remove(".");
            result.Remove("..");
            if (result.Empty())
                GetFileSystem()->RemoveDir(parentPath, false);
            else
                break;
        }
    }

    cacheInfo_.Erase(it);
}

void Pipeline::AddCacheEntry(const String& resourceName, const String& cacheResourceName)
{
    auto* project = GetSubsystem<Project>();
    CacheEntry& entry = cacheInfo_[resourceName];
    entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetResourcePath() + resourceName);
    if (entry.mtime_ == 0)
        entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetCachePath() + resourceName);
    entry.files_.Insert(cacheResourceName);
}

void Pipeline::AddCacheEntry(const String& resourceName, const StringVector& cacheResourceNames)
{
    for (const String& cacheResourceName : cacheResourceNames)
        AddCacheEntry(resourceName, cacheResourceName);
}

ResourcePathLock Pipeline::LockResourcePath(const String& resourcePath)
{
    ResourcePathLock result(this, resourcePath);
    return std::move(result);
}

void Pipeline::SaveCacheInfo()
{
    JSONFile file(context_);
    JSONValue& root = file.GetRoot();
    root = JSONValue(JSON_OBJECT);

    StringVector keys = cacheInfo_.Keys();
    Sort(keys.Begin(), keys.End());
    for (const String& resourceName : keys)
    {
        const CacheEntry& entry = cacheInfo_[resourceName];
        JSONValue files(JSON_ARRAY);
        for (const String& fileName : entry.files_)
            files.Push(fileName);

        JSONValue en(JSON_OBJECT);
        en["mtime"] = entry.mtime_;
        en["files"] = files;

        root[resourceName] = en;
    }

    file.SaveFile(GetCache()->GetResourceFileName("CacheInfo.json"));
}

void Pipeline::Reschedule(const String& resourceName)
{
    MutexLock lock(lock_);
    reschedule_.EmplaceBack(resourceName);
}

void Pipeline::StartWorkItems(ConverterKinds converterKinds, const StringVector& resourcePaths)
{
    for (SharedPtr<Converter> converter : converters_)
    {
        if (converterKinds & converter->GetKind())
        {
            GetWorkQueue()->AddWorkItem([this, converter, resourcePaths]()
            {
                converter->Execute(resourcePaths);
            });
        }
    }
}

void Pipeline::HandleEndFrame(StringHash, VariantMap&)
{
    if (cacheInfoOutOfDate_ && GetWorkQueue()->IsCompleted(0))
    {
        cacheInfoOutOfDate_ = false;
        SaveCacheInfo();
    }

    DispatchChangedAssets();

    MutexLock lock(lock_);
    StartWorkItems(CONVERTER_ONLINE, reschedule_);
    reschedule_.Clear();
}

ResourcePathLock::ResourcePathLock(Pipeline* pipeline, const String& resourcePath)
    : pipeline_(pipeline)
    , resourcePath_(resourcePath)
{
    pipeline->lock_.Acquire();

    bool isLocked = true;
    while (isLocked)
    {
        isLocked = false;
        for (const String& path : pipeline->lockedPaths_)
        {
            if (resourcePath.StartsWith(path))
            {
                isLocked = true;
                pipeline->lock_.Release();
                Time::Sleep(100);
                pipeline->lock_.Acquire();
                break;
            }
        }
    }

    pipeline->lockedPaths_.Push(resourcePath);
    pipeline->lock_.Release();
}

ResourcePathLock::~ResourcePathLock()
{
    if (pipeline_)
    {
        MutexLock lock(pipeline_->lock_);
        pipeline_->lockedPaths_.Remove(resourcePath_);
    }
}

}
