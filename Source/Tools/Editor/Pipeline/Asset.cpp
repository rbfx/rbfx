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
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include <Toolbox/SystemUI/Widgets.h>
#include <Toolbox/SystemUI/ResourceBrowser.h>

#include "EditorEvents.h"
#include "Project.h"
#include "Pipeline/Pipeline.h"
#include "Pipeline/Asset.h"
#include "Pipeline/Importers/ModelImporter.h"


namespace Urho3D
{

Asset::Asset(Context* context)
    : Serializable(context)
{
    SubscribeToEvent(E_RESOURCERENAMED, [this](StringHash, VariantMap& args) {
        using namespace ResourceRenamed;
        const ea::string& from = args[P_FROM].GetString();
        const ea::string& to = args[P_TO].GetString();

        bool isDir = from.ends_with("/");
        if (isDir)
        {
            if (!name_.starts_with(from))
                return;
        }
        else
        {
            if (name_ != from)
                return;
        }

        if (extraInspectors_.contains(from))
        {
            extraInspectors_[to] = extraInspectors_[from];
            extraInspectors_.erase(from);
        }

        auto* fs = context_->GetFileSystem();
        auto* project = GetSubsystem<Project>();
        ea::string newName = to + (isDir ? name_.substr(from.size()) : "");

        ea::string assetPathFrom = project->GetResourcePath() + RemoveTrailingSlash(name_) + ".asset";
        if (fs->FileExists(assetPathFrom))
        {
            ea::string assetPathTo = project->GetResourcePath() + RemoveTrailingSlash(newName) + ".asset";
            if (!fs->Rename(assetPathFrom, assetPathTo))
            {
                URHO3D_LOGERROR("Failed to rename '{}' to '{}'", assetPathFrom, assetPathTo);
                return;
            }
        }

        name_ = newName;
        resourcePath_ = project->GetResourcePath() + name_;
    });

    SubscribeToEvent(E_RESOURCEBROWSERDELETE, [this](StringHash, VariantMap& args) {
        using namespace ResourceBrowserDelete;
        extraInspectors_.erase(args[P_NAME].GetString());
    });

    SubscribeToEvent(E_EDITORFLAVORADDED, [this](StringHash, VariantMap& args) { OnFlavorAdded(args); });
    SubscribeToEvent(E_EDITORFLAVORREMOVED, [this](StringHash, VariantMap& args) { OnFlavorRemoved(args); });
}

void Asset::RegisterObject(Context* context)
{
    context->RegisterFactory<Asset>();
}

void Asset::SetName(const ea::string& name)
{
    assert(name_.empty());
    auto* project = GetSubsystem<Project>();
    resourcePath_ = project->GetResourcePath() + name;
    name_ = name;
    contentType_ = ::Urho3D::GetContentType(context_, name);
}

bool Asset::IsOutOfDate(Flavor* flavor) const
{
    for (const auto& importer : GetImporters(flavor))
    {
        if (importer->IsOutOfDate())
            return true;
    }
    return false;
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
    auto* project = GetSubsystem<Project>();
    if (currentExtraInspectorProvider_)
    {
        auto* fs = GetSubsystem<FileSystem>();

        ea::string resourceFileName = context_->GetCache()->GetResourceFileName(currentExtraInspectorProvider_->GetResourceName());
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

    // Use flavors list from the pipeline because it is sorted. Asset::importers_ is unordered.
    for (const SharedPtr<Flavor>& flavor : project->GetPipeline()->GetFlavors())
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
                // TODO: Look into subdirectories and show only importers valid for contents of the folder.
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
                    tabVisible = ui::BeginTabItem(flavor->GetName().c_str());
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
            isModified |= !importer->GetByproducts().empty();
            if (isModified)
                break;
        }
    }

    // Check if any of asset attributes are modified
    for (unsigned i = 0; i < GetNumAttributes() && !isModified; i++)
        isModified |= GetAttribute(i) != GetAttributeDefault(i);

    if (isModified)
    {
        JSONFile file(context_);
        JSONOutputArchive archive(&file);
        if (Serialize(archive))
        {
            if (file.SaveFile(assetPath))
                return true;
        }
    }
    else
    {
        context_->GetFileSystem()->Delete(assetPath);
        return true;
    }

    URHO3D_LOGERROR("Saving {} failed.", assetPath);
    return false;
}

