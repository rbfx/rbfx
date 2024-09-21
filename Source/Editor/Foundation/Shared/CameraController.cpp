//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../../Foundation/Shared/CameraController.h"

#include <Urho3D/Graphics/Camera.h>

namespace Urho3D
{
namespace
{

const auto Hotkey_MoveForward = EditorHotkey{"EditorCamera.MoveForward"}.Hold(SCANCODE_W).Hold(MOUSEB_RIGHT).MaybeShift();
const auto Hotkey_MoveBackward = EditorHotkey{"EditorCamera.MoveBackward"}.Hold(SCANCODE_S).Hold(MOUSEB_RIGHT).MaybeShift();
const auto Hotkey_MoveLeft = EditorHotkey{"EditorCamera.MoveLeft"}.Hold(SCANCODE_A).Hold(MOUSEB_RIGHT).MaybeShift();
const auto Hotkey_MoveRight = EditorHotkey{"EditorCamera.MoveRight"}.Hold(SCANCODE_D).Hold(MOUSEB_RIGHT).MaybeShift();
const auto Hotkey_MoveUp = EditorHotkey{"EditorCamera.MoveUp"}.Hold(SCANCODE_E).Hold(MOUSEB_RIGHT).MaybeShift();
const auto Hotkey_MoveDown = EditorHotkey{"EditorCamera.MoveDown"}.Hold(SCANCODE_Q).Hold(MOUSEB_RIGHT).MaybeShift();

const auto Hotkey_MoveAccelerate = EditorHotkey{"EditorCamera.MoveAccelerate"}.Hold(SCANCODE_LSHIFT).Hold(MOUSEB_RIGHT).MaybeShift();
const auto Hotkey_LookAround = EditorHotkey{"EditorCamera.LookAround"}.Hold(MOUSEB_RIGHT).MaybeShift().MaybeAlt().MaybeCtrl();
const auto Hotkey_OrbitAround = EditorHotkey{"EditorCamera.OrbitAround"}.Alt().Hold(MOUSEB_RIGHT).MaybeShift();

}


void CameraController::Settings::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "MouseSensitivity", mouseSensitivity_, Settings{}.mouseSensitivity_);
    SerializeOptionalValue(archive, "MinSpeed", minSpeed_, Settings{}.minSpeed_);
    SerializeOptionalValue(archive, "MaxSpeed", maxSpeed_, Settings{}.maxSpeed_);
    SerializeOptionalValue(archive, "ScrollSpeed", scrollSpeed_, Settings{}.scrollSpeed_);
    SerializeOptionalValue(archive, "Acceleration", acceleration_, Settings{}.acceleration_);
    SerializeOptionalValue(archive, "ShiftFactor", shiftFactor_, Settings{}.shiftFactor_);
    SerializeOptionalValue(archive, "FocusDistance", focusDistance_, Settings{}.focusDistance_);
    SerializeOptionalValue(archive, "Orthographic", orthographic_, Settings{}.orthographic_);
    SerializeOptionalValue(archive, "OrthographicSize", orthoSize_, Settings{}.orthoSize_);
}

void CameraController::Settings::RenderSettings()
{
    ui::DragFloat("Mouse Sensitivity", &mouseSensitivity_, 0.01f, 0.0f, 1.0f, "%.2f");
    ui::DragFloat("Min Speed", &minSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Max Speed", &maxSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Scroll Speed", &scrollSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Acceleration", &acceleration_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Shift Factor", &shiftFactor_, 0.5f, 1.0f, 10.0f, "%.1f");
    ui::DragFloat("Focus Distance", &focusDistance_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::Checkbox("Orthographic", &orthographic_);
    ui::InputFloat("Orthographic Size", &orthoSize_, 0.1f, 1.0f, "%.1f");
}

CameraController::PageState::PageState()
{
    LookAt(Vector3{0.0f, 5.0f, -10.0f}, Vector3::ZERO);
}

void CameraController::PageState::LookAt(const BoundingBox& box)
{
    auto center = box.Center();
    auto pos = center + box.Size().Length() * Vector3::ONE;
    LookAt(pos, center);
}

void CameraController::PageState::LookAt(const Vector3& position, const Vector3& target)
{
    lastCameraPosition_ = position;
    lastCameraRotation_.FromLookRotation(target - position, Vector3::UP);
    yaw_ = lastCameraRotation_.YawAngle();
    pitch_ = lastCameraRotation_.PitchAngle();
}

void CameraController::PageState::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Position", lastCameraPosition_);
    SerializeOptionalValue(archive, "Rotation", lastCameraRotation_);

    if (archive.IsInput())
    {
        yaw_ = lastCameraRotation_.YawAngle();
        pitch_ = lastCameraRotation_.PitchAngle();
    }
}

CameraController::CameraController(Context* context, HotkeyManager* hotkeyManager)
    : Object(context)
    , hotkeyManager_(hotkeyManager)
{
    hotkeyManager->BindPassiveHotkey(Hotkey_MoveForward);
    hotkeyManager->BindPassiveHotkey(Hotkey_MoveBackward);
    hotkeyManager->BindPassiveHotkey(Hotkey_MoveLeft);
    hotkeyManager->BindPassiveHotkey(Hotkey_MoveRight);
    hotkeyManager->BindPassiveHotkey(Hotkey_MoveUp);
    hotkeyManager->BindPassiveHotkey(Hotkey_MoveDown);

    hotkeyManager->BindPassiveHotkey(Hotkey_MoveAccelerate);
    hotkeyManager->BindPassiveHotkey(Hotkey_LookAround);
    hotkeyManager->BindPassiveHotkey(Hotkey_OrbitAround);
}

