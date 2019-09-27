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
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/Resource/JSONFile.h>
#include "EditorEvents.h"
#include "Project.h"
#include "Pipeline/Pipeline.h"

namespace Urho3D
{

Pipeline::Pipeline(Context* context)
    : Object(context)
    , watcher_(context)
{
    if (GetEngine()->IsHeadless())
        return;

    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(Pipeline, OnEndFrame));
    SubscribeToEvent(E_EDITORPROJECTSERIALIZE, [this](StringHash, VariantMap& args) {
        auto* archive = static_cast<Archive*>(args[EditorProjectSerialize::P_ARCHIVE].GetVoidPtr());
        if (archive->IsInput())
        {
            EnableWatcher();
            BuildCache(DEFAULT_PIPELINE_FLAVOR, PipelineBuildFlag::SKIP_UP_TO_DATE);
        }
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
    SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap&) { OnUpdate(); });
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
    }, 0);                              // Lowest possible priority.
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
    GetWorkQueue()->Complete(0);
}

void Pipeline::CreatePaksAsync(const ea::string& flavor)
{
    pendingPackageFlavor_.push_back(flavor);
    /*
     * Packaging 010
     *
     * Each project has one or more flavors. "default" flavor is present in all projects and is special. Package of "default" flavor
     * contains common assets required by all builds of the project.
     *
     * If asset is not imported by the engine, but used in it's raw form - it will always be included in Resources-default.pak
     *
     * If asset is imported by the engine and has no custom importer settings or if custom settings are set to "default" flavor - byproducts
     * of import procedure will always be included in Resources-default.pak while source asset will not be included in any pak.
     *
     * If asset is imported by the engine and has custom importer settings for flavors other than "default" - each flavor will execute asset
     * import and include byproducts in Resources-flavor.pak. If flavor has no custom settings - settings of default flavor will be used.
     * Source asset will not be included in any paks. No byproducts will be included in Resources-default.pak.
     *
     * Final product must always include Resources-default.pak and any additional paks. For example mobile games may ship
     * Resources-default.pak + Resources-android.pak for android builds as well as Resources-default.pak + Resources-ios.pak for iOS builds.
     * A game meant for desktop computers may ship Resources-default.pak + Resources-low.pak + Resources-medium.pak + Resources-high.pak
     * where low/medium/high flavors have asset import configurations for different asset quality. User would load resources from one of
     * these paks depending on game quality settings.
     *
     */
}

void Pipeline::OnUpdate()
{
    if (packager_.NotNull())
    {
        ui::OpenPopup("Packaging Files");
        if (ui::BeginPopupModal(packagerModalTitle_.c_str(), nullptr,
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_Popup))
        {
            ui::ProgressBar(packager_->GetProgress());
            ui::SetCursorPosX(ui::GetCursorPosX() + 200);
            ui::EndPopup();
        }

        if (packager_->IsCompleted())
        {
            packager_ = nullptr;
        }
    }
    else if (!pendingPackageFlavor_.empty())
    {
        auto* fs = GetFileSystem();
        auto* project = GetSubsystem<Project>();

        ea::string flavor = pendingPackageFlavor_.front();
        ea::string packageFile = Format("{}/Resources-{}.pak", project->GetProjectPath(), flavor);
        packagerModalTitle_ = "Packaging " + GetFileNameAndExtension(packageFile);
        pendingPackageFlavor_.pop_front();

        packager_ = new Packager(context_);
        if (!packager_->OpenPackage(packageFile, flavor))
            return;

        StringVector results;
        fs->ScanDir(results, project->GetResourcePath(), "*.*", SCAN_FILES, true);

        bool isDefaultFlavor = flavor == DEFAULT_PIPELINE_FLAVOR;
        for (const ea::string& resourceName : results)
        {
            if (resourceName.ends_with(".asset"))
                continue;

            bool hasCustomFlavorSettings = HasFlavorSettings(resourceName);
            // Accept default flavor with no custom settings or non-default flavor with custom settings.
            if (isDefaultFlavor == hasCustomFlavorSettings)
                continue;

            if (Asset* asset = GetAsset(resourceName))
            {
                ScheduleImport(asset, flavor, PipelineBuildFlag::EXECUTE_OPTIONAL | PipelineBuildFlag::SKIP_UP_TO_DATE);
                packager_->AddAsset(asset);
            }
        }
        packager_->Start();
    }
}

bool Pipeline::HasFlavorSettings(const ea::string& resourceName)
{
    StringVector parts = resourceName.split('/');
    bool isDir = resourceName.ends_with("/");

    while (!parts.empty())
    {
        ea::string checkName;
        for (const ea::string& part : parts)
        {
            checkName += part;
            checkName += "/";
        }
        if (!isDir)
        {
            checkName = checkName.substr(0, checkName.length() - 1);    // Strip /
            isDir = true;                                               // next iteration will be dir
        }
        parts.pop_back();

        if (Asset* asset = GetAsset(checkName))
        {
            for (const auto& flavors : asset->GetImporters())
            {
                if (flavors.first == DEFAULT_PIPELINE_FLAVOR)
                    continue;

                for (const auto& importer : flavors.second)
                {
                    if (importer->IsModified())
                        return true;
                }
            }
        }
    }

    return false;
}

bool Pipeline::Serialize(Archive& archive)
{
    if (auto block = archive.OpenUnorderedBlock("pipeline"))
    {
        if (!SerializeValue(archive, "flavors", flavors_))
            return false;

        if (archive.IsInput())
        {
            for (const ea::string& flavor : flavors_)
            {
                for (auto& asset : assets_)
                    asset.second->AddFlavor(flavor);
            }
        }
    }
    return true;
}

}
