//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/CommonEditorActions.h"
#include "../Core/IniHelpers.h"
#include "../Foundation/SceneViewTab.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Plugins/PluginManager.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_RewindSimulation, "SceneViewTab.RewindSimulation", QUAL_NONE, KEY_UNKNOWN);
URHO3D_EDITOR_HOTKEY(Hotkey_TogglePaused, "SceneViewTab.TogglePaused", QUAL_NONE, KEY_PAUSE);

URHO3D_EDITOR_HOTKEY(Hotkey_Cut, "SceneViewTab.Cut", QUAL_CTRL, KEY_X);
URHO3D_EDITOR_HOTKEY(Hotkey_Copy, "SceneViewTab.Copy", QUAL_CTRL, KEY_C);
URHO3D_EDITOR_HOTKEY(Hotkey_Paste, "SceneViewTab.Paste", QUAL_CTRL, KEY_V);
URHO3D_EDITOR_HOTKEY(Hotkey_Delete, "SceneViewTab.Delete", QUAL_NONE, KEY_DELETE);

}

void SerializeValue(Archive& archive, const char* name, SceneViewPage& page, const SceneViewTab* owner)
{
    auto block = archive.OpenUnorderedBlock(name);

    {
        PackedSceneSelection selection;
        if (!archive.IsInput())
            page.selection_.Save(selection);

        SerializeOptionalValue(archive, "Selection", selection, AlwaysSerialize{});

        if (archive.IsInput())
            page.selection_.Load(page.scene_, selection);
    }

    {
        auto addonsBlock = archive.OpenUnorderedBlock("Addons");
        for (const SceneViewAddon* addon : owner->GetAddonsByName())
        {
            ea::any& state = page.GetAddonData(*addon);
            SerializeOptionalValue(archive, addon->GetUniqueName().c_str(), state, AlwaysSerialize{},
                [&](Archive& archive, const char* name, ea::any& value)
            {
                addon->SerializePageState(archive, name, value);
            });
        }
    }
}

void Foundation_SceneViewTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<SceneViewTab>(context));
}

SceneViewPage::SceneViewPage(Scene* scene)
    : Object(scene->GetContext())
    , scene_(scene)
    , renderer_(MakeShared<SceneRendererToTexture>(scene_))
    , cfgFileName_(scene_->GetFileName() + ".cfg")
{
}

SceneViewPage::~SceneViewPage()
{
}

ea::any& SceneViewPage::GetAddonData(const SceneViewAddon& addon)
{
    auto& [addonPtr, addonData] = addonData_[addon.GetUniqueName()];
    if (addonPtr != &addon)
    {
        addonPtr = &addon;
        addonData = ea::any{};
    }
    return addonData;
}

void SceneViewPage::RewindSimulation()
{
    scene_->SetUpdateEnabled(false);
    if (simulationBase_)
    {
        simulationBase_->ToScene(scene_);
        simulationBase_ = ea::nullopt;
    }
}

SceneViewAddon::SceneViewAddon(SceneViewTab* owner)
    : Object(owner->GetContext())
    , owner_(owner)
{
}

SceneViewAddon::~SceneViewAddon()
{
}

void SceneViewAddon::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

void SceneViewAddon::SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const
{
    // Just open empty block
    EmptySerializableObject placeholder;
    SerializeOptionalValue(archive, name, placeholder, AlwaysSerialize{});
}

bool SceneViewTab::ByPriority::operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const
{
    return lhs->GetInputPriority() > rhs->GetInputPriority();
}

bool SceneViewTab::ByName::operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const
{
    return lhs->GetUniqueName() < rhs->GetUniqueName();
}

