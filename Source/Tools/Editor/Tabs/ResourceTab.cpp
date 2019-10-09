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

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Octree.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Toolbox/IO/ContentUtilities.h>
#include <SDL/SDL_clipboard.h>

#include "EditorEvents.h"
#include "Inspector/MaterialInspector.h"
#include "Inspector/ModelInspector.h"
#include "Tabs/Scene/SceneTab.h"
#include "Tabs/UI/UITab.h"
#include "Tabs/InspectorTab.h"
#include "Pipeline/Importers/ModelImporter.h"
#include "Editor.h"
#include "ResourceTab.h"

namespace Urho3D
{

static ea::unordered_map<ContentType, ea::string> contentToTabType{
    {CTYPE_SCENE, "SceneTab"},
    {CTYPE_UILAYOUT, "UITab"},
};

ResourceTab::ResourceTab(Context* context)
    : Tab(context)
{
    SetID("29d1a5dc-6b8d-4a27-bfb2-a84417f33ee2");
    SetTitle("Resources");
    isUtility_ = true;

    SubscribeToEvent(E_INSPECTORLOCATERESOURCE, [&](StringHash, VariantMap& args) {
        auto resourceName = args[InspectorLocateResource::P_NAME].GetString();

        auto* project = GetSubsystem<Project>();
        auto* fs = GetFileSystem();

        resourcePath_ = GetPath(resourceName);
        if (fs->FileExists(project->GetCachePath() + resourceName))
        {
            // File is in the cache. resourcePath_ should point to a directory of source resource. For example:
            // We have Resources/Models/cube.fbx which is a source model.
            // It is converted to Cache/Models/cube.fbx/Model.mdl
            // ResourceBrowserWidget() expects:
            // * resourcePath = Models/ (path same as if cube.fbx was selected)
            // * resourceSelection = cube.fbx/Model.mdl (selection also includes a directory which resides in cache)
            while (!fs->DirExists(project->GetResourcePath() + resourcePath_))
                resourcePath_ = GetParentPath(resourcePath_);
            resourceSelection_ = resourceName.substr(resourcePath_.length());
        }
        else
            resourceSelection_ = GetFileNameAndExtension(resourceName);
        flags_ |= RBF_SCROLL_TO_CURRENT;
        if (ui::GetIO().KeyCtrl)
            SelectCurrentItemInspector();
    });
    SubscribeToEvent(E_RESOURCERENAMED, [&](StringHash, VariantMap& args) {
        using namespace ResourceRenamed;
        const ea::string& from = args[P_FROM].GetString();
        const ea::string& to = args[P_TO].GetString();
        if (from == resourcePath_ + resourceSelection_)
        {
            resourcePath_ = GetParentPath(to);
            resourceSelection_ = GetFileNameAndExtension(RemoveTrailingSlash(to));
            if (to.ends_with("/"))
                resourceSelection_ = AddTrailingSlash(resourceSelection_);
        }
    });
    SubscribeToEvent(E_RESOURCEBROWSERDELETE, [&](StringHash, VariantMap& args) {
        using namespace ResourceBrowserDelete;
        auto* project = GetSubsystem<Project>();
        auto fileName = project->GetResourcePath() + args[P_NAME].GetString();
        if (GetFileSystem()->FileExists(fileName))
            GetFileSystem()->Delete(fileName);
        else if (GetFileSystem()->DirExists(fileName))
            GetFileSystem()->RemoveDir(fileName, true);
    });
}

bool ResourceTab::RenderWindowContent()
{
    auto* project = GetSubsystem<Project>();
    auto action = ResourceBrowserWidget(project->GetResourcePath(), project->GetCachePath(), resourcePath_,
        resourceSelection_, flags_);
    if (action == RBR_ITEM_OPEN)
    {
        ea::string selected = resourcePath_ + resourceSelection_;
        auto it = contentToTabType.find(GetContentType(context_, selected));
        if (it != contentToTabType.end())
        {
            if (auto* tab = GetSubsystem<Editor>()->GetTab(it->second))
            {
                if (tab->IsUtility())
                {
                    // Tabs that can be opened once.
                    tab->LoadResource(selected);
                    tab->Activate();
                }
                else
                {
                    // Tabs that can be opened multiple times.
                    if ((tab = GetSubsystem<Editor>()->GetTabByResource(selected)))
                        tab->Activate();
                    else if ((tab = GetSubsystem<Editor>()->CreateTab(it->second)))
                    {
                        tab->LoadResource(selected);
                        tab->AutoPlace();
                        tab->Activate();
                    }
                }
            }
            else if ((tab = GetSubsystem<Editor>()->CreateTab(it->second)))
            {
                // Tabs that can be opened multiple times.
                tab->LoadResource(selected);
                tab->AutoPlace();
                tab->Activate();
            }
        }
        else
        {
            // Unknown resources are opened with associated application.
            ea::string resourcePath = GetSubsystem<Project>()->GetResourcePath() + selected;
            if (!GetFileSystem()->Exists(resourcePath))
                resourcePath = GetSubsystem<Project>()->GetCachePath() + selected;

            if (GetFileSystem()->Exists(resourcePath))
                GetFileSystem()->SystemOpen(resourcePath);
        }
    }
    else if (action == RBR_ITEM_CONTEXT_MENU)
        ui::OpenPopup("Resource Context Menu");
    else if (action == RBR_ITEM_SELECTED)
        SelectCurrentItemInspector();

    flags_ = RBF_NONE;

    bool hasSelection = !resourceSelection_.empty();
    if (hasSelection && ui::IsWindowFocused())
    {
        if (ui::IsKeyReleased(SCANCODE_F2))
            flags_ |= RBF_RENAME_CURRENT;

        if (ui::IsKeyReleased(SCANCODE_DELETE))
            flags_ |= RBF_DELETE_CURRENT;
    }

    if (ui::BeginPopup("Resource Context Menu"))
    {
        if (ui::BeginMenu("Create"))
        {
            if (ui::MenuItem(ICON_FA_FOLDER " Folder"))
            {
                ea::string newFolderName("New Folder");
                ea::string path = GetNewResourcePath(resourcePath_ + newFolderName);
                if (GetFileSystem()->CreateDir(path))
                {
                    flags_ |= RBF_RENAME_CURRENT | RBF_SCROLL_TO_CURRENT;
                    resourceSelection_ = newFolderName;
                }
                else
                    URHO3D_LOGERRORF("Failed creating folder '%s'.", path.c_str());
            }

            if (ui::MenuItem("Scene"))
            {
                auto path = GetNewResourcePath(resourcePath_ + "New Scene.xml");
                GetFileSystem()->CreateDirsRecursive(GetPath(path));

                SharedPtr<Scene> scene(new Scene(context_));
                scene->CreateComponent<Octree>();
                File file(context_, path, FILE_WRITE);
                if (file.IsOpen())
                {
                    scene->SaveXML(file);
                    flags_ |= RBF_RENAME_CURRENT | RBF_SCROLL_TO_CURRENT;
                    resourceSelection_ = GetFileNameAndExtension(path);
                }
                else
                    URHO3D_LOGERRORF("Failed opening file '%s'.", path.c_str());
            }

            if (ui::MenuItem("Material"))
            {
                auto path = GetNewResourcePath(resourcePath_ + "New Material.xml");
                GetFileSystem()->CreateDirsRecursive(GetPath(path));

                SharedPtr<Material> material(new Material(context_));
                File file(context_, path, FILE_WRITE);
                if (file.IsOpen())
                {
                    material->Save(file);
                    flags_ |= RBF_RENAME_CURRENT | RBF_SCROLL_TO_CURRENT;
                    resourceSelection_ = GetFileNameAndExtension(path);
                }
                else
                    URHO3D_LOGERRORF("Failed opening file '%s'.", path.c_str());
            }

            if (ui::MenuItem("UI Layout"))
            {
                auto path = GetNewResourcePath(resourcePath_ + "New UI Layout.xml");
                GetFileSystem()->CreateDirsRecursive(GetPath(path));

                SharedPtr<UIElement> scene(new UIElement(context_));
                XMLFile layout(context_);
                auto root = layout.GetOrCreateRoot("element");
                if (scene->SaveXML(root) && layout.SaveFile(path))
                {
                    flags_ |= RBF_RENAME_CURRENT | RBF_SCROLL_TO_CURRENT;
                    resourceSelection_ = GetFileNameAndExtension(path);
                }
                else
                    URHO3D_LOGERRORF("Failed opening file '%s'.", path.c_str());
            }

            ui::EndMenu();
        }

        if (!hasSelection)
            ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);

        if (ui::MenuItem("Copy Path") && hasSelection)
            SDL_SetClipboardText((resourcePath_ + resourceSelection_).c_str());

        if (ui::MenuItem("Rename", "F2") && hasSelection)
            flags_ |= RBF_RENAME_CURRENT;

        if (ui::MenuItem("Delete", "Del") && hasSelection)
            flags_ |= RBF_DELETE_CURRENT;

        if (!hasSelection)
            ui::PopStyleColor();

        using namespace EditorResourceContextMenu;
        ea::string selected = resourcePath_ + resourceSelection_;
        ContentType ctype = GetContentType(context_, selected);
        SendEvent(E_EDITORRESOURCECONTEXTMENU, P_CTYPE, ctype, P_RESOURCENAME, selected);

        ui::EndPopup();
    }

