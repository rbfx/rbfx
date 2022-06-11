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

#include <Urho3D/Core/Timer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/SystemUI/SystemUI.h>

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

void EditorCamera::Settings::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "MouseSensitivity", mouseSensitivity_);
    SerializeOptionalValue(archive, "MinSpeed", minSpeed_);
    SerializeOptionalValue(archive, "MaxSpeed", maxSpeed_);
    SerializeOptionalValue(archive, "Acceleration", acceleration_);
    SerializeOptionalValue(archive, "ShiftFactor", shiftFactor_);
}

void EditorCamera::Settings::RenderSettings()
{
    ui::DragFloat("Mouse Sensitivity", &mouseSensitivity_, 0.01f, 0.0f, 1.0f, "%.2f");
    ui::DragFloat("Min Speed", &minSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Max Speed", &maxSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Acceleration", &acceleration_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Shift Factor", &shiftFactor_, 0.5f, 1.0f, 10.0f, "%.1f");
}

EditorCamera::EditorCamera(SceneViewTab* owner, SettingsPage* settings)
    : SceneViewAddon(owner)
    , settings_(settings)
{
}

Vector2 EditorCamera::GetMouseMove() const
{
    auto systemUI = GetSubsystem<SystemUI>();
    return systemUI->GetRelativeMouseMove();
}

Vector3 EditorCamera::GetMoveDirection() const
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

bool EditorCamera::GetMoveAccelerated() const
{
    return ui::IsKeyDown(Input::GetKeyFromScancode(SCANCODE_LSHIFT));
}

void EditorCamera::ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed)
{
    auto systemUI = GetSubsystem<SystemUI>();

    const bool wasActive = isActive_;
    isActive_ = (wasActive || ui::IsItemHovered()) && ui::IsMouseDown(MOUSEB_RIGHT);
    if (isActive_)
    {
        if (!wasActive)
            systemUI->SetRelativeMouseMove(true, true);
    }
    else if (wasActive)
    {
        systemUI->SetRelativeMouseMove(false, true);
    }

    PageState& state = GetOrInitializeState(scenePage);
    UpdateState(scenePage, state);
}

EditorCamera::PageState& EditorCamera::GetOrInitializeState(SceneViewPage& scenePage) const
{
    ea::any& stateWrapped = scenePage.GetAddonData(*this);
    if (!stateWrapped.has_value())
    {
        PageState state;
        state.LookAt(Vector3{0.0f, 5.0f, -10.0f}, Vector3::ZERO);
        stateWrapped = state;
    }
    return ea::any_cast<PageState&>(stateWrapped);
}

void EditorCamera::UpdateState(SceneViewPage& scenePage, PageState& state)
{
    const Settings& cfg = settings_->GetValues();

    Camera* camera = scenePage.renderer_->GetCamera();
    Node* node = camera->GetNode();

    // Restore camera to previous step if moved
    if (state.lastCameraPosition_ != node->GetPosition())
        node->SetPosition(state.lastCameraPosition_);
    if (state.lastCameraRotation_ != node->GetRotation())
        node->SetRotation(state.lastCameraRotation_);

    if (isActive_)
    {
        // Apply mouse movement
        const Vector2 mouseMove = GetMouseMove() * cfg.mouseSensitivity_;
        state.yaw_ = Mod(state.yaw_ + mouseMove.x_, 360.0f);
        state.pitch_ = Clamp(state.pitch_ + mouseMove.y_, -89.0f, 89.0f);

        node->SetRotation(Quaternion{state.pitch_, state.yaw_, 0.0f});
        state.lastCameraRotation_ = node->GetRotation();

        // Apply camera movement
        const float timeStep = GetSubsystem<Time>()->GetTimeStep();
        const Vector3 moveDirection = GetMoveDirection();
        const float multiplier = GetMoveAccelerated() ? cfg.shiftFactor_ : 1.0f;
        if (moveDirection == Vector3::ZERO)
            state.currentMoveSpeed_ = cfg.minSpeed_;

        node->Translate(moveDirection * state.currentMoveSpeed_ * multiplier * timeStep);
        state.lastCameraPosition_ = node->GetPosition();

        // Apply acceleration
        state.currentMoveSpeed_ = ea::min(cfg.maxSpeed_, state.currentMoveSpeed_ + cfg.acceleration_ * timeStep);
    }
    else
    {
        state.currentMoveSpeed_ = cfg.minSpeed_;
    }
}

}