SceneViewTab::SceneViewTab(Context* context)
    : ResourceEditorTab(context, "Scene", "9f4f7432-dd60-4c83-aecd-2f6cf69d3549",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault | EditorTabFlag::FocusOnStart,
        EditorTabPlacement::DockCenter)
{
    auto project = GetProject();
    project->IgnoreFileNamePattern("*.xml.cfg");

    BindHotkey(Hotkey_RewindSimulation, &SceneViewTab::RewindSimulation);
    BindHotkey(Hotkey_TogglePaused, &SceneViewTab::ToggleSimulationPaused);
    BindHotkey(Hotkey_Cut, &SceneViewTab::CutSelection);
    BindHotkey(Hotkey_Copy, &SceneViewTab::CopySelection);
    BindHotkey(Hotkey_Paste, &SceneViewTab::PasteNextToSelection);
    BindHotkey(Hotkey_Delete, &SceneViewTab::DeleteSelection);
}

SceneViewTab::~SceneViewTab()
{
}

void SceneViewTab::RegisterAddon(const SharedPtr<SceneViewAddon>& addon)
{
    addons_.push_back(addon);
    addonsByInputPriority_.insert(addon);
    addonsByName_.insert(addon);
}

void SceneViewTab::SetupPluginContext()
{
    SceneViewPage* activePage = GetActivePage();

    auto pluginManager = GetSubsystem<PluginManager>();
    if (activePage)
    {
        pluginManager->SetParameter(Plugin_SceneName, GetActiveResourceName());
        pluginManager->SetParameter(Plugin_ScenePosition, activePage->renderer_->GetCameraPosition());
        pluginManager->SetParameter(Plugin_SceneRotation, activePage->renderer_->GetCameraRotation());
    }
    else
    {
        pluginManager->SetParameter(Plugin_SceneName, Variant::EMPTY);
        pluginManager->SetParameter(Plugin_ScenePosition, Variant::EMPTY);
        pluginManager->SetParameter(Plugin_SceneRotation, Variant::EMPTY);
    }
}

void SceneViewTab::ResumeSimulation()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    if (!activePage->simulationBase_)
    {
        PushAction<EmptyEditorAction>();
        activePage->simulationBase_ = PackedSceneData::FromScene(activePage->scene_);
    }
    activePage->scene_->SetUpdateEnabled(true);
}

void SceneViewTab::PauseSimulation()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    activePage->scene_->SetUpdateEnabled(false);
}

void SceneViewTab::ToggleSimulationPaused()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    if (activePage->scene_->IsUpdateEnabled())
        PauseSimulation();
    else
        ResumeSimulation();
}

void SceneViewTab::RewindSimulation()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    // This is a little hack used to rewind consistently both via menu and via Undo action
    auto project = GetProject();
    UndoManager* undoManager = project->GetUndoManager();
    undoManager->Undo();
    //activePage->RewindSimulation();
}

void SceneViewTab::CutSelection()
{
    CopySelection();
    DeleteSelection();
}

void SceneViewTab::CopySelection()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    SceneSelection& selection = activePage->selection_;
    const auto& selectedNodes = selection.GetNodes();
    const auto& selectedComponents = selection.GetComponents();

    if (!selectedNodes.empty())
        clipboard_ = PackedSceneData::FromNodes(selectedNodes.begin(), selectedNodes.end());
    else if (!selectedComponents.empty())
        clipboard_ = PackedSceneData::FromComponents(selectedComponents.begin(), selectedComponents.end());
}

void SceneViewTab::PasteNextToSelection()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    SceneSelection& selection = activePage->selection_;
    if (clipboard_.HasNodes())
    {
        Node* siblingNode = selection.GetActiveNodeOrScene();
        Node* parentNode = siblingNode && siblingNode->GetParent() ? siblingNode->GetParent() : activePage->scene_;

        selection.Clear();
        for (const PackedNodeData& packedNode : clipboard_.GetNodes())
        {
            Node* newNode = packedNode.SpawnCopy(parentNode);
            selection.SetSelected(newNode, true);
            PushAction<CreateRemoveNodeAction>(newNode, false);
        }
    }
    else if (clipboard_.HasComponents())
    {
        Node* parentNode = selection.GetActiveNodeOrScene();

        selection.Clear();
        for (const PackedComponentData& packedComponent : clipboard_.GetComponents())
        {
            Component* newComponent = packedComponent.SpawnCopy(parentNode);
            selection.SetSelected(newComponent, true);
            PushAction<CreateRemoveComponentAction>(newComponent, false);
        }
    }
}

