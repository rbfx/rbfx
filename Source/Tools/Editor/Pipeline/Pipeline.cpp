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
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/Resource/JSONFile.h>
#include "EditorEvents.h"
#include "Project.h"
#include "Pipeline/Pipeline.h"

namespace Urho3D
{

static const unsigned PIPELINE_TASK_PRIORITY = 1000;

Pipeline::Pipeline(Context* context)
    : Object(context)
    , watcher_(context)
{
    if (GetEngine()->IsHeadless())
        return;

    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Pipeline, OnEndFrame));
    SubscribeToEvent(E_EDITORPROJECTLOADING, [this](StringHash, VariantMap&) {
        EnableWatcher();
        BuildCache(DEFAULT_PIPELINE_FLAVOR, PipelineBuildFlag::SKIP_UP_TO_DATE);
    });
    SubscribeToEvent(E_RESOURCERENAMED, [this](StringHash, VariantMap& args) {
        using namespace ResourceRenamed;
        ea::string from = args[P_FROM].GetString();
        ea::string to = args[P_TO].GetString();

        if (from.ends_with("/"))
        {
            ea::unordered_map<ea::string, SharedPtr<Asset>> assets{};
            for (auto it = assets_.begin(); it != assets_.end();)
            {
                if (it->first.starts_with(from))
                {
                    assets[to + it->first.substr(from.length())] = it->second;
                    it = assets_.erase(it);
                }
                else
                    ++it;
            }
            for (const auto& pair : assets)
                assets_[pair.first] = pair.second;
        }
        else
        {
            auto it = assets_.find(from);
            if (it != assets_.end())
            {
                assets_[to] = it->second;
                assets_.erase(it);
            }
        }
    });
}

void Pipeline::RegisterObject(Context* context)
{
    context->RegisterFactory<Pipeline>();
}

void Pipeline::EnableWatcher()
{
    auto* project = GetSubsystem<Project>();
    GetFileSystem()->CreateDirsRecursive(project->GetCachePath());
    watcher_.StartWatching(project->GetResourcePath(), true);
}

void Pipeline::OnEndFrame(StringHash, VariantMap&)
{
    FileChange entry;
    while (watcher_.GetNextChange(entry))
    {
        if (entry.fileName_.ends_with(".asset"))
            continue;

        if (Asset* asset = GetAsset(entry.fileName_))
            ScheduleImport(asset);
    }

    if (!dirtyAssets_.empty())
    {
        MutexLock lock(mutex_);
        dirtyAssets_.back()->Save();
        dirtyAssets_.pop_back();
    }
}

void Pipeline::ClearCache(const ea::string& resourceName)
{
    for (const auto& asset : assets_)
        asset.second->ClearCache();
}

Asset* Pipeline::GetAsset(const eastl::string& resourceName, bool autoCreate)
{
    MutexLock lock(mutex_);

    if (resourceName.empty() || resourceName.ends_with(".asset"))
        return nullptr;

    auto* project = GetSubsystem<Project>();
    auto* fs = GetFileSystem();

    ea::string resourcePath = project->GetResourcePath() + resourceName;
    ea::string resourceDirName;
    if (fs->DirExists(resourcePath))
    {
        resourcePath = AddTrailingSlash(resourcePath);
        resourceDirName = AddTrailingSlash(resourceName);
    }
    const ea::string& actualResourceName = !resourceDirName.empty() ? resourceDirName : resourceName;

    auto it = assets_.find(actualResourceName);
    if (it != assets_.end())
        return it->second;

    if (!autoCreate)
        return nullptr;

    if (fs->Exists(resourcePath))
    {
        SharedPtr<Asset> asset(context_->CreateObject<Asset>());
        asset->Initialize(actualResourceName);
        assert(asset->GetName() == actualResourceName);
        assets_[actualResourceName] = asset;
        return asset.Get();
    }

    return nullptr;
}

void Pipeline::AddFlavor(const ea::string& name)
{
    if (flavors_.contains(name))
        return;

    flavors_.push_back(name);

    for (auto& asset : assets_)
        asset.second->AddFlavor(name);
}

void Pipeline::RemoveFlavor(const ea::string& name)
{
    if (name == DEFAULT_PIPELINE_FLAVOR)
        return;

    for (auto& asset : assets_)
        asset.second->RemoveFlavor(name);

    flavors_.erase_first(name);
}

void Pipeline::RenameFlavor(const ea::string& oldName, const ea::string& newName)
{
    if (oldName == DEFAULT_PIPELINE_FLAVOR || newName == DEFAULT_PIPELINE_FLAVOR)
        return;

    auto it = flavors_.find(oldName);
    if (it == flavors_.end())
        return;

    *it = newName;

    for (auto& asset : assets_)
        asset.second->RenameFlavor(oldName, newName);
}

SharedPtr<WorkItem> Pipeline::ScheduleImport(Asset* asset, const ea::string& flavor, PipelineBuildFlags flags)
{
    assert(asset != nullptr);
    if (asset->importing_)
        return {};

    if (flags & PipelineBuildFlag::SKIP_UP_TO_DATE && !asset->IsOutOfDate(flavor))
        return {};

    asset->AddRef();
    asset->importing_ = true;
    return GetWorkQueue()->AddWorkItem([this, asset, flavor, flags]()
    {
        if (ExecuteImport(asset, flavor, flags))
        {
            MutexLock lock(mutex_);
            dirtyAssets_.push_back(SharedPtr(asset));
        }
        asset->importing_ = false;
        asset->ReleaseRef();            // TODO: Chance to free asset on non-main thread.
    }, PIPELINE_TASK_PRIORITY);
}

bool Pipeline::ExecuteImport(Asset* asset, const ea::string& flavor, PipelineBuildFlags flags)
{
    bool importedAnything = false;
    auto* project = GetSubsystem<Project>();
    bool isDefaultFlavor = flavor == DEFAULT_PIPELINE_FLAVOR;

    ea::string outputPath = project->GetCachePath();
    if (!isDefaultFlavor)
        outputPath += AddTrailingSlash(flavor);

    for (AssetImporter* importer : asset->importers_[flavor])
    {
        // Skip optional importers (importing default flavor when editor is running most likely)
        if (!(flags & PipelineBuildFlag::EXECUTE_OPTIONAL) && importer->IsOptional())
            continue;

        if (!importer->Accepts(asset->GetResourcePath()))
            continue;

        Log::GetLogger("pipeline").Info("{} is importing '{}'.", importer->GetTypeName(), asset->GetName());

        if (importer->Execute(asset, outputPath))
        {
            importedAnything = true;
            for (const ea::string& byproduct : importer->GetByproducts())
            {
                if (Asset* byproductAsset = GetAsset(byproduct))
                    ExecuteImport(byproductAsset, flavor, flags);
            }
        }
    }

    return importedAnything;
}

void Pipeline::BuildCache(const ea::string& flavor, PipelineBuildFlags flags)
{
    auto* project = GetSubsystem<Project>();
    auto* fs = GetFileSystem();

    StringVector results;
    fs->ScanDir(results, project->GetResourcePath(), "*.*", SCAN_FILES, true);

    for (const ea::string& resourceName : results)
    {
        if (resourceName.ends_with(".asset"))
            continue;

        if (Asset* asset = GetAsset(resourceName))
            ScheduleImport(asset, flavor, flags);
    }
}

void Pipeline::WaitForCompletion() const
{
    GetWorkQueue()->Complete(PIPELINE_TASK_PRIORITY);
}

}
