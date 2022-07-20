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
#include "../Project/CreateComponentMenu.h"

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
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
#include <Urho3D/SystemUI/NodeInspectorWidget.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_RewindSimulation = HotkeyInfo{"SceneViewTab.RewindSimulation"}.Press(KEY_UNKNOWN);
const auto Hotkey_TogglePaused = HotkeyInfo{"SceneViewTab.TogglePaused"}.Press(KEY_PAUSE);

const auto Hotkey_Cut = HotkeyInfo{"SceneViewTab.Cut"}.Ctrl().Press(KEY_X);
const auto Hotkey_Copy = HotkeyInfo{"SceneViewTab.Copy"}.Ctrl().Press(KEY_C);
const auto Hotkey_Paste = HotkeyInfo{"SceneViewTab.Paste"}.Ctrl().Press(KEY_V);
const auto Hotkey_PasteInto = HotkeyInfo{"SceneViewTab.PasteInto"}.Ctrl().Shift().Press(KEY_V);
const auto Hotkey_Delete = HotkeyInfo{"SceneViewTab.Delete"}.Press(KEY_DELETE);
const auto Hotkey_Duplicate = HotkeyInfo{"SceneViewTab.Duplicate"}.Ctrl().Press(KEY_D);

const auto Hotkey_Focus = HotkeyInfo{"SceneViewTab.Focus"}.Press(KEY_F);

const auto Hotkey_CreateSiblingNode = HotkeyInfo{"SceneViewTab.CreateSiblingNode"}.Ctrl().Press(KEY_N);
const auto Hotkey_CreateChildNode = HotkeyInfo{"SceneViewTab.CreateChildNode"}.Ctrl().Shift().Press(KEY_N);

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

void SceneViewPage::StartSimulation()
{
    simulationBase_ = PackedSceneData::FromScene(scene_);
    selection_.Save(selectionBase_);
}

void SceneViewPage::RewindSimulation()
{
    scene_->SetUpdateEnabled(false);
    if (simulationBase_)
    {
        simulationBase_->ToScene(scene_);
        selection_.Load(scene_, selectionBase_);
        simulationBase_ = ea::nullopt;
        selectionBase_.Clear();
    }
}

void SceneViewPage::BeginSelection()
{
    selection_.Update();
    selection_.Save(oldSelection_);
}