Vector2 CameraController::GetMouseMove() const
{
    const auto systemUI = GetSubsystem<SystemUI>();
    return systemUI->GetRelativeMouseMove();
}

Vector3 CameraController::GetMoveDirection() const
{
    const ea::pair<const EditorHotkey&, Vector3> keyMapping[]{
        {Hotkey_MoveForward, Vector3::FORWARD},
        {Hotkey_MoveBackward, Vector3::BACK},
        {Hotkey_MoveLeft, Vector3::LEFT},
        {Hotkey_MoveRight, Vector3::RIGHT},
        {Hotkey_MoveUp, Vector3::UP},
        {Hotkey_MoveDown, Vector3::DOWN},
    };

    Vector3 moveDirection;
    for (const auto& [hotkey, direction] : keyMapping)
    {
        if (hotkeyManager_->IsHotkeyActive(hotkey))
            moveDirection += direction;
    }
    return moveDirection.Normalized();
}

void CameraController::ProcessInput(Camera* camera, PageState& state, const Settings* settings)
{
    if (!settings)
    {
        auto settingsManager = GetSubsystem<Project>()->GetSettingsManager();
        auto settingsPage = dynamic_cast<SettingsPage*>(settingsManager->FindPage("Editor.Scene:Camera"));
        if (settingsPage)
        {
            settings = &settingsPage->GetValues();
        }
    }
    if (!settings)
    {
        return;
    }
    camera->SetOrthographic(settings->orthographic_);
    if (settings->orthographic_)
    {
        const float aspectRatio = camera->GetAspectRatio();
        camera->SetOrthoSize(settings->orthoSize_);
        camera->SetAspectRatioInternal(aspectRatio);
    }

    const auto systemUI = GetSubsystem<SystemUI>();

    const bool wasActive = isActive_;
    isActive_ = (wasActive || ui::IsItemHovered()) && hotkeyManager_->IsHotkeyActive(Hotkey_LookAround);
    if (isActive_)
    {
        if (!wasActive)
            systemUI->SetRelativeMouseMove(true, true);
    }
    else if (wasActive)
    {
        systemUI->SetRelativeMouseMove(false, true);
    }

    UpdateState(*settings, camera, state);
}


void CameraController::UpdateState(const Settings& cfg, const Camera* camera, PageState& state) const
{
    auto time = GetSubsystem<Time>();

    Node* node = camera->GetNode();

    // Restore camera to previous step if moved
    if (state.lastCameraPosition_ != node->GetPosition())
        node->SetPosition(state.lastCameraPosition_);
    if (state.lastCameraRotation_ != node->GetRotation())
        node->SetRotation(state.lastCameraRotation_);

    const bool isAccelerated = hotkeyManager_->IsHotkeyActive(Hotkey_MoveAccelerate);
    const bool isOrbiting = hotkeyManager_->IsHotkeyActive(Hotkey_OrbitAround);
    if (isActive_ && !isOrbiting)
    {
        // Apply mouse movement
        const Vector2 mouseMove = GetMouseMove() * cfg.mouseSensitivity_;
        state.yaw_ = Mod(state.yaw_ + mouseMove.x_, 360.0f);
        state.pitch_ = Clamp(state.pitch_ + mouseMove.y_, -89.0f, 89.0f);

        node->SetRotation(Quaternion{state.pitch_, state.yaw_, 0.0f});

        // Apply camera movement
        const float timeStep = GetSubsystem<Time>()->GetTimeStep();
        const Vector3 moveDirection = GetMoveDirection();
        const float multiplier = isAccelerated ? cfg.shiftFactor_ : 1.0f;
        if (moveDirection == Vector3::ZERO)
            state.currentMoveSpeed_ = cfg.minSpeed_;

        node->Translate(moveDirection * state.currentMoveSpeed_ * multiplier * timeStep);

        // Apply acceleration
        state.currentMoveSpeed_ = ea::min(cfg.maxSpeed_, state.currentMoveSpeed_ + cfg.acceleration_ * timeStep);
    }
    else
    {
        state.currentMoveSpeed_ = cfg.minSpeed_;
    }

    if (isOrbiting)
    {
        if (!state.orbitPosition_)
            state.orbitPosition_ = node->GetPosition() + node->GetRotation() * Vector3{0.0f, 0.0f, cfg.focusDistance_};

        const Vector2 mouseMove = GetMouseMove() * cfg.mouseSensitivity_;
        state.yaw_ = Mod(state.yaw_ + mouseMove.x_, 360.0f);
        state.pitch_ = Clamp(state.pitch_ + mouseMove.y_, -89.0f, 89.0f);

        node->SetRotation(Quaternion{state.pitch_, state.yaw_, 0.0f});
        state.lastCameraRotation_ = node->GetRotation();

        node->SetPosition(*state.orbitPosition_ - node->GetRotation() * Vector3{0.0f, 0.0f, cfg.focusDistance_});
        state.lastCameraPosition_ = node->GetPosition();
    }
    else
    {
        state.orbitPosition_ = ea::nullopt;
    }

    if (ui::IsItemHovered() && Abs(ui::GetMouseWheel()) > 0.05f)
    {
        state.pendingOffset_ += node->GetWorldDirection() * cfg.scrollSpeed_ * Sign(ui::GetMouseWheel());
    }

    if (state.pendingOffset_.Length() > 0.05f)
    {
        const float factor = InverseExponentialDecay(cfg.focusSpeed_ * time->GetTimeStep());
        node->Translate(state.pendingOffset_ * factor, TS_WORLD);
        state.pendingOffset_ *= 1.0f - factor;
    }

    state.lastCameraRotation_ = node->GetRotation();
    state.lastCameraPosition_ = node->GetPosition();
}

} // namespace Urho3D
