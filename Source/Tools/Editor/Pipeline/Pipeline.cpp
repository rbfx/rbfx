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
#include <Urho3D/Engine/ApplicationSettings.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/JSONArchive.h>
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
            BuildCache(nullptr, PipelineBuildFlag::SKIP_UP_TO_DATE);
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
    SubscribeToEvent(E_EDITORIMPORTERATTRIBUTEMODIFIED, [this](StringHash, VariantMap& args) { OnImporterModified(args); });
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

Asset* Pipeline::GetAsset(const ea::string& resourceName, bool autoCreate)
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
        asset->SetName(actualResourceName);
        asset->Load();
        assert(asset->GetName() == actualResourceName);
        assets_[actualResourceName] = asset;
        return asset.Get();
    }

    return nullptr;
}

Flavor* Pipeline::AddFlavor(const ea::string& name)
{
    auto it = ea::find(flavors_.begin(), flavors_.end(), name, [](const Flavor* flavor, const ea::string& name) {
        return flavor->GetName() == name;
    });
    if (it != flavors_.end())
        return {};

    SharedPtr<Flavor> flavor(new Flavor(context_));
    flavor->SetName(name);
    flavors_.push_back(flavor);

    using namespace EditorFlavorAdded;
    VariantMap& args = GetEventDataMap();
    args[P_FLAVOR] = flavor;
    SendEvent(E_EDITORFLAVORADDED);

    SortFlavors();
    return flavor;
}

bool Pipeline::RemoveFlavor(const ea::string& name)
{
    if (name == Flavor::DEFAULT)
        return false;

    auto it = ea::find(flavors_.begin(), flavors_.end(), name, [](const Flavor* flavor, const ea::string& name) {
        return flavor->GetName() == name;
    });
    if (it == flavors_.end())
        return false;

    Flavor* flavor = *it;

    using namespace EditorFlavorRemoved;
    VariantMap& args = GetEventDataMap();
    args[P_FLAVOR] = flavor;
    SendEvent(E_EDITORFLAVORREMOVED);

    flavors_.erase(it);
    return true;
}

bool Pipeline::RenameFlavor(const ea::string& oldName, const ea::string& newName)
{
    if (oldName == Flavor::DEFAULT || newName == Flavor::DEFAULT)
        return false;

    auto it = ea::find(flavors_.begin(), flavors_.end(), oldName, [](const Flavor* flavor, const ea::string& name) {
        return flavor->GetName() == name;
    });
    if (it == flavors_.end())
        return false;

    Flavor* flavor = *it;
    flavor->SetName(newName);
    SortFlavors();

    using namespace EditorFlavorRenamed;
    VariantMap& args = GetEventDataMap();
    args[P_FLAVOR] = flavor;
    args[P_OLDNAME] = oldName;
    args[P_NEWNAME] = newName;
    SendEvent(E_EDITORFLAVORRENAMED);

    return true;
}

