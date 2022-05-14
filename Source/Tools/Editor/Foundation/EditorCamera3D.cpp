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

#include "../Foundation/EditorCamera3D.h"

#include <Urho3D/Core/Timer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/SystemUI/SystemUI.h>

namespace Urho3D
{

void Foundation_EditorCamera3D(Context* context, SceneViewTab* sceneViewTab)
{
    auto project = sceneViewTab->GetProject();
    auto settingsManager = project->GetSettingsManager();

    auto settingsPage = MakeShared<EditorCamera3DSettings>(context);
    settingsManager->AddPage(settingsPage);

    sceneViewTab->RegisterCameraController<EditorCamera3D>(settingsPage);
}

void EditorCamera3DSettings::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "MouseSensitivity", mouseSensitivity_);
    SerializeOptionalValue(archive, "MinSpeed", minSpeed_);
    SerializeOptionalValue(archive, "MaxSpeed", maxSpeed_);
    SerializeOptionalValue(archive, "Acceleration", acceleration_);
    SerializeOptionalValue(archive, "ShiftFactor", shiftFactor_);
}

void EditorCamera3DSettings::RenderSettings()
{
    ui::DragFloat("Mouse Sensitivity", &mouseSensitivity_, 0.01f, 0.0f, 1.0f, "%.2f");
    ui::DragFloat("Mouse Sensitivity", &minSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Mouse Sensitivity", &maxSpeed_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Mouse Sensitivity", &acceleration_, 0.1f, 0.1f, 100.0f, "%.1f");
    ui::DragFloat("Mouse Sensitivity", &shiftFactor_, 0.5f, 1.0f, 10.0f, "%.1f");
}

EditorCamera3D::EditorCamera3D(Scene* scene, Camera* camera, EditorCamera3DSettings* settings)
    : SceneCameraController(scene, camera)
    , settings_(settings)
{
    Reset(Vector3{0.0f, 5.0f, -10.0f}, Vector3::ZERO);
}

void EditorCamera3D::Reset(const Vector3& position, const Vector3& lookAt)
{
    lastCameraPosition_ = position;
    lastCameraRotation_ = Quaternion{Vector3::FORWARD, lookAt - position};
    yaw_ = lastCameraRotation_.YawAngle();
    pitch_ = lastCameraRotation_.PitchAngle();
}

bool EditorCamera3D::IsActive(bool wasActive)
{
    return (wasActive || ui::IsItemHovered()) && ui::IsMouseDown(MOUSEB_RIGHT);
}

void EditorCamera3D::Update(bool isActive)
{
    if (!camera_ || !settings_)
        return;

    Node* node = camera_->GetNode();

    // Restore camera to previous step if moved
    if (lastCameraPosition_ != node->GetPosition())
        node->SetPosition(lastCameraPosition_);
    if (lastCameraRotation_ != node->GetRotation())
        node->SetRotation(lastCameraRotation_);

    if (isActive)
    {
        // Apply mouse movement
        const Vector2 mouseMove = GetMouseMove() * settings_->mouseSensitivity_;
        yaw_ = Mod(yaw_ + mouseMove.x_, 360.0f);
        pitch_ = Clamp(pitch_ + mouseMove.y_, -89.0f, 89.0f);

        node->SetRotation(Quaternion{pitch_, yaw_, 0.0f});
        lastCameraRotation_ = node->GetRotation();

        // Apply camera movement
        const float timeStep = GetSubsystem<Time>()->GetTimeStep();
        const Vector3 moveDirection = GetMoveDirection();
        const float multiplier = GetMoveAccelerated() ? settings_->shiftFactor_ : 1.0f;
        if (moveDirection == Vector3::ZERO)
            currentMoveSpeed_ = settings_->minSpeed_;

        node->Translate(moveDirection * currentMoveSpeed_ * multiplier * timeStep);
        lastCameraPosition_ = node->GetPosition();

        // Apply acceleration
        currentMoveSpeed_ = ea::min(settings_->maxSpeed_, currentMoveSpeed_ + settings_->acceleration_ * timeStep);
    }
    else
    {
        currentMoveSpeed_ = settings_->minSpeed_;
    }
}

}
