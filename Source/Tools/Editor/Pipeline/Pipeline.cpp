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
#include "Packager.h"
#include "Pipeline.h"

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
        BuildCache();
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
    if (resourceName.empty())
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

void Pipeline::ScheduleImport(Asset* asset, const ea::string& flavor, PipelineBuildFlags flags,
    const ea::string& inputFile)
{
    assert(asset != nullptr);
    if (asset->importing_)
        return;

    const ea::string& importFile = !inputFile.empty() ? inputFile : asset->GetResourcePath();

    asset->AddRef();
    asset->importing_ = true;
    GetWorkQueue()->AddWorkItem([this, asset, flavor, importFile, flags]()
    {
        if (ExecuteImport(asset, flavor, flags, importFile))
        {
            MutexLock lock(mutex_);
            dirtyAssets_.push_back(SharedPtr<Asset>(asset));
        }
        asset->importing_ = false;
        asset->ReleaseRef();
    }, PIPELINE_TASK_PRIORITY);
}

bool Pipeline::ExecuteImport(Asset* asset, const ea::string& flavor, PipelineBuildFlags flags,
    const ea::string& inputFile)
{
    bool importedAnything = false;
    auto* project = GetSubsystem<Project>();
    bool isDefaultFlavor = flavor == DEFAULT_PIPELINE_FLAVOR;

    ea::string outputPath = project->GetCachePath();
    if (!isDefaultFlavor)
        outputPath += AddTrailingSlash(flavor);

    if (asset->GetName().contains("/"))
        outputPath += AddTrailingSlash(GetPath(asset->GetName()));

    if (inputFile.starts_with(outputPath))
    {
        // inputFile points to a byproduct of this asset. Append a subdirectory of input file to the output.
        outputPath += GetPath(inputFile).substr(outputPath.length());
    }

    for (AssetImporter* importer : asset->importers_[flavor])
    {
        // Skip optional importers (importing default flavor when editor is running most likely)
        if (!(flags & PipelineBuildFlag::EXECUTE_OPTIONAL) && importer->IsOptional())
            continue;

        if (!importer->Accepts(inputFile))
            // Default flavor does not execute
            continue;

        GetLog()->GetLogger("pipeline").Info("{} is importing '{}'.", importer->GetTypeName(), GetFileNameAndExtension(inputFile));

        if (importer->Execute(asset, inputFile, outputPath))
        {
            importedAnything = true;
            for (const ea::string& byproduct : importer->GetByproducts())
                ExecuteImport(asset, flavor, Urho3D::PipelineBuildFlags(), byproduct);
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
        if (Asset* asset = GetAsset(resourceName))
        {
            if (flags & PipelineBuildFlag::SKIP_UP_TO_DATE && !asset->IsOutOfDate())
                continue;
            ScheduleImport(asset, flavor, flags);
        }
    }
}

void Pipeline::WaitForCompletion() const
{
    GetWorkQueue()->Complete(PIPELINE_TASK_PRIORITY);
}

}
