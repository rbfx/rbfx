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
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>

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

void SerializeValue(Archive& archive, const char* name, SceneViewPage& page)
{
    auto block = archive.OpenUnorderedBlock(name);

    SerializeOptionalValue(archive, "CurrentCameraController", page.currentCameraController_, AlwaysSerialize{});

    for (SceneCameraController* controller : page.cameraControllers_)
        SerializeOptionalValue(archive, controller->GetTypeName().c_str(), *controller, AlwaysSerialize{});
}

void Foundation_SceneViewTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<SceneViewTab>(context));
}

SceneCameraController::SceneCameraController(Scene* scene, Camera* camera)
    : Object(scene->GetContext())
    , scene_(scene)
    , camera_(camera)
{
}

SceneCameraController::~SceneCameraController()
{
}

Vector2 SceneCameraController::GetMouseMove() const
{
    auto systemUI = GetSubsystem<SystemUI>();
    return systemUI->GetRelativeMouseMove();
}

Vector3 SceneCameraController::GetMoveDirection() const
{
    static const ea::pair<Scancode, Vector3> keyMapping[]{
        {SCANCODE_W, Vector3::FORWARD},
        {SCANCODE_S, Vector3::BACK},
        {SCANCODE_A, Vector3::LEFT},
        {SCANCODE_D, Vector3::RIGHT},
        {SCANCODE_SPACE, Vector3::UP},
        {SCANCODE_LCTRL, Vector3::DOWN},
    };

    Vector3 moveDirection;
    for (const auto& [scancode, direction] : keyMapping)
    {
        if (ui::IsKeyDown(Input::GetKeyFromScancode(scancode)))
            moveDirection += direction;
    }
    return moveDirection.Normalized();
}

bool SceneCameraController::GetMoveAccelerated() const
{
    return ui::IsKeyDown(Input::GetKeyFromScancode(SCANCODE_LSHIFT));
}

SceneCameraController* SceneViewPage::GetCurrentCameraController() const
{
    return currentCameraController_ < cameraControllers_.size()
        ? cameraControllers_[currentCameraController_]
        : nullptr;
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

    HotkeyManager* hotkeyManager = project->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_RewindSimulation, &SceneViewTab::RewindSimulation);
    hotkeyManager->BindHotkey(this, Hotkey_TogglePaused, &SceneViewTab::ToggleSimulationPaused);
    hotkeyManager->BindHotkey(this, Hotkey_Cut, &SceneViewTab::CutSelection);
    hotkeyManager->BindHotkey(this, Hotkey_Copy, &SceneViewTab::CopySelection);
    hotkeyManager->BindHotkey(this, Hotkey_Paste, &SceneViewTab::PasteNextToSelection);
    hotkeyManager->BindHotkey(this, Hotkey_Delete, &SceneViewTab::DeleteSelection);
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

void SceneViewTab::RegisterCameraController(const SceneCameraControllerDesc& desc)
{
    cameraControllers_.push_back(desc);
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

    activePage->RewindSimulation();
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
            PushAction(MakeShared<CreateRemoveNodeAction>(newNode, false));
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
            PushAction(MakeShared<CreateRemoveComponentAction>(newComponent, false));
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
            PushAction(MakeShared<CreateRemoveNodeAction>(node, true));
            node->Remove();
        }
    }

    for (Component* component : selectedComponents)
    {
        if (component)
        {
            PushAction(MakeShared<CreateRemoveComponentAction>(component, true));
            component->Remove();
        }
    }

    activePage->selection_.Clear();
}

void SceneViewTab::RenderMenu()
{
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    if (ui::BeginMenu("Edit"))
    {
        RenderEditMenuItems();

        ui::Separator();

        if (ui::MenuItem("Cut", hotkeyManager->GetHotkeyLabel(Hotkey_Cut).c_str()))
            CutSelection();
        if (ui::MenuItem("Copy", hotkeyManager->GetHotkeyLabel(Hotkey_Copy).c_str()))
            CopySelection();
        if (ui::MenuItem("Paste", hotkeyManager->GetHotkeyLabel(Hotkey_Paste).c_str()))
            PasteNextToSelection();
        if (ui::MenuItem("Delete", hotkeyManager->GetHotkeyLabel(Hotkey_Delete).c_str()))
            DeleteSelection();

        ui::Separator();

        ui::EndMenu();
    }
}

