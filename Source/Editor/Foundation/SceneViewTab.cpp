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
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/SystemUI/NodeInspectorWidget.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_RewindSimulation = EditorHotkey{"SceneViewTab.RewindSimulation"}.Press(KEY_UNKNOWN);
const auto Hotkey_TogglePaused = EditorHotkey{"SceneViewTab.TogglePaused"}.Press(KEY_PAUSE);

const auto Hotkey_Cut = EditorHotkey{"SceneViewTab.Cut"}.Ctrl().Press(KEY_X);
const auto Hotkey_Copy = EditorHotkey{"SceneViewTab.Copy"}.Ctrl().Press(KEY_C);
const auto Hotkey_Paste = EditorHotkey{"SceneViewTab.Paste"}.Ctrl().Press(KEY_V);
const auto Hotkey_PasteInto = EditorHotkey{"SceneViewTab.PasteInto"}.Ctrl().Shift().Press(KEY_V);
const auto Hotkey_Delete = EditorHotkey{"SceneViewTab.Delete"}.Press(KEY_DELETE);
const auto Hotkey_Duplicate = EditorHotkey{"SceneViewTab.Duplicate"}.Ctrl().Press(KEY_D);

const auto Hotkey_Focus = EditorHotkey{"SceneViewTab.Focus"}.Press(KEY_F);
const auto Hotkey_MoveToLatest = EditorHotkey{"SceneViewTab.MoveToLatest"};
const auto Hotkey_MovePositionToLatest = EditorHotkey{"SceneViewTab.MovePositionToLatest"};
const auto Hotkey_MoveRotationToLatest = EditorHotkey{"SceneViewTab.MoveRotationToLatest"};
const auto Hotkey_MoveScaleToLatest = EditorHotkey{"SceneViewTab.MoveScaleToLatest"};

const auto Hotkey_CreateSiblingNode = EditorHotkey{"SceneViewTab.CreateSiblingNode"}.Ctrl().Press(KEY_N);
const auto Hotkey_CreateChildNode = EditorHotkey{"SceneViewTab.CreateChildNode"}.Ctrl().Shift().Press(KEY_N);

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

void Foundation_SceneViewTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<SceneViewTab>(context));
}

SceneViewPage::SceneViewPage(Resource* resource, Scene* scene)
    : Object(scene->GetContext())
    , resource_(resource)
    , scene_(scene)
    , renderer_(MakeShared<SceneRendererToTexture>(scene_))
    , cfgFileName_(scene_->GetFileName() + ".user.json")
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

bool SceneViewPage::IsSimulationActive() const
{
    return currentSimulationAction_ && !currentSimulationAction_->IsComplete();
}

void SceneViewPage::StartSimulation(SceneViewTab* owner)
{
    currentSimulationAction_ = owner->PushAction<SimulateSceneAction>(this);
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
    BindHotkey(Hotkey_RewindSimulation, &SceneViewTab::RewindSimulation);
    BindHotkey(Hotkey_TogglePaused, &SceneViewTab::ToggleSimulationPaused);
    BindHotkey(Hotkey_Cut, &SceneViewTab::CutSelection);
    BindHotkey(Hotkey_Copy, &SceneViewTab::CopySelection);
    BindHotkey(Hotkey_Paste, &SceneViewTab::PasteNextToSelection);
    BindHotkey(Hotkey_PasteInto, &SceneViewTab::PasteIntoSelection);
    BindHotkey(Hotkey_Delete, &SceneViewTab::DeleteSelection);
    BindHotkey(Hotkey_Duplicate, &SceneViewTab::DuplicateSelection);
    BindHotkey(Hotkey_Focus, &SceneViewTab::FocusSelection);
    BindHotkey(Hotkey_MoveToLatest, &SceneViewTab::MoveSelectionToLatest);
    BindHotkey(Hotkey_MovePositionToLatest, &SceneViewTab::MoveSelectionPositionToLatest);
    BindHotkey(Hotkey_MoveRotationToLatest, &SceneViewTab::MoveSelectionRotationToLatest);
    BindHotkey(Hotkey_MoveScaleToLatest, &SceneViewTab::MoveSelectionScaleToLatest);
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
    const bool hasClipboard = clipboard_.HasNodesOrComponents();

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

    if (ui::MenuItem("Move to Latest", GetHotkeyLabel(Hotkey_MoveToLatest).c_str(), false, hasSelection))
        MoveSelectionToLatest(selection);
    if (ui::BeginMenu("Move Attribute to Latest...", hasSelection))
    {
        if (ui::MenuItem("Position", GetHotkeyLabel(Hotkey_MovePositionToLatest).c_str(), false, hasSelection))
            MoveSelectionPositionToLatest(selection);
        if (ui::MenuItem("Rotation", GetHotkeyLabel(Hotkey_MoveRotationToLatest).c_str(), false, hasSelection))
            MoveSelectionRotationToLatest(selection);
        if (ui::MenuItem("Scale", GetHotkeyLabel(Hotkey_MoveScaleToLatest).c_str(), false, hasSelection))
            MoveSelectionScaleToLatest(selection);
        ui::EndMenu();
    }

    if (SceneViewPage* activePage = GetActivePage())
    {
        ui::Separator();
        OnSelectionEditMenu(this, *activePage, scene, selection);
    }
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

    if (!activePage->IsSimulationActive())
        activePage->StartSimulation(this);
    activePage->scene_->SetUpdateEnabled(true);
}

