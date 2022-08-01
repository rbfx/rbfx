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

#include "../../Foundation/SceneViewTab/EditorCamera.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

void Foundation_EditorCamera(Context* context, SceneViewTab* sceneViewTab)
{
    auto project = sceneViewTab->GetProject();
    auto settingsManager = project->GetSettingsManager();

    auto settingsPage = MakeShared<EditorCamera::SettingsPage>(context);
    settingsManager->AddPage(settingsPage);

    sceneViewTab->RegisterAddon<EditorCamera>(settingsPage);
}


EditorCamera::EditorCamera(SceneViewTab* owner, SettingsPage* settings)
    : SceneViewAddon(owner)
    , cameraController_(MakeShared<CameraController>(owner->GetContext(), owner->GetHotkeyManager()))
    , settings_(settings)
{
    owner_->OnLookAt.Subscribe(this, &EditorCamera::LookAtPosition);
}

void EditorCamera::ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed)
{
    auto& state = GetOrInitializeState(scenePage);
    Camera* camera = scenePage.renderer_->GetCamera();
    const auto& cfg = settings_->GetValues();

    cameraController_->ProcessInput(camera, state, &cfg);
}

void EditorCamera::SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const
{
    if (!stateWrapped.has_value())
        stateWrapped = CameraController::PageState{};
    SerializeValue(archive, name, ea::any_cast<CameraController::PageState&>(stateWrapped));
}

CameraController::PageState& EditorCamera::GetOrInitializeState(SceneViewPage& scenePage) const
{
    ea::any& stateWrapped = scenePage.GetAddonData(*this);
    if (!stateWrapped.has_value())
        stateWrapped = CameraController::PageState{};
    return ea::any_cast<CameraController::PageState&>(stateWrapped);
}

void EditorCamera::LookAtPosition(SceneViewPage& scenePage, const Vector3& position) const
{
    auto& state = GetOrInitializeState(scenePage);
    Camera* camera = scenePage.renderer_->GetCamera();
    const auto& cfg = settings_->GetValues();
    Node* node = camera->GetNode();

    const Vector3 newPosition = position - node->GetRotation() * Vector3{0.0f, 0.0f, cfg.focusDistance_};
    state.pendingOffset_ += newPosition - state.lastCameraPosition_;
}

}
