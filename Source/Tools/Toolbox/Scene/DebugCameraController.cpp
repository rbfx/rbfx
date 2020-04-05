//
// Copyright (c) 2017 the Urho3D project.
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
#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Scene.h>
#include "DebugCameraController.h"


namespace Urho3D
{

DebugCameraController::DebugCameraController(Context* context)
    : LogicComponent(context)
{
}

void DebugCameraController::Update(float timeStep)
{
    // Do not move if the UI has a focused element
    if (context_->GetUI()->GetFocusElement())
        return;

    // Do not move if interacting with UI controls
    if (context_->GetSystemUI()->IsAnyItemActive())
        return;

    Input* input = context_->GetInput();

    // Movement speed as world units per second
    if (input->GetKeyPress(KEY_KP_PLUS))
        speed_ += 1.f;
    else if (input->GetKeyPress(KEY_KP_MINUS))
        speed_ -= 1.f;

    if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        IntVector2 delta = input->GetMouseMove();
        if (input->IsMouseVisible() && delta != IntVector2::ZERO)
            input->SetMouseVisible(false);

        if (!input->IsMouseVisible())
        {
            RunFrame(timeStep);
            ui::SetMouseCursor(ImGuiMouseCursor_None);  // Hint for SceneView that mouse was hidden on this frame
        }
    }
    else if (!input->IsMouseVisible())
    {
        input->SetMouseVisible(true);
        ui::SetMouseCursor(ImGuiMouseCursor_None);  // Hint for SceneView that mouse was hidden on this frame
    }
}

DebugCameraController3D::DebugCameraController3D(Context* context)
    : DebugCameraController(context)
{
}

void DebugCameraController3D::RunFrame(float timeStep)
{
    Input* input = context_->GetInput();
    IntVector2 delta = input->GetMouseMove();
    float moveSpeed_ = speed_;
    if (input->GetKeyDown(KEY_SHIFT))
        moveSpeed_ *= 2;

    if (isRotationCenterValid_ && input->GetQualifierDown(QUAL_ALT))
    {
#if URHO3D_SYSTEMUI
        // The following LOC is use to make SystemUI be not outlined
        // refer to: https://github.com/rokups/rbfx/pull/151#issuecomment-562489285
        ui::GetCurrentContext()->NavWindowingToggleLayer = false;
#endif
        auto yaw = GetNode()->GetRotation().EulerAngles().x_;
        GetNode()->RotateAround(rotationCenter_, Quaternion(mouseSensitivity_ * delta.x_, Vector3::UP), TS_WORLD);
        auto angle = mouseSensitivity_ * delta.y_;
        if (yaw + angle > 89.f)
        {
            angle = 89.f - yaw;
        }
        else if (yaw + angle < -89.f)
        {
            angle = -89.f - yaw;
        }
        GetNode()->RotateAround(rotationCenter_, Quaternion(angle, GetNode()->GetRight()), TS_WORLD);
        GetNode()->LookAt(rotationCenter_);
    }
    else
    {
        auto yaw = GetNode()->GetRotation().EulerAngles().x_;
        if ((yaw > -90.f && yaw < 90.f) || (yaw <= -90.f && delta.y_ > 0) || (yaw >= 90.f && delta.y_ < 0))
            GetNode()->RotateAround(Vector3::ZERO, Quaternion(mouseSensitivity_ * delta.y_, Vector3::RIGHT), TS_LOCAL);
        GetNode()->RotateAround(GetNode()->GetPosition(), Quaternion(mouseSensitivity_ * delta.x_, Vector3::UP), TS_WORLD);
    }

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        GetNode()->Translate(Vector3::FORWARD * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_S))
        GetNode()->Translate(Vector3::BACK * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_A))
        GetNode()->Translate(Vector3::LEFT * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_D))
        GetNode()->Translate(Vector3::RIGHT * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_E))
        GetNode()->Translate(Vector3::UP * moveSpeed_ * timeStep, TS_WORLD);
    if (input->GetKeyDown(KEY_Q))
        GetNode()->Translate(Vector3::DOWN * moveSpeed_ * timeStep, TS_WORLD);
}

void DebugCameraController3D::SetRotationCenter(const Vector3& center)
{
    rotationCenter_ = center;
    isRotationCenterValid_ = true;
}

DebugCameraController2D::DebugCameraController2D(Context* context)
    : DebugCameraController(context)
{
    SetUpdateEventMask(USE_NO_EVENT);
}

void DebugCameraController2D::RunFrame(float timeStep)
{
    Input* input = context_->GetInput();
    IntVector2 delta = input->GetMouseMove();
    // Movement speed as world units per second
    float moveSpeed_ = speed_;
    if (input->GetKeyDown(KEY_SHIFT))
        moveSpeed_ *= 2;

    Node* node = GetNode();
    node->Translate2D(Vector2{static_cast<float>(-delta.x_), static_cast<float>(delta.y_)} * moveSpeed_ * timeStep);
    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        node->Translate(Vector3::UP * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_S))
        node->Translate(Vector3::DOWN * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_A))
        node->Translate(Vector3::LEFT * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_D))
        node->Translate(Vector3::RIGHT * moveSpeed_ * timeStep);
}

}