void SceneViewPage::EndSelection(SceneViewTab* owner)
{
    selection_.Save(newSelection_);
    if (oldSelection_ != newSelection_)
        owner->PushAction<ChangeSceneSelectionAction>(this, oldSelection_, newSelection_);
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

bool SceneViewTab::ByInputPriority::operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const
{
    return lhs->GetInputPriority() > rhs->GetInputPriority();
}

bool SceneViewTab::ByToolbarPriority::operator()(const SharedPtr<SceneViewAddon>& lhs, const SharedPtr<SceneViewAddon>& rhs) const
{
    return lhs->GetToolbarPriority() < rhs->GetToolbarPriority();
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
    BindHotkey(Hotkey_PasteInto, &SceneViewTab::PasteIntoSelection);
    BindHotkey(Hotkey_Delete, &SceneViewTab::DeleteSelection);
    BindHotkey(Hotkey_Duplicate, &SceneViewTab::DuplicateSelection);
    BindHotkey(Hotkey_Focus, &SceneViewTab::FocusSelection);
    BindHotkey(Hotkey_CreateSiblingNode, &SceneViewTab::CreateNodeNextToSelection);
    BindHotkey(Hotkey_CreateChildNode, &SceneViewTab::CreateNodeInSelection);
}

SceneViewTab::~SceneViewTab()
{
}

void SceneViewTab::RegisterAddon(const SharedPtr<SceneViewAddon>& addon)
{
    addons_.push_back(addon);
    addonsByInputPriority_.insert(addon);
    addonsByToolbarPriority_.insert(addon);
    addonsByName_.insert(addon);
}

void SceneViewTab::SetupPluginContext()
{
    SceneViewPage* activePage = GetActivePage();

    auto engine = GetSubsystem<Engine>();
    if (activePage)
    {
        engine->SetParameter(Param_SceneName, GetActiveResourceName());
        engine->SetParameter(Param_ScenePosition, activePage->renderer_->GetCameraPosition());
        engine->SetParameter(Param_SceneRotation, activePage->renderer_->GetCameraRotation());
    }
    else
    {
        engine->SetParameter(Param_SceneName, Variant::EMPTY);
        engine->SetParameter(Param_ScenePosition, Variant::EMPTY);
        engine->SetParameter(Param_SceneRotation, Variant::EMPTY);
    }
}

void SceneViewTab::RenderEditMenu(Scene* scene, SceneSelection& selection)
{
    const bool hasSelection = !selection.GetNodes().empty() || !selection.GetComponents().empty();
    const bool hasClipboard = clipboard_.HasData();

    if (ui::MenuItem("Cut", GetHotkeyLabel(Hotkey_Cut).c_str(), false, hasSelection))
        CutSelection(selection);
    if (ui::MenuItem("Copy", GetHotkeyLabel(Hotkey_Copy).c_str(), false, hasSelection))
        CopySelection(selection);
    if (ui::MenuItem("Paste", GetHotkeyLabel(Hotkey_Paste).c_str(), false, hasClipboard))
        PasteNextToSelection(scene, selection);
    if (ui::MenuItem("Paste Into", GetHotkeyLabel(Hotkey_PasteInto).c_str(), false, hasClipboard))
        PasteIntoSelection(scene, selection);
    if (ui::MenuItem("Delete", GetHotkeyLabel(Hotkey_Delete).c_str(), false, hasSelection))
        DeleteSelection(selection);
    if (ui::MenuItem("Duplicate", GetHotkeyLabel(Hotkey_Duplicate).c_str(), false, hasSelection))
        DuplicateSelection(selection);

    ui::Separator();

    if (ui::MenuItem("Focus", GetHotkeyLabel(Hotkey_Focus).c_str(), false, hasSelection))
        FocusSelection(selection);
}

void SceneViewTab::RenderCreateMenu(Scene* scene, SceneSelection& selection)
{
    if (ui::MenuItem("Create Node", GetHotkeyLabel(Hotkey_CreateSiblingNode).c_str()))
        CreateNodeNextToSelection(scene, selection);

    if (ui::MenuItem("Create Child Node", GetHotkeyLabel(Hotkey_CreateChildNode).c_str()))
        CreateNodeInSelection(scene, selection);

    ui::MenuItem("Create Component:", nullptr, false, false);
    ui::Indent();
    if (const auto componentType = RenderCreateComponentMenu(context_))
        CreateComponentInSelection(scene, selection, *componentType);
    ui::Unindent();
}

void SceneViewTab::ResumeSimulation()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    if (!activePage->simulationBase_)
    {
        PushAction<EmptyEditorAction>();
        activePage->StartSimulation();
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

void SceneViewTab::CutSelection(SceneSelection& selection)
{
    CopySelection(selection);
    DeleteSelection(selection);
}

void SceneViewTab::CopySelection(SceneSelection& selection)
{
    const auto& selectedNodes = selection.GetNodes();
    const auto& selectedComponents = selection.GetComponents();

    if (!selectedNodes.empty())
        clipboard_ = PackedSceneData::FromNodes(selectedNodes.begin(), selectedNodes.end());
    else if (!selectedComponents.empty())
        clipboard_ = PackedSceneData::FromComponents(selectedComponents.begin(), selectedComponents.end());
}

void SceneViewTab::PasteNextToSelection(Scene* scene, SceneSelection& selection)
{
    if (clipboard_.HasNodes())
    {
        Node* siblingNode = selection.GetActiveNodeOrScene();
        Node* parentNode = siblingNode && siblingNode->GetParent() ? siblingNode->GetParent() : scene;

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
        PasteIntoSelection(scene, selection);
    }
}

void SceneViewTab::PasteIntoSelection(Scene* scene, SceneSelection& selection)
{
    // Copy because selection changes during paste
    auto parentNodes = selection.GetEffectiveNodesAndScenes();
    if (parentNodes.empty())
        parentNodes.emplace(scene);

    if (clipboard_.HasNodes())
    {
        selection.Clear();
        for (Node* selectedNode : parentNodes)
        {
            for (const PackedNodeData& packedNode : clipboard_.GetNodes())
            {
                Node* newNode = packedNode.SpawnCopy(selectedNode);
                selection.SetSelected(newNode, true);
                PushAction<CreateRemoveNodeAction>(newNode, false);
            }
        }
    }
    else if (clipboard_.HasComponents())
    {
        selection.Clear();
        for (Node* selectedNode : parentNodes)
        {
            for (const PackedComponentData& packedComponent : clipboard_.GetComponents())
            {
                Component* newComponent = packedComponent.SpawnCopy(selectedNode);
                if (componentSelection_)
                    selection.SetSelected(newComponent, true);
                else
                    selection.SetSelected(selectedNode, true);
                PushAction<CreateRemoveComponentAction>(newComponent, false);
            }
        }
    }
}

void SceneViewTab::DeleteSelection(SceneSelection& selection)
{
    const auto& selectedNodes = selection.GetNodes();
    const auto& selectedComponents = selection.GetComponents();

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

    selection.Clear();
}

void SceneViewTab::DuplicateSelection(SceneSelection& selection)
{
    if (!selection.GetNodes().empty())
    {
        // Copy because selection changes during paste
        const auto selectedNodes = selection.GetNodes();
        selection.Clear();

        for (Node* node : selectedNodes)
        {
            Node* parent = node->GetParent();
            URHO3D_ASSERT(parent);

            const auto data = PackedNodeData{node};
            Node* newNode = data.SpawnCopy(parent);
            PushAction<CreateRemoveNodeAction>(newNode, false);
            selection.SetSelected(newNode, true);
        }
    }
    else if (!selection.GetComponents().empty())
    {
        // Copy because selection changes during paste
        const auto selectedComponents = selection.GetComponents();
        selection.Clear();

        for (Component* component : selectedComponents)
        {
            Node* node = component->GetNode();
            URHO3D_ASSERT(node);

            const auto data = PackedComponentData{component};
            Component* newComponent = data.SpawnCopy(node);
            PushAction<CreateRemoveComponentAction>(newComponent, false);
            if (componentSelection_)
                selection.SetSelected(newComponent, true);
            else
                selection.SetSelected(node, true);
        }
    }
}

void SceneViewTab::CreateNodeNextToSelection(Scene* scene, SceneSelection& selection)
{
    Node* siblingNode = selection.GetActiveNodeOrScene();
    Node* parentNode = siblingNode && siblingNode->GetParent() ? siblingNode->GetParent() : scene;

    Node* newNode = parentNode->CreateChild();
    selection.Clear();
    selection.SetSelected(newNode, true);
    PushAction<CreateRemoveNodeAction>(newNode, false);
}

void SceneViewTab::CreateNodeInSelection(Scene* scene, SceneSelection& selection)
{
    // Copy because selection changes during paste
    auto parentNodes = selection.GetEffectiveNodesAndScenes();
    if (parentNodes.empty())
        parentNodes.emplace(scene);

    selection.Clear();
    for (Node* selectedNode : parentNodes)
    {
        Node* newNode = selectedNode->CreateChild();
        selection.SetSelected(newNode, true);
        PushAction<CreateRemoveNodeAction>(newNode, false);
    }
}

void SceneViewTab::CreateComponentInSelection(Scene* scene, SceneSelection& selection, StringHash componentType)
{
    // Copy because selection changes during paste
    auto parentNodes = selection.GetEffectiveNodesAndScenes();
    if (parentNodes.empty())
        parentNodes.emplace(scene);

    selection.Clear();
    for (Node* selectedNode : parentNodes)
    {
        Component* newComponent = selectedNode->CreateComponent(componentType);
        if (componentSelection_)
            selection.SetSelected(newComponent, true);
        else
            selection.SetSelected(selectedNode, true);
        PushAction<CreateRemoveComponentAction>(newComponent, false);
    }
}

void SceneViewTab::FocusSelection(SceneSelection& selection)
{
    if (Node* activeNode = selection.GetActiveNode())
    {
        if (SceneViewPage* page = GetPage(activeNode->GetScene()))
            OnLookAt(this, *page, activeNode->GetWorldPosition());
    }
}

void SceneViewTab::CutSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        CutSelection(activePage->selection_);
}

void SceneViewTab::CopySelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        CopySelection(activePage->selection_);
}

