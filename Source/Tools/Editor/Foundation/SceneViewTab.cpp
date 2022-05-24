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

bool SceneSelection::IsSelected(Node* node) const
{
    return nodes_.contains(WeakPtr<Node>(node));
}

void SceneSelection::Update()
{
    const unsigned numComponents = components_.size();
    const unsigned numNodes = nodes_.size();

    ea::erase_if(components_, [](Component* component) { return component == nullptr; });
    ea::erase_if(nodes_, [](Node* node) { return node == nullptr; });

    if (components_.size() != numComponents || nodes_.size() != numNodes)
    {
        UpdateRevision();
        UpdateEffectiveNodes();
    }
}

void SceneSelection::Clear()
{
    UpdateRevision();

    nodes_.clear();
    components_.clear();
    activeNode_ = nullptr;
}

void SceneSelection::ConvertToNodes()
{
    UpdateRevision();

    for (Component* component : components_)
    {
        if (Node* node = component->GetNode())
            SetSelected(node, true);
    }
    components_.clear();
}

void SceneSelection::SetSelected(Component* component, bool selected, bool activated)
{
    UpdateRevision();

    const WeakPtr<Component> weakComponent{component};

    if (selected)
    {
        if (activeNode_ == nullptr || activated)
        {
            if (const WeakPtr<Node> weakNode{component->GetNode()})
                activeNode_ = weakNode;
        }
        components_.insert(weakComponent);
    }
    else
        components_.erase(weakComponent);

    UpdateEffectiveNodes();
}

void SceneSelection::SetSelected(Node* node, bool selected, bool activated)
{
    UpdateRevision();

    const WeakPtr<Node> weakNode{node};

    if (selected)
    {
        if (activeNode_ == nullptr || activated)
            activeNode_ = weakNode;
        nodes_.insert(weakNode);
    }
    else
        nodes_.erase(weakNode);

    UpdateEffectiveNodes();
}

void SceneSelection::UpdateEffectiveNodes()
{
    effectiveNodes_.clear();

    for (Node* node : nodes_)
    {
        if (const WeakPtr<Node> weakNode{node})
            effectiveNodes_.insert(weakNode);
    }
    for (Component* component : components_)
    {
        if (component)
        {
            if (const WeakPtr<Node> weakNode{component->GetNode()})
                effectiveNodes_.insert(weakNode);
        }
    }

    if (!effectiveNodes_.contains(activeNode_))
        activeNode_ = !effectiveNodes_.empty() ? *effectiveNodes_.begin() : nullptr;
}

SceneCameraController* SceneViewPage::GetCurrentCameraController() const
{
    return currentCameraController_ < cameraControllers_.size()
        ? cameraControllers_[currentCameraController_]
        : nullptr;
}

SceneViewAddon::SceneViewAddon(SceneViewTab* owner)
    : Object(owner->GetContext())
    , owner_(owner)
{
}

SceneViewAddon::~SceneViewAddon()
{
}

SceneViewTab::SceneViewTab(Context* context)
    : ResourceEditorTab(context, "Scene View", "9f4f7432-dd60-4c83-aecd-2f6cf69d3549",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
{
    auto project = GetProject();
    project->IgnoreFileNamePattern("*.xml.cfg");
}

SceneViewTab::~SceneViewTab()
{
}

void SceneViewTab::RegisterAddon(const SharedPtr<SceneViewAddon>& addon)
{
    addons_.push_back(addon);
    addonsByInputPriority_.insert(addon);
    addonsByTitle_.insert(addon);
}

void SceneViewTab::RegisterCameraController(const SceneCameraControllerDesc& desc)
{
    cameraControllers_.push_back(desc);
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

void SceneViewTab::UpdateAndRenderContextMenuItems()
{
    BaseClassName::UpdateAndRenderContextMenuItems();

    if (SceneViewPage* activePage = GetActivePage())
    {
        ResetSeparator();

        unsigned index = 0;
        for (const SceneCameraController* controller : activePage->cameraControllers_)
        {
            const bool selected = index == activePage->currentCameraController_;
            if (ui::MenuItem(controller->GetTitle().c_str(), nullptr, selected))
                activePage->currentCameraController_ = index;
            ++index;
        }
    }

    SetSeparator();

    for (SceneViewAddon* addon : addonsByTitle_)
    {
        if (addon->NeedTabContextMenu())
        {
            ResetSeparator();
            if (ui::BeginMenu(addon->GetContextMenuTitle().c_str()))
            {
                addon->RenderTabContextMenu();
                ui::EndMenu();
            }
        }
    }

    SetSeparator();
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

    scenes_[resourceName] = CreatePage(scene);
}

void SceneViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    scenes_.erase(resourceName);
}

void SceneViewTab::OnActiveResourceChanged(const ea::string& resourceName)
{
    for (const auto& [name, data] : scenes_)
        data.renderer_->SetActive(name == resourceName);
}

void SceneViewTab::OnResourceSaved(const ea::string& resourceName)
{
    SceneViewPage* page = GetPage(resourceName);
    if (!page)
        return;

    SavePageConfig(*page);
}

void SceneViewTab::UpdateAndRenderContent()
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
        addon->UpdateAndRender(page);
}

void SceneViewTab::UpdateFocused()
{

}

IntVector2 SceneViewTab::GetContentSize() const
{
    const ImGuiContext& g = *GImGui;
    const ImGuiWindow* window = g.CurrentWindow;
    const ImRect rect = ImRound(window->ContentRegionRect);
    return {RoundToInt(rect.GetWidth()), RoundToInt(rect.GetHeight())};
}

SceneViewPage* SceneViewTab::GetPage(const ea::string& resourceName)
{
    auto iter = scenes_.find(resourceName);
    return iter != scenes_.end() ? &iter->second : nullptr;
}

SceneViewPage* SceneViewTab::GetActivePage()
{
    return GetPage(GetActiveResourceName());
}

SceneViewPage SceneViewTab::CreatePage(Scene* scene) const
{
    SceneViewPage page;

    page.scene_ = scene;
    page.renderer_ = MakeShared<SceneRendererToTexture>(scene);
    page.cfgFileName_ = scene->GetFileName() + ".cfg";

    for (const SceneCameraControllerDesc& desc : cameraControllers_)
        page.cameraControllers_.push_back(desc.factory_(scene, page.renderer_->GetCamera()));

    LoadPageConfig(page);
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

}