void SceneViewTab::DeleteSelection()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    const auto& selectedNodes = activePage->selection_.GetNodes();
    const auto& selectedComponents = activePage->selection_.GetComponents();

    for (Node* node : selectedNodes)
    {
        if (node && node->GetParent() != nullptr)
        {
            PushAction<CreateRemoveNodeAction>(node, true);
            node->Remove();
        }
    }

    for (Component* component : selectedComponents)
    {
        if (component)
        {
            PushAction<CreateRemoveComponentAction>(component, true);
            component->Remove();
        }
    }

    activePage->selection_.Clear();
}

void SceneViewTab::RenderMenu()
{
    if (ui::BeginMenu("Edit"))
    {
        RenderEditMenuItems();

        ui::Separator();

        if (ui::MenuItem("Cut", GetHotkeyLabel(Hotkey_Cut).c_str()))
            CutSelection();
        if (ui::MenuItem("Copy", GetHotkeyLabel(Hotkey_Copy).c_str()))
            CopySelection();
        if (ui::MenuItem("Paste", GetHotkeyLabel(Hotkey_Paste).c_str()))
            PasteNextToSelection();
        if (ui::MenuItem("Delete", GetHotkeyLabel(Hotkey_Delete).c_str()))
            DeleteSelection();

        ui::Separator();

        ui::EndMenu();
    }
}

void SceneViewTab::RenderToolbar()
{
    SceneViewPage* activePage = GetActivePage();

    {
        const bool canRewind = activePage && activePage->simulationBase_;
        ui::BeginDisabled(!canRewind);
        if (Widgets::ToolbarButton(ICON_FA_BACKWARD_FAST, "Rewind Simulation"))
            RewindSimulation();
        ui::EndDisabled();
    }

    {
        const bool isStarted = activePage && activePage->simulationBase_;
        const bool isUpdating = activePage && activePage->scene_->IsUpdateEnabled();
        const char* label = isUpdating ? ICON_FA_PAUSE : ICON_FA_PLAY;
        const char* tooltip = isUpdating ? "Pause Simulation" : (isStarted ? "Resume Simulation" : "Start Simulation");
        ui::BeginDisabled(!activePage);
        if (Widgets::ToolbarButton(label, tooltip))
            ToggleSimulationPaused();
        ui::EndDisabled();
    }

    Widgets::ToolbarSeparator();
}

bool SceneViewTab::CanOpenResource(const OpenResourceRequest& request)
{
    return request.GetXMLFile() && request.GetTypeHint() == "scene";
}

void SceneViewTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    ResourceEditorTab::WriteIniSettings(output);
    for (SceneViewAddon* addon : addons_)
        addon->WriteIniSettings(output);
}

void SceneViewTab::ReadIniSettings(const char* line)
{
    ResourceEditorTab::ReadIniSettings(line);
    for (SceneViewAddon* addon : addons_)
        addon->ReadIniSettings(line);
}

void SceneViewTab::PushAction(SharedPtr<EditorAction> action)
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    // Ignore all actions while simulating
    if (activePage->simulationBase_)
        return;

    const auto wrappedAction = MakeShared<RewindSceneActionWrapper>(action, activePage);
    BaseClassName::PushAction(wrappedAction);
}