void SceneViewTab::PauseSimulation()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    if (activePage->IsSimulationActive())
        activePage->currentSimulationAction_->Complete(true);
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
    if (!activePage || !activePage->IsSimulationActive())
        return;

    // Simulation is stored as EditorAction, so we can just undo it
    UndoManager* undoManager = GetUndoManager();
    undoManager->Undo();
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
        clipboard_ = PackedNodeComponentData::FromNodes(selectedNodes.begin(), selectedNodes.end());
    else if (!selectedComponents.empty())
        clipboard_ = PackedNodeComponentData::FromComponents(selectedComponents.begin(), selectedComponents.end());
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

void SceneViewTab::MoveSelectionToLatest(SceneSelection& selection)
{
    if (Node* activeNode = selection.GetActiveNode())
    {
        const Matrix3x4 worldTransform = activeNode->GetWorldTransform();

        for (Node* node : selection.GetNodes())
        {
            if (node && node != activeNode)
                node->SetWorldTransform(worldTransform);
        }
    }
}

void SceneViewTab::MoveSelectionPositionToLatest(SceneSelection& selection)
{
    if (Node* activeNode = selection.GetActiveNode())
    {
        const Vector3 worldPosition = activeNode->GetWorldPosition();

        for (Node* node : selection.GetNodes())
        {
            if (node && node != activeNode)
                node->SetWorldPosition(worldPosition);
        }
    }
}

void SceneViewTab::MoveSelectionRotationToLatest(SceneSelection& selection)
{
    if (Node* activeNode = selection.GetActiveNode())
    {
        const Quaternion worldRotation = activeNode->GetWorldRotation();

        for (Node* node : selection.GetNodes())
        {
            if (node && node != activeNode)
                node->SetWorldRotation(worldRotation);
        }
    }
}