SharedPtr<WorkItem> Pipeline::ScheduleImport(Asset* asset, Flavor* flavor, PipelineBuildFlags flags)
{
    assert(asset != nullptr);
    if (asset->importing_)
        return {};

    if (flavor == nullptr)
        flavor = GetDefaultFlavor();

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

bool Pipeline::ExecuteImport(Asset* asset, Flavor* flavor, PipelineBuildFlags flags)
{
    bool importedAnything = false;
    auto* project = GetSubsystem<Project>();

    ea::string outputPath = project->GetCachePath();
    if (!flavor->IsDefault())
        outputPath += AddTrailingSlash(flavor->GetName());

    for (AssetImporter* importer : asset->importers_[SharedPtr(flavor)])
    {
        // Skip optional importers (importing default flavor when editor is running most likely)
        if (!(flags & PipelineBuildFlag::EXECUTE_OPTIONAL) && importer->IsOptional())
            continue;

        if (!importer->Accepts(asset->GetResourcePath()))
            continue;

        if (importer->Execute(asset, outputPath))
        {
            logger_.Info("{} imported 'res://{}'.", importer->GetTypeName(), asset->GetName());

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

void Pipeline::BuildCache(Flavor* flavor, PipelineBuildFlags flags)
{
    auto* project = GetSubsystem<Project>();
    auto* fs = GetFileSystem();

    if (flavor == nullptr)
        flavor = GetDefaultFlavor();

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

void Pipeline::CreatePaksAsync(Flavor* flavor)
{
    pendingPackageFlavor_.push_back(SharedPtr(flavor));
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

        Flavor* flavor = pendingPackageFlavor_.front();
        ea::string packageFile = Format("{}/Resources-{}.pak", project->GetProjectPath(), flavor->GetName());
        packagerModalTitle_ = "Packaging " + GetFileNameAndExtension(packageFile);
        pendingPackageFlavor_.pop_front();

        packager_ = new Packager(context_);
        if (!packager_->OpenPackage(packageFile, flavor))
            return;

        StringVector results;
        fs->ScanDir(results, project->GetResourcePath(), "*.*", SCAN_FILES, true);

        for (const ea::string& resourceName : results)
        {
            if (resourceName.ends_with(".asset"))
                continue;

            bool hasCustomFlavorSettings = HasFlavorSettings(resourceName);
            // Accept default flavor with no custom settings or non-default flavor with custom settings.
            if (flavor->IsDefault() == hasCustomFlavorSettings)
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
                if (flavors.first->IsDefault())
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
        if (auto block = archive.OpenSequentialBlock("flavors"))
        {
            for (unsigned i = 0, num = archive.IsInput() ? block.GetSizeHint() : flavors_.size(); i < num; i++)
            {
                if (auto block = archive.OpenUnorderedBlock("flavor"))
                {
                    Flavor* flavor = nullptr;
                    ea::string flavorName;
                    if (!archive.IsInput())
                    {
                        flavor = flavors_[i];
                        flavorName = flavor->GetName();
                    }
                    if (!SerializeValue(archive, "name", flavorName))
                        return false;

                    if (archive.IsInput())
                        flavor = AddFlavor(flavorName);

                    ea::map<ea::string, Variant>& parameters = flavor->GetEngineParameters();
                    if (auto block = archive.OpenMapBlock("settings", parameters.size()))
                    {
                        if (archive.IsInput())
                        {
                            for (unsigned j = 0; j < block.GetSizeHint(); ++j)
                            {
                                ea::string key;
                                if (!archive.SerializeKey(key))
                                    return false;
                                if (!SerializeValue(archive, "value", parameters[key]))
                                    return false;
                            }
                        }
                        else
                        {
                            for (auto& pair : parameters)
                            {
                                if (!archive.SerializeKey(const_cast<ea::string&>(pair.first)))
                                    return false;
                                if (!SerializeValue(archive, "value", pair.second))
                                    return false;
                            }
                        }
                    }
                }
            }
        }
    }

    // Add default flavor if:
    // * This is a new proejct and no flavors were loaded from project file.
    // * User modified project file and renamed default flavor to something else.
    if (archive.IsInput() && (flavors_.empty() || GetDefaultFlavor()->GetName() != Flavor::DEFAULT))
        AddFlavor(Flavor::DEFAULT);

    return true;
}

void Pipeline::SortFlavors()
{
    if (flavors_.size() < 2)
        return;
    auto it = ea::find(flavors_.begin(), flavors_.end(), Flavor::DEFAULT, [](const Flavor* flavor, const ea::string& name) {
        return flavor->GetName() == name;
    });
    if (it != flavors_.end())
        ea::swap(*it, *flavors_.begin());                 // default first
    ea::quick_sort(flavors_.begin() + 1, flavors_.end()); // sort the rest
}

Flavor* Pipeline::GetFlavor(const ea::string& name) const
{
    auto it = ea::find(flavors_.begin(), flavors_.end(), name, [](const Flavor* flavor, const ea::string& name) {
        return flavor->GetName() == name;
    });
    if (it == flavors_.end())
        return nullptr;
    return *it;
}

void Pipeline::OnImporterModified(VariantMap& args)
{
    using namespace EditorImporterAttributeModified;
    auto* asset = static_cast<Asset*>(args[P_ASSET].GetPtr());
    auto* importer = static_cast<AssetImporter*>(args[P_IMPORTER].GetPtr());

    if (asset->IsMetaAsset())
        // Meta-assets (directories) are not imported.
        return;

    if (!importer->GetFlavor()->IsImportedByDefault())
        return;

    if (!importer->IsOutOfDate())
        return;

    ScheduleImport(asset, importer->GetFlavor());
}

bool Pipeline::CookSettings() const
{
    auto* project = GetSubsystem<Project>();
    ApplicationSettings settings(context_);
    settings.defaultScene_ = project->GetDefaultSceneName();

    for (Flavor* flavor : GetFlavors())
    {
        settings.engineParameters_.clear();
        if (!flavor->IsDefault())
        {
            // All flavors inherit default flavor settings
            auto* defaultFlavor = GetDefaultFlavor();
            settings.engineParameters_.insert(defaultFlavor->GetEngineParameters().begin(), defaultFlavor->GetEngineParameters().end());
        }
        // And then override these any of default settings with ones from the flavor itself
        for (const auto& pair : flavor->GetEngineParameters())
            settings.engineParameters_[pair.first] = pair.second;

#if URHO3D_PLUGINS
        settings.plugins_.clear();
        for (Plugin* plugin : project->GetPlugins()->GetPlugins())
        {
            if (!plugin->IsManagedManually())
                continue;

            if (plugin->IsPrivate())
                continue;

            settings.plugins_.push_back(plugin->GetName());
        }
#endif

        JSONFile file(context_);
        JSONOutputArchive archive(&file);
        if (!settings.Serialize(archive))
            return false;
        GetFileSystem()->CreateDirsRecursive(flavor->GetCachePath());
        file.SaveFile(flavor->GetCachePath() + "Settings.json");
    }
    return true;
}

}