void SceneViewTab::RenderContextMenuItems()
{
    BaseClassName::RenderContextMenuItems();

    if (SceneViewPage* activePage = GetActivePage())
    {
        contextMenuSeparator_.Reset();

        const char* rewindTitle = ICON_FA_BACKWARD_FAST " Rewind Simulation";
        const char* rewindShortcut = GetHotkeyLabel(Hotkey_RewindSimulation).c_str();
        if (ui::MenuItem(rewindTitle, rewindShortcut, false, !!activePage->simulationBase_))
            RewindSimulation();

        const char* pauseTitle = !activePage->scene_->IsUpdateEnabled()
            ? (activePage->simulationBase_ ? ICON_FA_PLAY " Resume Simulation" : ICON_FA_PLAY " Start Simulation")
            : ICON_FA_PAUSE " Pause Simulation";
        if (ui::MenuItem(pauseTitle, GetHotkeyLabel(Hotkey_TogglePaused).c_str()))
            ToggleSimulationPaused();
    }

    contextMenuSeparator_.Add();

    for (SceneViewAddon* addon : addonsByName_)
    {
        if (addon->RenderTabContextMenu())
            contextMenuSeparator_.Reset();
    }

    contextMenuSeparator_.Add();
}

void SceneViewTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    BaseClassName::ApplyHotkeys(hotkeyManager);

    for (SceneViewAddon* addon : addons_)
        addon->ApplyHotkeys(hotkeyManager);
}

void SceneViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto xmlFile = cache->GetResource<XMLFile>(resourceName);

    if (!xmlFile)
    {
        URHO3D_LOGERROR("Cannot load scene file '%s'", resourceName);
        return;
    }

    auto scene = MakeShared<Scene>(context_);
    scene->LoadXML(xmlFile->GetRoot());
    scene->SetFileName(xmlFile->GetAbsoluteFileName());
    scene->SetUpdateEnabled(false);

    const bool isActive = resourceName == GetActiveResourceName();
    scenes_[resourceName] = CreatePage(scene, isActive);
}

void SceneViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    scenes_.erase(resourceName);
}

void SceneViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
    if (SceneViewPage* oldActivePage = GetPage(oldResourceName))
        oldActivePage->scene_->SetUpdateEnabled(false);

    for (const auto& [name, data] : scenes_)
        data->renderer_->SetActive(name == newResourceName);

    if (SceneViewPage* newActivePage = GetPage(newResourceName))
        InspectSelection(*newActivePage);
}

void SceneViewTab::OnResourceSaved(const ea::string& resourceName)
{
    SceneViewPage* page = GetPage(resourceName);
    if (!page)
        return;

    SavePageConfig(*page);
    SavePageScene(*page);
}

void SceneViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
    SceneViewPage* page = GetPage(resourceName);
    if (!page)
        return;

    SavePageConfig(*page);
}

void SceneViewTab::SavePageScene(SceneViewPage& page) const
{
    XMLFile xmlFile(context_);
    XMLElement rootElement = xmlFile.GetOrCreateRoot("scene");

    page.scene_->SetUpdateEnabled(false);
    page.scene_->SaveXML(rootElement);

    xmlFile.SaveFile(page.scene_->GetFileName());
}

void SceneViewTab::PreRenderUpdate()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
    {
        oldSelectionResourceName_.clear();
        oldSelection_.Clear();
        return;
    }

    activePage->selection_.Update();

    oldSelectionResourceName_ = GetActiveResourceName();
    activePage->selection_.Save(oldSelection_);
}

void SceneViewTab::PostRenderUpdate()
{
    SceneViewPage* activePage = GetActivePage();
    if (activePage && oldSelectionResourceName_ == GetActiveResourceName())
    {
        activePage->selection_.Save(newSelection_);
        if (oldSelection_ != newSelection_)
            PushAction<ChangeSceneSelectionAction>(activePage, oldSelection_, newSelection_);
    }
}

void SceneViewTab::RenderContent()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    if (!activePage->scene_->HasComponent<DebugRenderer>())
    {
        auto debug = activePage->scene_->GetOrCreateComponent<DebugRenderer>();
        debug->SetTemporary(true);
        debug->SetLineAntiAlias(true);
    }

    activePage->renderer_->SetTextureSize(GetContentSize());
    activePage->renderer_->Update();

    const ImVec2 basePosition = ui::GetCursorPos();

    Texture2D* sceneTexture = activePage->renderer_->GetTexture();
    ui::SetCursorPos(basePosition);
    ui::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));

    const auto contentAreaMin = ToVector2(ui::GetItemRectMin());
    const auto contentAreaMax = ToVector2(ui::GetItemRectMax());
    activePage->contentArea_ = Rect{contentAreaMin, contentAreaMax};

    UpdateAddons(*activePage);
}