bool SceneViewTab::CanOpenResource(const OpenResourceRequest& request)
{
    return request.xmlFile_ && request.xmlFile_->GetRoot("scene");
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
    // TODO(editor): Hide boilerplate with HotkeyManager
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    BaseClassName::RenderContextMenuItems();

    if (SceneViewPage* activePage = GetActivePage())
    {
        contextMenuSeparator_.Reset();

        const char* rewindTitle = ICON_FA_BACKWARD_FAST " Rewind Simulation";
        const char* rewindShortcut = hotkeyManager->GetHotkeyLabel(Hotkey_RewindSimulation).c_str();
        if (ui::MenuItem(rewindTitle, rewindShortcut, false, !!activePage->simulationBase_))
            RewindSimulation();

        const char* pauseTitle = !activePage->scene_->IsUpdateEnabled()
            ? (activePage->simulationBase_ ? ICON_FA_PLAY " Resume Simulation" : ICON_FA_PLAY " Start Simulation")
            : ICON_FA_PAUSE " Pause Simulation";
        if (ui::MenuItem(pauseTitle, hotkeyManager->GetHotkeyLabel(Hotkey_TogglePaused).c_str()))
            ToggleSimulationPaused();

        unsigned index = 0;
        for (const SceneCameraController* controller : activePage->cameraControllers_)
        {
            const bool selected = index == activePage->currentCameraController_;
            if (ui::MenuItem(controller->GetTitle().c_str(), nullptr, selected))
                activePage->currentCameraController_ = index;
            ++index;
        }
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

void SceneViewTab::OnActiveResourceChanged(const ea::string& resourceName)
{
    if (SceneViewPage* activePage = GetActivePage())
        activePage->scene_->SetUpdateEnabled(false);

    for (const auto& [name, data] : scenes_)
        data->renderer_->SetActive(name == resourceName);
}

void SceneViewTab::OnResourceSaved(const ea::string& resourceName)
{
    SceneViewPage* page = GetPage(resourceName);
    if (!page)
        return;

    SavePageConfig(*page);
    SavePageScene(*page);
}

void SceneViewTab::SavePageScene(SceneViewPage& page) const
{
    XMLFile xmlFile(context_);
    XMLElement rootElement = xmlFile.GetOrCreateRoot("scene");

    page.scene_->SetUpdateEnabled(false);
    page.scene_->SaveXML(rootElement);

    xmlFile.SaveFile(page.scene_->GetFileName());
}

void SceneViewTab::RenderContent()
{
    SceneViewPage* activePage = GetActivePage();
    if (!activePage)
        return;

    activePage->selection_.Update();

    activePage->renderer_->SetTextureSize(GetContentSize());
    activePage->renderer_->Update();

    const ImVec2 basePosition = ui::GetCursorPos();

    Texture2D* sceneTexture = activePage->renderer_->GetTexture();
    ui::SetCursorPos(basePosition);
    ui::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));

    const auto contentAreaMin = static_cast<Vector2>(ui::GetItemRectMin());
    const auto contentAreaMax = static_cast<Vector2>(ui::GetItemRectMax());
    activePage->contentArea_ = Rect{contentAreaMin, contentAreaMax};

    UpdateCameraController(*activePage);
    UpdateAddons(*activePage);
}

void SceneViewTab::UpdateCameraController(SceneViewPage& page)
{
    auto systemUI = GetSubsystem<SystemUI>();

    const auto cameraController = page.GetCurrentCameraController();
    const bool wasActive = isCameraControllerActive_;
    const bool isActive = cameraController && cameraController->IsActive(wasActive);
    if (isActive)
    {
        if (!wasActive)
            systemUI->SetRelativeMouseMove(true, true);
    }
    else if (wasActive)
    {
        systemUI->SetRelativeMouseMove(false, true);
    }
    cameraController->Update(isActive);

    isCameraControllerActive_ = isActive;
}

void SceneViewTab::UpdateAddons(SceneViewPage& page)
{
    bool mouseConsumed = isCameraControllerActive_;
    for (SceneViewAddon* addon : addonsByInputPriority_)
        addon->ProcessInput(page, mouseConsumed);

    for (SceneViewAddon* addon : addons_)
        addon->Render(page);
}

void SceneViewTab::UpdateFocused()
{

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

SharedPtr<SceneViewPage> SceneViewTab::CreatePage(Scene* scene, bool isActive) const
{
    auto page = MakeShared<SceneViewPage>();

    page->scene_ = scene;
    page->renderer_ = MakeShared<SceneRendererToTexture>(scene);
    page->cfgFileName_ = scene->GetFileName() + ".cfg";

    page->renderer_->SetActive(isActive);

    for (const SceneCameraControllerDesc& desc : cameraControllers_)
        page->cameraControllers_.push_back(desc.factory_(scene, page->renderer_->GetCamera()));

    LoadPageConfig(*page);
    return page;
}

void SceneViewTab::SavePageConfig(const SceneViewPage& page) const
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    jsonFile->SaveObject("Scene", page);
    jsonFile->SaveFile(page.cfgFileName_);
}

void SceneViewTab::LoadPageConfig(SceneViewPage& page) const
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    jsonFile->LoadFile(page.cfgFileName_);
    jsonFile->LoadObject("Scene", page);
}

RewindSceneActionWrapper::RewindSceneActionWrapper(SharedPtr<EditorAction> action, SceneViewPage* page)
    : BaseEditorActionWrapper(action)
    , page_(page)
{

}

bool RewindSceneActionWrapper::IsAlive() const
{
    return page_ && BaseEditorActionWrapper::IsAlive();
}

void RewindSceneActionWrapper::Redo() const
{
    page_->RewindSimulation();
    BaseEditorActionWrapper::Redo();
}

void RewindSceneActionWrapper::Undo() const
{
    page_->RewindSimulation();
    BaseEditorActionWrapper::Undo();
}

}