void SceneViewTab::MoveSelectionScaleToLatest(SceneSelection& selection)
{
    if (Node* activeNode = selection.GetActiveNode())
    {
        const Vector3 worldScale = activeNode->GetWorldScale();

        for (Node* node : selection.GetNodes())
        {
            if (node && node != activeNode)
                node->SetWorldScale(worldScale);
        }
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

void SceneViewTab::MoveSelectionToLatest()
{
    if (SceneViewPage* activePage = GetActivePage())
        MoveSelectionToLatest(activePage->selection_);
}

void SceneViewTab::MoveSelectionPositionToLatest()
{
    if (SceneViewPage* activePage = GetActivePage())
        MoveSelectionPositionToLatest(activePage->selection_);
}

void SceneViewTab::MoveSelectionRotationToLatest()
{
    if (SceneViewPage* activePage = GetActivePage())
        MoveSelectionRotationToLatest(activePage->selection_);
}

void SceneViewTab::MoveSelectionScaleToLatest()
{
    if (SceneViewPage* activePage = GetActivePage())
        MoveSelectionScaleToLatest(activePage->selection_);
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
        const bool canRewind = activePage && activePage->IsSimulationActive();
        ui::BeginDisabled(!canRewind);
        if (Widgets::ToolbarButton(ICON_FA_CIRCLE_CHEVRON_LEFT, "Rewind Scene Simulation"))
            RewindSimulation();
        ui::EndDisabled();
    }

    {
        const bool isStarted = activePage && activePage->IsSimulationActive();
        const bool isUpdating = activePage && activePage->scene_->IsUpdateEnabled();
        const char* label = isUpdating ? ICON_FA_CIRCLE_PAUSE : ICON_FA_CIRCLE_PLAY;
        const char* tooltip = isUpdating ? "Pause Scene Simulation" : (isStarted ? "Resume Scene Simulation" : "Start Scene Simulation");
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
    if (activePage->IsSimulationActive())
        return ea::nullopt;

    if (auto selectionAction = dynamic_cast<ChangeSceneSelectionAction*>(action.Get()))
        return BaseClassName::PushAction(action);

    const auto wrappedAction = MakeShared<PreserveSceneSelectionWrapper>(action, activePage);
    return BaseClassName::PushAction(wrappedAction);
}

void SceneViewTab::RenderContextMenuItems()
{
    BaseClassName::RenderContextMenuItems();

    if (SceneViewPage* activePage = GetActivePage())
    {
        contextMenuSeparator_.Reset();

        const char* rewindTitle = ICON_FA_CIRCLE_CHEVRON_LEFT " Rewind Scene Simulation";
        const char* rewindShortcut = GetHotkeyLabel(Hotkey_RewindSimulation).c_str();
        if (ui::MenuItem(rewindTitle, rewindShortcut, false, activePage->IsSimulationActive()))
            RewindSimulation();

        const char* pauseTitle = !activePage->scene_->IsUpdateEnabled()
            ? (activePage->IsSimulationActive() ? ICON_FA_CIRCLE_PLAY " Resume Scene Simulation" : ICON_FA_CIRCLE_PLAY " Start Scene Simulation")
            : ICON_FA_CIRCLE_PAUSE " Pause Scene Simulation";
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

    const bool isActive = resourceName == GetActiveResourceName();
    scenes_[resourceName] = CreatePage(xmlFile, isActive);
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

    if (GetActiveResourceName() == resourceName)
        SavePagePreview(*page);
}

void SceneViewTab::SavePageScene(SceneViewPage& page) const
{
    XMLFile xmlFile(context_);
    XMLElement rootElement = xmlFile.GetOrCreateRoot("scene");

    page.scene_->SetUpdateEnabled(false);
    page.scene_->SaveXML(rootElement);

    VectorBuffer buffer;
    xmlFile.Save(buffer);

    auto sharedBuffer = ea::make_shared<ByteVector>(ea::move(buffer.GetBuffer()));

    auto project = GetProject();
    project->SaveFileDelayed(page.resource_->GetAbsoluteFileName(), page.resource_->GetName(), sharedBuffer);

    auto cache = GetSubsystem<ResourceCache>();
    cache->ReleaseResource(page.resource_->GetName(), true);
}

void SceneViewTab::SavePagePreview(SceneViewPage& page) const
{
    Texture2D* texture = page.renderer_->GetTexture();
    SharedPtr<Image> image = texture->GetImage();

    if (!image)
        return;

    // Crop to square
    const IntVector2 size{image->GetWidth(), image->GetHeight()};
    const int minSide = ea::min(size.x_, size.y_);
    const IntVector2 excess = size - IntVector2::ONE * minSide;
    const IntVector2 offsetTopLeft = excess / 2;
    const IntVector2 offsetRightBottom = excess - offsetTopLeft;

    const IntRect rect{offsetTopLeft, size - offsetRightBottom};
    SharedPtr<Image> croppedImage = image->GetSubimage(rect);

    const ea::string& path = GetProject()->GetPreviewPngPath();
    croppedImage->SavePNG(path);
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
    Widgets::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));

    const auto contentAreaMin = ToVector2(ui::GetItemRectMin());
    const auto contentAreaMax = ToVector2(ui::GetItemRectMax());
    activePage->contentArea_ = Rect{contentAreaMin, contentAreaMax};

    UpdateCameraRay();
    UpdateAddons(*activePage);
}

void SceneViewTab::UpdateAddons(SceneViewPage& page)
{
    bool mouseConsumed = false;

    if (UpdateDropToScene())
        mouseConsumed = true;

    for (SceneViewAddon* addon : addonsByInputPriority_)
        addon->ProcessInput(page, mouseConsumed);

    for (SceneViewAddon* addon : addons_)
        addon->Render(page);
}

void SceneViewTab::UpdateCameraRay()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    ImGuiIO& io = ui::GetIO();
    Camera* camera = activePage->renderer_->GetCamera();

    const ImRect viewportRect{ui::GetItemRectMin(), ui::GetItemRectMax()};
    const auto pos = ToVector2((io.MousePos - viewportRect.Min) / viewportRect.GetSize());
    activePage->cameraRay_ = camera->GetScreenRay(pos.x_, pos.y_);
}