    return true;
}

ea::string ResourceTab::GetNewResourcePath(const ea::string& name)
{
    auto* project = GetSubsystem<Project>();
    if (!GetFileSystem()->FileExists(project->GetResourcePath() + name))
        return project->GetResourcePath() + name;

    auto basePath = GetPath(name);
    auto baseName = GetFileName(name);
    auto ext = GetExtension(name, false);

    for (auto i = 1; i < M_MAX_INT; i++)
    {
        auto newName = project->GetResourcePath() + ToString("%s%s %d%s", basePath.c_str(), baseName.c_str(), i, ext.c_str());
        if (!GetFileSystem()->FileExists(newName))
            return newName;
    }

    std::abort();
}

void ResourceTab::ClearSelection()
{
    inspectors_.clear();
    resourceSelection_.clear();
}

void ResourceTab::RenderInspector(const char* filter)
{
    for (auto& pair : inspectors_)
    {
        if (pair.first.NotNull())
            pair.second->RenderInspector(filter);
    }
}

void ResourceTab::SelectCurrentItemInspector()
{
    ea::string selected = resourcePath_ + resourceSelection_;

    inspectors_.clear();

    auto* pipeline = GetSubsystem<Pipeline>();
    if (Asset* asset = pipeline->GetAsset(selected))
    {
        // This is a meta-asset or a source asset whose byproducts we would like to view.
        inspectors_.push_back({SharedPtr(asset), dynamic_cast<IInspectorProvider*>(asset)});

        StringVector byproducts;
        for (AssetImporter* importer : asset->GetImporters(pipeline->GetDefaultFlavor()))
            byproducts.insert(byproducts.end(), importer->GetByproducts().begin(), importer->GetByproducts().end());

        ea::quick_sort(byproducts.begin(), byproducts.end());
        for (const ea::string& resourceName : byproducts)
        {
            if (ResourceInspector* inspector = CreateInspector(resourceName))
                inspectors_.push_back({SharedPtr(inspector), dynamic_cast<IInspectorProvider*>(inspector)});
        }
    }

    // This may be a byproduct, or preprocessed resource.
    if (ResourceInspector* inspector = CreateInspector(selected))
        inspectors_.push_back({SharedPtr(inspector), dynamic_cast<IInspectorProvider*>(inspector)});

    GetSubsystem<Editor>()->GetTab<InspectorTab>()->SetProvider(this);

    using namespace EditorResourceSelected;
    SendEvent(E_EDITORRESOURCESELECTED, P_CTYPE, GetContentType(context_, selected), P_RESOURCENAME, selected);
}

ResourceInspector* ResourceTab::CreateInspector(const ea::string& resourceName) const
{
    ResourceInspector* result = nullptr;
    switch (GetContentType(context_, resourceName))
    {
    case CTYPE_MODEL:
        result = context_->CreateObject<ModelInspector>().Detach();
        break;
    case CTYPE_MATERIAL:
        result = context_->CreateObject<MaterialInspector>().Detach();
        break;
    default:
        break;
    }

    if (result != nullptr)
        result->SetResource(resourceName);

    return result;
}

}
