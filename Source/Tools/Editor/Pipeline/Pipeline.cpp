//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <EASTL/sort.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/Resource/JSONFile.h>
#include "EditorEvents.h"
#include "Project.h"
#include "Converter.h"
#include "Packager.h"
#include "Pipeline.h"
#include "GlobResources.h"


namespace Urho3D
{

static const char* AssetDatabaseFile = "AssetDatabase.json";

enum PipelineWorkPriority
{
    PWP_ASSET_IMPORT = 90000,
    PWP_PACKAGE_FILES = 100000,
};

Pipeline::Pipeline(Context* context)
    : Serializable(context)
    , watcher_(context)
{
    if (!GetEngine()->IsHeadless())
        SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Pipeline, HandleEndFrame));

    SubscribeToEvent(E_EDITORPROJECTCLOSING, [this](StringHash, VariantMap&) {
        SaveCacheInfo();
    });
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
        if (!converter || !converter->LoadJSON(converterData))
            return false;

        converters_.push_back(converter);
    }

    // CacheInfo.json
    {
        cacheInfo_.clear();
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
        for (auto it = root.begin(); it != root.end(); it++)
        {
            if (!it->second.Contains("mtime") || !it->second.Contains("files"))
                continue;

            cacheInfo_[it->first] = CacheEntry();
            auto& entry = cacheInfo_[it->first];

            entry.mtime_ = it->second["mtime"].GetUInt();
            const auto& files = it->second["files"];
            for (unsigned i = 0, size = files.Size(); i < size; i++)
                entry.files_.insert(files[i].GetString());
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

void Pipeline::BuildCache(ConverterKinds converterKinds, const StringVector& files)
{
    auto* project = GetSubsystem<Project>();

    // Schedule asset conversion
    StringVector resourcePaths{files};

    // If we are not processing explicit list of files - process all resources.
    if (resourcePaths.empty())
    {
        // Clean stale cache items
        StringVector resourceFiles, cacheFiles;
        GetFileSystem()->ScanDir(resourceFiles, project->GetResourcePath(), "*", SCAN_FILES, true);
        GetFileSystem()->ScanDir(cacheFiles, project->GetCachePath(), "*", SCAN_FILES, true);

        // Remove cache items that no longer correspond to existing resource
        for (const ea::string& resourceName : cacheInfo_.keys())
        {
            // Source asset may be in resources dir or in cache dir.
            if (!resourceFiles.contains(resourceName) && !cacheFiles.contains(resourceName))
                ClearCache(resourceName);
        }

        resourcePaths = resourceFiles;
    }

    // Remove up to date entries from processing
    if (skipUpToDateAssets_)
    {
        for (auto it = resourcePaths.begin(); it != resourcePaths.end();)
        {
            if (IsCacheOutOfDate(*it))
                // Pass to converter
                ++it;
            else
                // Ignore
                it = resourcePaths.erase(it);
        }
    }

    if (resourcePaths.empty())
        return;

    cacheInfoOutOfDate_ = true;
    StartWorkItems(converterKinds, resourcePaths);
}

void Pipeline::WaitForCompletion()
{
    while (!GetWorkQueue()->IsCompleted(0))
    {
        Time::Sleep(100);
        {
            MutexLock lock(lock_);
            if (!reschedule_.empty())
            {
                StartWorkItems(reschedule_);
                reschedule_.clear();
            }
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

bool Pipeline::IsCacheOutOfDate(const ea::string& resourceName) const
{
    auto it = cacheInfo_.find(resourceName);
    if (it == cacheInfo_.end())
        return true;

    auto* project = GetSubsystem<Project>();
    unsigned mtime = GetFileSystem()->GetLastModifiedTime(project->GetResourcePath() + resourceName);
    if (mtime == 0)
        mtime = GetFileSystem()->GetLastModifiedTime(project->GetCachePath() + resourceName);

    for (const ea::string& cachedFile : it->second.files_)
    {
        if (!GetFileSystem()->Exists(project->GetCachePath() + cachedFile))
            return true;
    }

    return it->second.mtime_ < mtime;
}

void Pipeline::ClearCache(const ea::string& resourceName)
{
    auto it = cacheInfo_.find(resourceName);
    if (it == cacheInfo_.end())
        return;

    auto* project = GetSubsystem<Project>();
    const CacheEntry& cacheEntry = it->second;
    for (const ea::string& cacheFile : cacheEntry.files_)
    {
        const ea::string& fullPath = project->GetCachePath() + cacheFile;
        GetFileSystem()->Delete(fullPath);

        for (ea::string parentPath = GetParentPath(fullPath);; parentPath = GetParentPath(parentPath))
        {
            if (!GetFileSystem()->DirExists(parentPath))
                continue;
            StringVector result;
            GetFileSystem()->ScanDir(result, parentPath, "*", SCAN_FILES|SCAN_DIRS|SCAN_HIDDEN, false);
            result.erase_first(".");
            result.erase_first("..");
            if (result.empty())
                GetFileSystem()->RemoveDir(parentPath, false);
            else
                break;
        }
    }

    cacheInfo_.erase(it);
}

void Pipeline::AddCacheEntry(const ea::string& resourceName, const ea::string& cacheResourceName)
{
    auto* project = GetSubsystem<Project>();
    CacheEntry& entry = cacheInfo_[resourceName];
    entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetResourcePath() + resourceName);
    if (entry.mtime_ == 0)
        entry.mtime_ = GetFileSystem()->GetLastModifiedTime(project->GetCachePath() + resourceName);
    entry.files_.insert(cacheResourceName);
}

void Pipeline::AddCacheEntry(const ea::string& resourceName, const StringVector& cacheResourceNames)
{
    for (const ea::string& cacheResourceName : cacheResourceNames)
        AddCacheEntry(resourceName, cacheResourceName);
}

ResourcePathLock Pipeline::LockResourcePath(const ea::string& resourcePath)
{
    ResourcePathLock result(this, resourcePath);
    return std::move(result);
}

void Pipeline::SaveCacheInfo()
{
    JSONFile file(context_);
    JSONValue& root = file.GetRoot();
    root = JSONValue(JSON_OBJECT);

    StringVector keys = cacheInfo_.keys();
    ea::quick_sort(keys.begin(), keys.end());
    for (const ea::string& resourceName : keys)
    {
        const CacheEntry& entry = cacheInfo_[resourceName];
        JSONValue files(JSON_ARRAY);
        for (const ea::string& fileName : entry.files_)
            files.Push(fileName);

        JSONValue en(JSON_OBJECT);
        en["mtime"] = entry.mtime_;
        en["files"] = files;

        root[resourceName] = en;
    }

    file.SaveFile(GetSubsystem<Project>()->GetCachePath() + "CacheInfo.json");
}

void Pipeline::Reschedule(const ea::string& resourceName)
{
    MutexLock lock(lock_);
    reschedule_.emplace_back(resourceName);
}

void Pipeline::StartWorkItems(const StringVector& resourcePaths)
{
    StartWorkItems(executingConverterKinds_, resourcePaths);
}

void Pipeline::StartWorkItems(ConverterKinds converterKinds, const StringVector& resourcePaths)
{
    executingConverterKinds_ = converterKinds;
    for (SharedPtr<Converter>& converter : converters_)
    {
        if (converterKinds & converter->GetKind())
        {
            GetWorkQueue()->AddWorkItem([this, converter, resourcePaths]()
            {
                converter->Execute(resourcePaths);
            }, PWP_ASSET_IMPORT);
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
    reschedule_.clear();
}

void Pipeline::CreatePaksAsync()
{
    // TODO: we need proper task dependency!
    GetWorkQueue()->Complete(PWP_PACKAGE_FILES);
    GetWorkQueue()->AddWorkItem([this]() {
        struct PackagingEntry
        {
            ea::string pakName_;
            ea::vector<std::regex> patterns_;
        };
        ea::vector<PackagingEntry> rules;

        auto* project = GetSubsystem<Project>();
        const ea::string& projectPath = GetSubsystem<Project>()->GetProjectPath();
        auto logger = Log::GetLogger("packager");

        JSONFile file(context_);
        if (file.LoadFile(projectPath + "Packaging.json"))
        {
            JSONValue& root = file.GetRoot();
            const JSONArray& entries = root.GetArray();
            for (int i = 0; i < entries.size(); i++)
            {
                const JSONObject& entry = entries[i].GetObject();

                auto inputIt = entry.find("input");
                if (inputIt == entry.end())
                {
                    logger.Error("Packaging rule must contain 'input' with a list of glob expressions.");
                    continue;
                }

                auto outputIt = entry.find("output");
                if (outputIt == entry.end())
                {
                    logger.Error("Packaging rule must contain 'output' file name.");
                    continue;
                }

                const JSONArray& patterns = inputIt->second.GetArray();

                PackagingEntry newEntry{};
                newEntry.pakName_ = outputIt->second.GetString();

                if (newEntry.pakName_.empty())
                {
                    logger.Error("Packaging rule must contain a valid output file name.");
                    continue;
                }

                for (int j = 0; j < patterns.size(); j++)
                {
                    const ea::string& pattern = patterns[j].GetString();
                    if (pattern.empty())
                    {
                        logger.Error("Input glob pattern can not be empty.");
                        continue;
                    }
                    else
                        newEntry.patterns_.push_back(GlobToRegex(pattern));
                }

                if (patterns.empty())
                {
                    logger.Error("At least one input glob pattern must exist.");
                    continue;
                }

                rules.emplace_back(std::move(newEntry));
            }
        }
        else
        {
            if (GetFileSystem()->FileExists(projectPath + "Pipeline.json"))
                logger.Error("Parsing of Pipeline.json failed. Using default rules.");

            // Default rule, package everything into single pak.
            rules.emplace_back(PackagingEntry{"Resources.pak", {std::regex(".+")}});
        }

//        ea::shared_ptr<PackageFile> packageFile(new PackageFile(context_, "Data.pak"));

        StringVector resourceFiles, cacheFiles;
        GetFileSystem()->ScanDir(resourceFiles, project->GetResourcePath(), "*", SCAN_FILES, true);
        GetFileSystem()->ScanDir(cacheFiles, project->GetCachePath(), "*", SCAN_FILES, true);
        GetFileSystem()->CreateDir(projectPath + "/Output");

        for (const PackagingEntry& rule : rules)
        {
            lock_.Acquire();
            assert(lockedPaths_.empty());

            Packager pkg(context_);
            pkg.OpenPackage(projectPath + "Output/" + rule.pakName_);

            auto gatherFiles = [&](const ea::string& baseDir, const StringVector& resources)
            {
                for (const ea::string& name : resources)
                {
                    if (cacheInfo_.contains(name))
                        logger.Info("{} not included because it has byproducts.", name);
                    else
                    {
                        bool matched = false;
                        for (const std::regex& pattern : rule.patterns_)
                        {
                            if ((matched = std::regex_match(name.c_str(), pattern)))
                                break;
                        }

                        if (!matched)
                            logger.Info("{} not included in {}", name, rule.pakName_);
                        else
                            pkg.AddFile(baseDir, name);
                    }
                }
            };

            gatherFiles(project->GetResourcePath(), resourceFiles);
            gatherFiles(project->GetCachePath(), cacheFiles);

            lock_.Release();

            pkg.Write();
        }

        logger.Info("Files packaged and saved to {}Output", projectPath);

    }, PWP_PACKAGE_FILES);
}

ResourcePathLock::ResourcePathLock(Pipeline* pipeline, const ea::string& resourcePath)
    : pipeline_(pipeline)
    , resourcePath_(resourcePath)
{
    pipeline->lock_.Acquire();

    bool isLocked = true;
    while (isLocked)
    {
        isLocked = false;
        for (const ea::string& path : pipeline->lockedPaths_)
        {
            if (resourcePath.starts_with(path))
            {
                isLocked = true;
                pipeline->lock_.Release();
                Time::Sleep(100);
                pipeline->lock_.Acquire();
                break;
            }
        }
    }

    pipeline->lockedPaths_.push_back(resourcePath);
    pipeline->lock_.Release();
}

ResourcePathLock::~ResourcePathLock()
{
    if (pipeline_)
    {
        MutexLock lock(pipeline_->lock_);
        pipeline_->lockedPaths_.erase_first(resourcePath_);
    }
}

}
