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
    SetUpdateEventMask(USE_UPDATE);
}

void DebugCameraController::Start()
{
    // Add a head-light so we can view even unlit objects.
    light_ = GetNode()->CreateComponent<Light>();
    light_->SetColor(Color::WHITE);
    light_->SetLightType(LIGHT_DIRECTIONAL);

    // Initialize yaw and pitch values so that first rotation does not snap camera to unexpected angles.
    auto rot = GetNode()->GetRotation();
    yaw_ = rot.YawAngle();
    pitch_ = rot.PitchAngle();
}

void DebugCameraController::Stop()
{
    light_->Remove();
    light_ = nullptr;
}

void DebugCameraController::Update(float timeStep)
{
    // Do not move if the UI has a focused element
    if (GetUI()->GetFocusElement())
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
        if (input->IsMouseVisible())
            input->SetMouseVisible(false);
        // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += mouseSensitivity_ * mouseMove.x_;
        pitch_ = Clamp(pitch_ + mouseSensitivity_ * mouseMove.y_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        GetNode()->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
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

}
