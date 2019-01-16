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
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) {
        DispatchChangedAssets();
    });
}

Pipeline::~Pipeline()
{
    GetWorkQueue()->Complete(0);
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
//            GetFileSystem()->RemoveDir(project->GetCachePath(), true);
//            GetFileSystem()->CreateDir(project->GetCachePath());
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
                entry.files_.Push(files[i].GetString());
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

void Pipeline::BuildCache(ConverterKinds converterKinds)
{
    GetWorkQueue()->AddWorkItem([this, converterKinds]()
    {
        auto* project = GetSubsystem<Project>();
        for (auto& converter : converters_)
        {
            if ((converterKinds& converter->GetKind()) == converter->GetKind())
            {
                StringVector resourcePaths, cachePaths;
                GetFileSystem()->ScanDir(resourcePaths, project->GetResourcePath(), "*", SCAN_FILES, true);
                GetFileSystem()->ScanDir(cachePaths, project->GetCachePath(), "*", SCAN_FILES, true);
                resourcePaths.Push(cachePaths);

                for (auto it = resourcePaths.Begin(); it != resourcePaths.End();)
                {
                    if (IsCacheOutOfDate(*it))
                        // Pass to converter
                        ++it;
                    else
                        // Ignore
                        it = resourcePaths.Erase(it);
                }

                if (!resourcePaths.Empty())
                    converter->Execute(resourcePaths);
            }
        }
        SaveCacheInfo();
    });
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
        VacuumCache();
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
    }

    cacheInfo_.Erase(it);
}

void Pipeline::VacuumCache()
{
    auto* project = GetSubsystem<Project>();

    StringVector resourceFiles, cacheFiles;
    GetFileSystem()->ScanDir(resourceFiles, project->GetResourcePath(), "*", SCAN_FILES, true);
    GetFileSystem()->ScanDir(cacheFiles, project->GetCachePath(), "*", SCAN_FILES, true);

    // Remove cache items that no longer correspond to existing resource
    for (const String& resourceName : cacheInfo_.Keys())
    {
        if (!resourceFiles.Contains(resourceName) && !cacheFiles.Contains(resourceName))
            ClearCache(resourceName);
    }

    // Rebuild cache
    BuildCache(CONVERTER_ONLINE);
}

void Pipeline::AddCacheEntry(const String& resourceName, const String& cacheResourceName)
{
    auto* project = GetSubsystem<Project>();
    CacheEntry& entry = cacheInfo_[resourceName];
    entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetResourcePath() + resourceName);
    if (entry.mtime_ == 0)
        entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetCachePath() + resourceName);
    entry.files_.Push(cacheResourceName);
}

void Pipeline::AddCacheEntry(const String& resourceName, const StringVector& cacheResourceNames)
{
    auto* project = GetSubsystem<Project>();
    CacheEntry& entry = cacheInfo_[resourceName];
    entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetResourcePath() + resourceName);
    if (entry.mtime_ == 0)
        entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetCachePath() + resourceName);
    entry.files_.Push(cacheResourceNames);
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

    auto* project = GetSubsystem<Project>();
    file.SaveFile(project->GetCachePath() + "CacheInfo.json");
}

ResourcePathLock::ResourcePathLock(Pipeline* pipeline, const String& resourcePath)
    : pipeline_(pipeline)
    , resourcePath_(resourcePath)
{
    pipeline->lock_.Acquire();
    while (pipeline->lockedPaths_.Contains(resourcePath))
    {
        pipeline->lock_.Release();
        Time::Sleep(100);
        pipeline->lock_.Acquire();
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
