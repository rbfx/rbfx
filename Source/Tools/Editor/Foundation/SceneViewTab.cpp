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

namespace
{

void SerializeValue(Archive& archive, const char* name, SceneViewPage& page)
{
    auto block = archive.OpenUnorderedBlock(name);

    SerializeOptionalValue(archive, "CurrentCameraController", page.currentCameraController_, AlwaysSerialize{});

    for (SceneCameraController* controller : page.cameraControllers_)
        SerializeOptionalValue(archive, controller->GetTypeName().c_str(), *controller, AlwaysSerialize{});
}

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

void SceneViewTab::RegisterCameraController(const SceneCameraControllerDesc& desc)
{
    cameraControllers_.push_back(desc);
}

bool SceneViewTab::CanOpenResource(const OpenResourceRequest& request)
{
    return request.xmlFile_ && request.xmlFile_->GetRoot("scene");
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

}

void SceneViewTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    BaseClassName::ApplyHotkeys(hotkeyManager);
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

    activePage->renderer_->SetTextureSize(GetContentSize());
    activePage->renderer_->Update();

    Texture2D* sceneTexture = activePage->renderer_->GetTexture();
    ui::SetCursorPos({0, 0});
    ui::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));

    UpdateCameraController(*activePage);
}

void SceneViewTab::UpdateCameraController(SceneViewPage& page)
{
    auto systemUI = GetSubsystem<SystemUI>();

    const auto cameraController = page.GetCurrentCameraController();
    const bool isActive = cameraController && cameraController->IsActive(wasCameraControllerActive_);
    if (isActive)
    {
        if (!wasCameraControllerActive_)
            systemUI->SetRelativeMouseMove(true, true);
    }
    else if (wasCameraControllerActive_)
    {
        systemUI->SetRelativeMouseMove(false, true);
    }
    cameraController->Update(isActive);

    wasCameraControllerActive_ = isActive;
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