void SceneViewTab::UpdateAddons(SceneViewPage& page)
{
    bool mouseConsumed = false;
    for (SceneViewAddon* addon : addonsByInputPriority_)
        addon->ProcessInput(page, mouseConsumed);

    for (SceneViewAddon* addon : addons_)
        addon->Render(page);
}

void SceneViewTab::InspectSelection(SceneViewPage& page)
{
    auto project = GetProject();
    auto request = MakeShared<InspectNodeComponentRequest>(context_, page.selection_.GetNodesAndScenes(), page.selection_.GetComponents());
    if (!request->IsEmpty())
        project->ProcessRequest(request, this);
}

SceneViewPage* SceneViewTab::GetPage(const ea::string& resourceName)
{
    auto iter = scenes_.find(resourceName);
    return iter != scenes_.end() ? iter->second : nullptr;
}

SceneViewPage* SceneViewTab::GetActivePage()
{
    return GetPage(GetActiveResourceName());
}

SharedPtr<SceneViewPage> SceneViewTab::CreatePage(Scene* scene, bool isActive)
{
    auto page = MakeShared<SceneViewPage>(scene);

    page->renderer_->SetActive(isActive);

    WeakPtr<SceneViewPage> weakPage{page};
    page->selection_.OnChanged.Subscribe(this, [weakPage](SceneViewTab* self)
    {
        if (weakPage)
            self->InspectSelection(*weakPage);
    });

    LoadPageConfig(*page);
    for (SceneViewAddon* addon : addons_)
        addon->Initialize(*page);
    return page;
}

void SceneViewTab::SavePageConfig(const SceneViewPage& page) const
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    jsonFile->SaveObject("Scene", page, this);
    jsonFile->SaveFile(page.cfgFileName_);
}

void SceneViewTab::LoadPageConfig(SceneViewPage& page) const
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    jsonFile->LoadFile(page.cfgFileName_);
    jsonFile->LoadObject("Scene", page, this);
}

RewindSceneActionWrapper::RewindSceneActionWrapper(SharedPtr<EditorAction> action, SceneViewPage* page)
    : BaseEditorActionWrapper(action)
    , page_(page)
{

}

bool RewindSceneActionWrapper::CanRedo() const
{
    return page_ && BaseEditorActionWrapper::CanRedo();
}

void RewindSceneActionWrapper::Redo() const
{
    page_->RewindSimulation();
    BaseEditorActionWrapper::Redo();
}

bool RewindSceneActionWrapper::CanUndo() const
{
    return page_ && BaseEditorActionWrapper::CanUndo();
}

void RewindSceneActionWrapper::Undo() const
{
    page_->RewindSimulation();
    BaseEditorActionWrapper::Undo();
}

ChangeSceneSelectionAction::ChangeSceneSelectionAction(SceneViewPage* page,
    const PackedSceneSelection& oldSelection, const PackedSceneSelection& newSelection)
    : page_(page)
    , oldSelection_(oldSelection)
    , newSelection_(newSelection)
{
}

void ChangeSceneSelectionAction::Redo() const
{
    if (page_)
        page_->selection_.Load(page_->scene_, newSelection_);
}

void ChangeSceneSelectionAction::Undo() const
{
    if (page_)
        page_->selection_.Load(page_->scene_, oldSelection_);
}

bool ChangeSceneSelectionAction::MergeWith(const EditorAction& other)
{
    auto otherAction = dynamic_cast<const ChangeSceneSelectionAction*>(&other);
    if (!otherAction)
        return false;

    if (page_ != otherAction->page_)
        return false;

    newSelection_ = otherAction->newSelection_;
    return true;
}

}