bool SceneViewTab::UpdateDropToScene()
{
    SceneViewPage* activePage = GetActivePage();
    if (activePage && ui::BeginDragDropTarget())
    {
        const auto payload = DragDropPayload::Get();

        if (!dragAndDropAddon_)
        {
            const auto isPayloadSupported = [&](SceneViewAddon* addon) { return addon->IsDragDropPayloadSupported(*activePage, payload); };
            const auto iter = ea::find_if(addonsByInputPriority_.begin(), addonsByInputPriority_.end(), isPayloadSupported);

            dragAndDropAddon_ = iter != addonsByInputPriority_.end() ? *iter : nullptr;
            if (dragAndDropAddon_)
                dragAndDropAddon_->BeginDragDrop(*activePage, payload);
        }

        if (dragAndDropAddon_)
        {
            if (ui::AcceptDragDropPayload(DragDropPayloadType.c_str(), ImGuiDragDropFlags_AcceptBeforeDelivery))
            {
                dragAndDropAddon_->UpdateDragDrop(payload);
                if (ui::GetDragDropPayload()->IsDelivery())
                {
                    dragAndDropAddon_->CompleteDragDrop(payload);
                    dragAndDropAddon_ = nullptr;
                }
            }
        }

        ui::EndDragDropTarget();
    }
    else if (dragAndDropAddon_)
    {
        dragAndDropAddon_->CancelDragDrop();
        dragAndDropAddon_ = nullptr;
    }
    return dragAndDropAddon_ != nullptr;
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

SharedPtr<SceneViewPage> SceneViewTab::CreatePage(XMLFile* xmlFile, bool isActive)
{
    auto scene = MakeShared<Scene>(context_);
    scene->LoadXML(xmlFile->GetRoot());
    scene->SetFileName(xmlFile->GetAbsoluteFileName());
    scene->SetUpdateEnabled(false);

    auto page = MakeShared<SceneViewPage>(xmlFile, scene);

    WeakPtr<SceneViewPage> weakPage{page};
    page->SubscribeToEvent(xmlFile, E_RELOADFINISHED, [this, weakPage](StringHash, VariantMap&)
    {
        if (weakPage && !IsResourceUnsaved(weakPage->resource_->GetName()))
        {
            const PackedSceneSelection selection = weakPage->selection_.Pack();
            auto xmlFile = static_cast<XMLFile*>(weakPage->resource_.Get());
            weakPage->scene_->LoadXML(xmlFile->GetRoot());
            weakPage->selection_.Load(weakPage->scene_, selection);
        }
    });

    page->renderer_->SetActive(isActive);

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
    auto fs = GetSubsystem<FileSystem>();
    auto jsonFile = MakeShared<JSONFile>(context_);
    if (fs->FileExists(page.cfgFileName_))
    {
        if (jsonFile->LoadFile(page.cfgFileName_))
            jsonFile->LoadObject("Scene", page, this);
    }
}

SimulateSceneAction::SimulateSceneAction(SceneViewPage* page)
    : page_(page)
{
    oldData_ = PackedSceneData::FromScene(page->scene_);
    page->selection_.Save(oldSelection_);
}

void SimulateSceneAction::Complete(bool force)
{
    if (!force)
        return;

    isComplete_ = true;
    if (page_)
    {
        page_->scene_->SetElapsedTime(0.0f);
        page_->scene_->SetUpdateEnabled(false);
        newData_ = PackedSceneData::FromScene(page_->scene_);
        page_->selection_.Save(newSelection_);
    }
}

bool SimulateSceneAction::CanUndoRedo() const
{
    return page_ != nullptr && newData_.HasSceneData();
}

void SimulateSceneAction::SetState(const PackedSceneData& data, const PackedSceneSelection& selection) const
{
    page_->scene_->SetUpdateEnabled(false);
    data.ToScene(page_->scene_);
    page_->selection_.Load(page_->scene_, selection);
}

void SimulateSceneAction::Redo() const
{
    SetState(newData_, newSelection_);
}

void SimulateSceneAction::Undo() const
{
    SetState(oldData_, oldSelection_);
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

PreserveSceneSelectionWrapper::PreserveSceneSelectionWrapper(SharedPtr<EditorAction> action, SceneViewPage* page)
    : BaseEditorActionWrapper(action)
    , page_(page)
    , selection_(page->selection_.Pack())
{
}

bool PreserveSceneSelectionWrapper::CanRedo() const
{
    return page_ && action_->CanRedo();
}

void PreserveSceneSelectionWrapper::Redo() const
{
    action_->Redo();
    page_->selection_.Load(page_->scene_, selection_);
}

bool PreserveSceneSelectionWrapper::CanUndo() const
{
    return page_ && action_->CanUndo();
}

void PreserveSceneSelectionWrapper::Undo() const
{
    action_->Undo();
    page_->selection_.Load(page_->scene_, selection_);
}

bool PreserveSceneSelectionWrapper::MergeWith(const EditorAction& other)
{
    const auto otherWrapper = dynamic_cast<const PreserveSceneSelectionWrapper*>(&other);
    if (!otherWrapper)
        return false;

    if (page_ != otherWrapper->page_ || selection_ != otherWrapper->selection_)
        return false;

    return action_->MergeWith(*otherWrapper->action_);
}

ea::vector<RayQueryResult> QueryGeometriesFromScene(Scene* scene, const Ray& ray, RayQueryLevel level, float maxDistance, unsigned viewMask)
{
    ea::vector<RayQueryResult> results;
    RayOctreeQuery query(results, ray, level, maxDistance, DRAWABLE_GEOMETRY, viewMask);
    scene->GetComponent<Octree>()->Raycast(query);
    return results;
}

}