bool Asset::Load()
{
    assert(!name_.empty());
    ea::string assetPath = RemoveTrailingSlash(resourcePath_) + ".asset";
    JSONFile file(context_);
    if (context_->GetFileSystem()->FileExists(assetPath))
    {
        if (!file.LoadFile(assetPath))
        {
            URHO3D_LOGERROR("Loading {} failed.", assetPath);
            return false;
        }
    }

    JSONInputArchive archive(&file);
    if (!Serialize(archive))
        return false;

    // Initialize flavor importers.
    for (Flavor* flavor : GetSubsystem<Pipeline>()->GetFlavors())
        AddFlavor(flavor);

    return true;
}

bool Asset::Serialize(Archive& archive)
{
    if (auto block = archive.OpenUnorderedBlock("asset"))
    {
        if (!BaseClassName::Serialize(archive, block))
            return false;

        auto* pipeline = GetSubsystem<Pipeline>();
        const ea::vector<SharedPtr<Flavor>>& flavors = pipeline->GetFlavors();
        if (auto block = archive.OpenUnorderedBlock("flavors"))
        {
            for (unsigned i = 0; i < flavors.size(); i++)
            {
                SharedPtr<Flavor> flavor = flavors[i];
                if (auto block = archive.OpenUnorderedBlock(flavor->GetName().c_str()))
                {
                    if (auto block = archive.OpenUnorderedBlock("importers"))
                    {
                        for (const TypeInfo* importerType : pipeline->GetImporterTypes())
                        {
                            SharedPtr<AssetImporter> importer;
                            if (archive.IsInput())
                            {
                                importer = context_->CreateObject(importerType->GetType())->Cast<AssetImporter>();
                                importer->Initialize(this, flavor);
                                importers_[flavor].emplace_back(importer);
                            }
                            else
                                importer = GetImporter(flavor, importerType->GetType());
                            if (auto block = archive.OpenUnorderedBlock(importerType->GetTypeName().c_str()))
                            {
                                if (!importer->Serialize(archive, block))
                                    return false;
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

void Asset::AddFlavor(Flavor* flavor)
{
    if (importers_.contains(SharedPtr(flavor)))
        return;

    ea::vector<SharedPtr<AssetImporter>>& importers = importers_[SharedPtr(flavor)];
    for (const TypeInfo* importerType : GetSubsystem<Pipeline>()->GetImporterTypes())
    {
        SharedPtr<AssetImporter> importer(context_->CreateObject(importerType->GetType())->Cast<AssetImporter>());
        importer->Initialize(this, flavor);
        importers.emplace_back(importer);
    }
}

const ea::vector<SharedPtr<AssetImporter>>& Asset::GetImporters(Flavor* flavor) const
{
    static ea::vector<SharedPtr<AssetImporter>> empty;
    auto it = importers_.find(SharedPtr(flavor));
    if (it == importers_.end())
        return empty;
    return it->second;
}

AssetImporter* Asset::GetImporter(Flavor* flavor, StringHash type) const
{
    for (AssetImporter* importer : GetImporters(flavor))
    {
        if (importer->GetType() == type)
            return importer;
    }
    return nullptr;
}

void Asset::ReimportOutOfDateRecursive() const
{
    if (!IsMetaAsset())
        return;

    auto* fs = context_->GetFileSystem();
    auto* project = GetSubsystem<Project>();
    Pipeline* pipeline = project->GetPipeline();

    StringVector files;
    fs->ScanDir(files, GetResourcePath(), "", SCAN_FILES, true);

    for (const ea::string& file : files)
    {
        if (Asset* asset = pipeline->GetAsset(GetName() + file))
        {
            if (asset->IsOutOfDate(pipeline->GetDefaultFlavor()))
                pipeline->ScheduleImport(asset);
        }
    }
}

void Asset::OnFlavorAdded(VariantMap& args)
{
    using namespace EditorFlavorAdded;
    auto* flavor = static_cast<Flavor*>(args[P_FLAVOR].GetPtr());
    AddFlavor(flavor);
}

void Asset::OnFlavorRemoved(VariantMap& args)
{
    using namespace EditorFlavorRemoved;
    auto* flavor = static_cast<Flavor*>(args[P_FLAVOR].GetPtr());
    importers_.erase(SharedPtr(flavor));
}

}
