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
#include "DebugCameraController.h"


namespace Urho3D
{

DebugCameraController::DebugCameraController(Context* context)
    : LogicComponent(context)
{
    SetUpdateEventMask(USE_NO_EVENT);
}

void DebugCameraController::Start()
{
}

void DebugCameraController::Stop()
{
}

void DebugCameraController::Update(float timeStep)
{
    // Do not move if the UI has a focused element
    if (GetUI()->GetFocusElement())
        return;

    // Do not move if interacting with UI controls
    if (GetSystemUI()->IsAnyItemActive())
        return;

    Input* input = GetInput();

    // Movement speed as world units per second
    float moveSpeed_ = speed_;
    if (input->GetKeyDown(KEY_SHIFT))
    {
        moveSpeed_ *= 2;

        if (input->GetKeyPress(KEY_KP_PLUS))
            speed_ += 1.f;
        else if (input->GetKeyPress(KEY_KP_MINUS))
            speed_ -= 1.f;
    }

    if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        IntVector2 delta = input->GetMouseMove();

        if (input->IsMouseVisible() && delta != IntVector2::ZERO)
            input->SetMouseVisible(false);

        auto yaw = GetNode()->GetRotation().EulerAngles().x_;
        if ((yaw > -90.f && yaw < 90.f) || (yaw <= -90.f && delta.y_ > 0) || (yaw >= 90.f && delta.y_ < 0))
            GetNode()->RotateAround(Vector3::ZERO, Quaternion(mouseSensitivity_ * delta.y_, Vector3::RIGHT), TS_LOCAL);
        GetNode()->RotateAround(GetNode()->GetPosition(), Quaternion(mouseSensitivity_ * delta.x_, Vector3::UP), TS_WORLD);
    }
    else if (!input->IsMouseVisible())
        input->SetMouseVisible(true);

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        GetNode()->Translate(Vector3::FORWARD * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_S))
        GetNode()->Translate(Vector3::BACK * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_A))
        GetNode()->Translate(Vector3::LEFT * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_D))
        GetNode()->Translate(Vector3::RIGHT * moveSpeed_ * timeStep);
}

DebugCameraController2D::DebugCameraController2D(Context* context)
    : LogicComponent(context)
{
    SetUpdateEventMask(USE_NO_EVENT);
}

void DebugCameraController2D::Start()
{
}

void DebugCameraController2D::Stop()
{
}

void DebugCameraController2D::Update(float timeStep)
{
    // Do not move if the UI has a focused element
    if (GetUI()->GetFocusElement())
        return;

    // Do not move if interacting with UI controls
    if (GetSystemUI()->IsAnyItemActive())
        return;

    Input* input = GetInput();

    // Movement speed as world units per second
    float moveSpeed_ = speed_;
    if (input->GetKeyDown(KEY_SHIFT))
    {
        moveSpeed_ *= 2;

        if (input->GetKeyPress(KEY_KP_PLUS))
            speed_ += 1.f;
        else if (input->GetKeyPress(KEY_KP_MINUS))
            speed_ -= 1.f;
    }

    if (input->GetMouseButtonDown(MOUSEB_RIGHT))
    {
        IntVector2 delta = input->GetMouseMove();

        if (input->IsMouseVisible() && delta != IntVector2::ZERO)
            input->SetMouseVisible(false);
        GetNode()->Translate2D(Vector2{(float)delta.x_ * -1.f, (float)delta.y_} * moveSpeed_ * timeStep);
    }
    else if (!input->IsMouseVisible())
        input->SetMouseVisible(true);

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        GetNode()->Translate(Vector3::UP * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_S))
        GetNode()->Translate(Vector3::DOWN * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_A))
        GetNode()->Translate(Vector3::LEFT * moveSpeed_ * timeStep);
    if (input->GetKeyDown(KEY_D))
        GetNode()->Translate(Vector3::RIGHT * moveSpeed_ * timeStep);
}

}
