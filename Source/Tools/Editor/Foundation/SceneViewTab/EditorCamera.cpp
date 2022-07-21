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
    SerializeOptionalValue(archive, "MouseSensitivity", mouseSensitivity_, Settings{}.mouseSensitivity_);
    SerializeOptionalValue(archive, "MinSpeed", minSpeed_, Settings{}.minSpeed_);
    SerializeOptionalValue(archive, "MaxSpeed", maxSpeed_, Settings{}.maxSpeed_);
    SerializeOptionalValue(archive, "Acceleration", acceleration_, Settings{}.acceleration_);
    SerializeOptionalValue(archive, "ShiftFactor", shiftFactor_, Settings{}.shiftFactor_);
    SerializeOptionalValue(archive, "FocusDistance", focusDistance_, Settings{}.focusDistance_);
}

void EditorCamera::Settings::RenderSettings()
{
    ui::DragFloat("Mouse Sensitivity", &mouseSensitivity_, 0.01f, 0.0f, 1.0f, "%.2f");
    ui::DragFloat("Min Speed", &minSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Max Speed", &maxSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Acceleration", &acceleration_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Shift Factor", &shiftFactor_, 0.5f, 1.0f, 10.0f, "%.1f");
    ui::DragFloat("Focus Distance", &focusDistance_, 0.1f, 0.1f, 100.0f, "%.1f");
}

EditorCamera::PageState::PageState()
{
    LookAt(Vector3{0.0f, 5.0f, -10.0f}, Vector3::ZERO);
}

void EditorCamera::PageState::LookAt(const Vector3& position, const Vector3& target)
{
    lastCameraPosition_ = position;
    lastCameraRotation_ = Quaternion{Vector3::FORWARD, target - position};
    yaw_ = lastCameraRotation_.YawAngle();
    pitch_ = lastCameraRotation_.PitchAngle();
}

void EditorCamera::PageState::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Position", lastCameraPosition_);
    SerializeOptionalValue(archive, "Rotation", lastCameraRotation_);

    if (archive.IsInput())
    {
        yaw_ = lastCameraRotation_.YawAngle();
        pitch_ = lastCameraRotation_.PitchAngle();
    }
}

EditorCamera::EditorCamera(SceneViewTab* owner, SettingsPage* settings)
    : SceneViewAddon(owner)
    , settings_(settings)
{
    owner_->OnLookAt.Subscribe(this, &EditorCamera::LookAtPosition);

    auto hotkeyManager = owner_->GetHotkeyManager();
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

Vector2 EditorCamera::GetMouseMove() const
{
    auto systemUI = GetSubsystem<SystemUI>();
    return systemUI->GetRelativeMouseMove();
}

Vector3 EditorCamera::GetMoveDirection() const
{
    auto hotkeyManager = owner_->GetHotkeyManager();

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
        if (hotkeyManager->IsHotkeyActive(hotkey))
            moveDirection += direction;
    }
    return moveDirection.Normalized();
}

void EditorCamera::ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed)
{
    auto systemUI = GetSubsystem<SystemUI>();
    auto hotkeyManager = owner_->GetHotkeyManager();

    const bool wasActive = isActive_;
    isActive_ = (wasActive || ui::IsItemHovered()) && hotkeyManager->IsHotkeyActive(Hotkey_LookAround);
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

void EditorCamera::SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const
{
    if (!stateWrapped.has_value())
        stateWrapped = PageState{};
    SerializeValue(archive, name, ea::any_cast<PageState&>(stateWrapped));
}

EditorCamera::PageState& EditorCamera::GetOrInitializeState(SceneViewPage& scenePage) const
{
    ea::any& stateWrapped = scenePage.GetAddonData(*this);
    if (!stateWrapped.has_value())
        stateWrapped = PageState{};
    return ea::any_cast<PageState&>(stateWrapped);
}

void EditorCamera::UpdateState(SceneViewPage& scenePage, PageState& state) const
{
    auto hotkeyManager = owner_->GetHotkeyManager();
    auto time = GetSubsystem<Time>();
    const Settings& cfg = settings_->GetValues();

    Camera* camera = scenePage.renderer_->GetCamera();
    Node* node = camera->GetNode();

    // Restore camera to previous step if moved
    if (state.lastCameraPosition_ != node->GetPosition())
        node->SetPosition(state.lastCameraPosition_);
    if (state.lastCameraRotation_ != node->GetRotation())
        node->SetRotation(state.lastCameraRotation_);

    const bool isAccelerated = hotkeyManager->IsHotkeyActive(Hotkey_MoveAccelerate);
    const bool isOrbiting = hotkeyManager->IsHotkeyActive(Hotkey_OrbitAround);
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
        else
            state.pendingOffset_ = Vector3::ZERO;

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

    if (state.pendingOffset_.Length() > 0.05f)
    {
        const float factor = ExpSmoothing(cfg.focusSpeed_, time->GetTimeStep());
        node->Translate(state.pendingOffset_ * factor, TS_WORLD);
        state.pendingOffset_ *= 1.0f - factor;
    }

    state.lastCameraRotation_ = node->GetRotation();
    state.lastCameraPosition_ = node->GetPosition();
}

void EditorCamera::LookAtPosition(SceneViewPage& scenePage, const Vector3& position) const
{
    PageState& state = GetOrInitializeState(scenePage);
    const Settings& cfg = settings_->GetValues();

    Camera* camera = scenePage.renderer_->GetCamera();
    Node* node = camera->GetNode();

    const Vector3 newPosition = position - node->GetRotation() * Vector3{0.0f, 0.0f, cfg.focusDistance_};
    state.pendingOffset_ += newPosition - state.lastCameraPosition_;
}

}
