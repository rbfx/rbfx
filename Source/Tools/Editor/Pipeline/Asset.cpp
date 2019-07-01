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
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include <Toolbox/SystemUI/Widgets.h>
#include <Toolbox/SystemUI/ResourceBrowser.h>

#include "Project.h"
#include "Pipeline.h"
#include "Asset.h"
#include "Importers/ModelImporter.h"


namespace Urho3D
{

Asset::Asset(Context* context)
    : Serializable(context)
{
    SubscribeToEvent(E_RESOURCERENAMED, [this](StringHash, VariantMap& args) {
        using namespace ResourceRenamed;
        const ea::string& from = args[P_FROM].GetString();
        const ea::string& to = args[P_TO].GetString();

        if (extraInspectors_.contains(from))
        {
            extraInspectors_[to] = extraInspectors_[from];
            extraInspectors_.erase(from);
        }

        if (name_ != from)
            return;

        auto* fs = GetFileSystem();

        ea::string assetPathFrom = GetCache()->GetResourceFileName(name_) + ".asset";
        ea::string assetPathTo = GetCache()->GetResourceFileName(to) + ".asset";

        if (!fs->Rename(assetPathFrom, assetPathTo))
        {
            URHO3D_LOGERROR("Failed to rename '{}' to '{}'", assetPathFrom, assetPathTo);
            return;
        }

        name_ = to;
    });

    SubscribeToEvent(E_RESOURCEBROWSERSELECT, [this](StringHash, VariantMap& args) {
        if (IsMetaAsset())
            return;

        using namespace ResourceBrowserSelect;
        const ea::string& resourceName = args[P_NAME].GetString();
        if (resourceName.starts_with(name_))
        {
            // Selected resource is a child of current asset.
            UpdateExtraInspectors(resourceName);
        }
    });

    SubscribeToEvent(E_RESOURCEBROWSERDELETE, [this](StringHash, VariantMap& args) {
        using namespace ResourceBrowserDelete;
        extraInspectors_.erase(args[P_NAME].GetString());
    });
}

void Asset::RegisterObject(Context* context)
{
    context->RegisterFactory<Asset>();
}

void Asset::Initialize(const ea::string& resourceName)
{
    assert(name_.empty());

    auto* project = GetSubsystem<Project>();

    resourcePath_ = project->GetResourcePath() + resourceName;
    name_ = resourceName;

    Load();

    // Default-initialize
    for (const ea::string& flavor : project->GetPipeline().GetFlavors())
        AddFlavor(flavor);

    contentType_ = ::Urho3D::GetContentType(context_, resourceName);
}

bool Asset::IsOutOfDate() const
{
    if (importers_.empty())
        // Asset is used as is and never converted.
        return false;

    auto* project = GetSubsystem<Project>();
    auto* fs = GetSubsystem<FileSystem>();
    unsigned mtime = fs->GetLastModifiedTime(resourcePath_);

    bool hasByproducts = false;
    bool hasAcceptingImporters = false;
    for (const auto& flavor : importers_)
    {
        for (const auto& importer : flavor.second)
        {
            hasAcceptingImporters |= importer->Accepts(resourcePath_);
            hasByproducts |= !importer->GetByproducts().empty();
            for (const ea::string& byproduct : importer->GetByproducts())
            {
                const ea::string& byproductPath = project->GetCachePath() + byproduct;
                if (fs->FileExists(byproductPath))
                {
                    if (fs->GetLastModifiedTime(byproductPath) < mtime)
                        // Out of date when resource is newer than any of byproducts.
                        return true;
                }
            }
        }
    }

    if (!hasAcceptingImporters)
        // Nothing to import.
        return false;

    // Out of date when nothing was imported.
    return !hasByproducts;
}

void Asset::ClearCache()
{
    auto* project = GetSubsystem<Project>();
    auto* fs = GetSubsystem<FileSystem>();

    for (const auto& flavor : importers_)
    {
        for (const auto& importer : flavor.second)
            importer->ClearByproducts();
    }

    // Delete cache directory where all byproducts of this asset go
    ea::string cachePath = project->GetCachePath() + GetName();
    if (fs->DirExists(cachePath))
        fs->RemoveDir(cachePath, true);
}

void Asset::RenderInspector(const char* filter)
{
    if (currentExtraInspectorProvider_)
    {
        auto* project = GetSubsystem<Project>();
        auto* fs = GetSubsystem<FileSystem>();

        ea::string resourceFileName = GetCache()->GetResourceFileName(currentExtraInspectorProvider_->GetResourceName());
        if (resourceFileName.empty() || !fs->FileExists(resourceFileName))
        {
            extraInspectors_.erase(currentExtraInspectorProvider_->GetResourceName());
            currentExtraInspectorProvider_ = nullptr;
        }
        else
            currentExtraInspectorProvider_->RenderInspector(filter);
    }

    bool tabBarStarted = false;
    bool save = false;
    StringVector flavors = importers_.keys();
    auto it = flavors.find(DEFAULT_PIPELINE_FLAVOR);
    ea::swap(*it, *flavors.begin());                    // default first
    ea::quick_sort(flavors.begin() + 1, flavors.end()); // sort the rest

    for (const ea::string& flavor : flavors)
    {
        bool tabStarted = false;
        bool tabVisible = false;
        for (const auto& importer : importers_[flavor])
        {
            bool importerSupportsFiles = false;
            if (IsMetaAsset())
            {
                // This is a meta-asset pointing to a directory. Such assets are used to hold importer settings
                // for downstream importers to inherit from. Show all importers for directories.
                importerSupportsFiles = true;
            }
            else
            {
                // This is a real asset. Show only those importers that can import asset itself or any of it's
                // byproducts.
                importerSupportsFiles = importer->Accepts(resourcePath_);
                for (const auto& siblingImporter : importers_[flavor])
                {
                    if (importer == siblingImporter)
                        continue;

                    for (const auto& byproduct : siblingImporter->GetByproducts())
                    {
                        importerSupportsFiles |= importer->Accepts(byproduct);
                        if (importerSupportsFiles)
                            break;
                    }
                    if (importerSupportsFiles)
                        break;
                }
            }

            if (importerSupportsFiles)
            {
                // Defer rendering of tab bar and tabs until we know that we have compatible importers. As a result if
                // file is not supported by any importer - tab bar with flavors and no content will not be shown.
                if (!tabBarStarted)
                {
                    ui::TextUnformatted("Importers");
                    ui::Separator();
                    ui::BeginTabBar(Format("###{}", (void*) this).c_str(), ImGuiTabBarFlags_None);
                    tabBarStarted = true;
                }

                if (!tabStarted)
                {
                    tabStarted = true;
                    tabVisible = ui::BeginTabItem(flavor.c_str());
                    if (tabVisible)
                        ui::SetHelpTooltip("Pipeline flavor");
                }

                if (tabVisible)
                {
                    importer->RenderInspector(filter);
                    save |= importer->attributesModified_;
                    importer->attributesModified_ = false;
                }
            }
        }

        if (tabVisible)
            ui::EndTabItem();
    }

    if (tabBarStarted)
        ui::EndTabBar();

    if (save)
        Save();
}

bool Asset::Save()
{
    ea::string assetPath = RemoveTrailingSlash(resourcePath_) + ".asset";

    bool isModified = false;

    // Check if asset importers have any attributes modified
    for (const auto& flavor : importers_)
    {
        for (const auto& importer : flavor.second)
        {
            isModified |= importer->IsModified();
            if (isModified)
                break;
        }
    }

    // Check if any of asset attributes are modified
    for (unsigned i = 0; i < GetNumAttributes() && !isModified; i++)
        isModified |= GetAttribute(i) != GetAttributeDefault(i);

    if (isModified)
    {
        JSONValue root{ };
        if (SaveJSON(root))
        {
            JSONFile file(context_);
            file.GetRoot() = root;
            if (file.SaveFile(assetPath))
                return true;
        }
    }
    else
    {
        GetFileSystem()->Delete(assetPath);
        return true;
    }

    URHO3D_LOGERROR("Saving {} failed.", assetPath);
    return false;
}

bool Asset::Load()
{
    ea::string assetPath = RemoveTrailingSlash(resourcePath_) + ".asset";
    if (!GetFileSystem()->FileExists(assetPath))
        // Default instance.
        return true;

    JSONFile file(context_);
    if (file.LoadFile(assetPath))
    {
        if (LoadJSON(file.GetRoot()))
            return true;
    }

    URHO3D_LOGERROR("Loading {} failed.", assetPath);
    return false;
}

bool Asset::SaveJSON(JSONValue& dest) const
{
    if (!Serializable::SaveJSON(dest))
        return false;

    auto* project = GetSubsystem<Project>();
    StringVector flavors = project->GetPipeline().GetFlavors();
    auto it = flavors.find(DEFAULT_PIPELINE_FLAVOR);
    ea::swap(*it, *flavors.begin());                    // default first
    ea::quick_sort(flavors.begin() + 1, flavors.end()); // sort the rest

    JSONValue& flavorsDest = dest["flavors"];
    flavorsDest.SetType(JSON_OBJECT);

    for (const ea::string& flavor : flavors)
    {
        JSONValue& flavorDest = flavorsDest[flavor];
        flavorDest.SetType(JSON_OBJECT);

        JSONValue& importersDest = flavorDest["importers"];
        importersDest.SetType(JSON_OBJECT);

        // TODO: sort importers by type name
        for (const auto& importer : importers_.find(flavor)->second)
        {
            JSONValue& importerDest = importersDest[importer->GetTypeName()];
            importerDest.SetType(JSON_OBJECT);
            if (!importer->SaveJSON(importerDest))
                return false;
        }
    }

    return true;
}

bool Asset::LoadJSON(const JSONValue& source)
{
    if (!Serializable::LoadJSON(source))
        return false;

    const JSONValue& flavorsSrc = source["flavors"];
    if (!flavorsSrc.IsObject())
        return false;

    auto* project = GetSubsystem<Project>();
    for (const ea::string& flavor : project->GetPipeline().GetFlavors())
    {
        const JSONValue& flavorSrc = flavorsSrc[flavor];
        if (!flavorSrc.IsObject())
            return false;

        const JSONValue& importersSrc = flavorSrc["importers"];
        if (!importersSrc.IsObject())
            return false;

        for (const auto& importerSrc : importersSrc)
        {
            if (!importerSrc.second.IsObject())
                return false;
            SharedPtr<AssetImporter> importer(context_->CreateObject(importerSrc.first)->Cast<AssetImporter>());
            importer->Initialize(this, flavor);
            if (!importer->LoadJSON(importerSrc.second))
                return false;
            importers_[flavor].emplace_back(importer);
        }
    }

    for (const ea::string& flavor : project->GetPipeline().GetFlavors())
    {
        for (const auto& importer : importers_[flavor])
            importer->SetInheritedDefaultsIfSet();
    }

    return true;
}

void Asset::AddFlavor(const ea::string& name)
{
    if (importers_.contains(name))
        return;

    auto* project = GetSubsystem<Project>();
    ea::vector<SharedPtr<AssetImporter>>& importers = importers_[name];
    for (StringHash importerType : project->GetPipeline().importers_)
    {
        SharedPtr<AssetImporter> importer(context_->CreateObject(importerType)->Cast<AssetImporter>());
        importer->Initialize(this, name);
        importers.emplace_back(importer);
    }
}

void Asset::RemoveFlavor(const ea::string& name)
{
    if (name == DEFAULT_PIPELINE_FLAVOR)
        return;
    importers_.erase(name);
}

void Asset::RenameFlavor(const ea::string& oldName, const ea::string& newName)
{
    if (oldName == DEFAULT_PIPELINE_FLAVOR || newName == DEFAULT_PIPELINE_FLAVOR)
        return;

    if (!importers_.contains(oldName) || importers_.contains(newName))
        return;

    ea::swap(importers_[oldName], importers_[newName]);

    importers_.erase(oldName);
}

void Asset::UpdateExtraInspectors(const ea::string& resourceName)
{
    auto it = extraInspectors_.find(resourceName);
    if (it != extraInspectors_.end())
    {
        currentExtraInspectorProvider_ = it->second.Get();
        return;
    }

    ResourceInspector* inspector = nullptr;
    switch (::Urho3D::GetContentType(context_, resourceName))
    {
    case CTYPE_MODEL:
    {
        currentExtraInspectorProvider_ = inspector = extraInspectors_[resourceName] =
            context_->CreateObject<ModelInspector>();
        break;
    }
    case CTYPE_MATERIAL:
    {
        currentExtraInspectorProvider_ = inspector = extraInspectors_[resourceName] =
            context_->CreateObject<MaterialInspector>();
        break;
    }
    default:
        currentExtraInspectorProvider_ = nullptr;
        break;
    }

    if (inspector != nullptr)
        inspector->SetResource(resourceName);
}

}