void SceneViewTab::PasteNextToSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        PasteNextToSelection(activePage->scene_, activePage->selection_);
}

void SceneViewTab::PasteIntoSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        PasteIntoSelection(activePage->scene_, activePage->selection_);
}

void SceneViewTab::DeleteSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        DeleteSelection(activePage->selection_);
}

void SceneViewTab::DuplicateSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        DuplicateSelection(activePage->selection_);
}

void SceneViewTab::CreateNodeNextToSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        CreateNodeNextToSelection(activePage->scene_, activePage->selection_);
}

void SceneViewTab::CreateNodeInSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        CreateNodeInSelection(activePage->scene_, activePage->selection_);
}

void SceneViewTab::FocusSelection()
{
    if (SceneViewPage* activePage = GetActivePage())
        FocusSelection(activePage->selection_);
}

void SceneViewTab::RenderMenu()
{
    if (ui::BeginMenu("Edit"))
    {
        RenderEditMenuItems();

        if (SceneViewPage* activePage = GetActivePage())
        {
            ui::Separator();
            RenderEditMenu(activePage->scene_, activePage->selection_);
        }

        ui::EndMenu();
    }

    if (SceneViewPage* activePage = GetActivePage())
    {
        if (ui::BeginMenu("Create"))
        {
            RenderCreateMenu(activePage->scene_, activePage->selection_);
            ui::EndMenu();
        }
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

    for (SceneViewAddon* addon : addonsByToolbarPriority_)
    {
        if (addon->RenderToolbar())
            Widgets::ToolbarSeparator();
    }
}

bool SceneViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<Scene>();
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

ea::optional<EditorActionFrame> SceneViewTab::PushAction(SharedPtr<EditorAction> action)
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return ea::nullopt;

    // Ignore all actions while simulating
    if (activePage->simulationBase_)
        return ea::nullopt;

    const auto wrappedAction = MakeShared<RewindSceneActionWrapper>(action, activePage);
    return BaseClassName::PushAction(wrappedAction);
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
    if (SceneViewPage* activePage = GetActivePage())
    {
        if (!componentSelection_)
            activePage->selection_.ConvertToNodes();
        activePage->BeginSelection();
    }
}

void SceneViewTab::PostRenderUpdate()
{
    if (SceneViewPage* activePage = GetActivePage())
        activePage->EndSelection(this);
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
    project->ProcessRequest(request, this);
}

SceneViewPage* SceneViewTab::GetPage(const ea::string& resourceName)
{
    const auto iter = scenes_.find(resourceName);
    return iter != scenes_.end() ? iter->second : nullptr;
}

SceneViewPage* SceneViewTab::GetPage(Scene* scene)
{
    const auto iter = ea::find_if(scenes_.begin(), scenes_.end(), [scene](const auto& pair) { return pair.second->scene_ == scene; });
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
    SetSelection(newSelection_);
}

void ChangeSceneSelectionAction::Undo() const
{
    SetSelection(oldSelection_);
}

void ChangeSceneSelectionAction::SetSelection(const PackedSceneSelection& selection) const
{
    if (page_)
    {
        page_->selection_.Load(page_->scene_, selection);
        page_->BeginSelection();
    }
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
