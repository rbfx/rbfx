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

#include "CameraController.h"
#include "../Core/CoreEvents.h"
#include "../UI/UI.h"
#include "../Input/Input.h"
#include "../Graphics/Renderer.h"

namespace Urho3D
{
CameraController::CameraController(Context* context)
    : Object(context)
{
    SetEnabled(true);
}

CameraController::~CameraController()
{
    SetEnabled(false);
}

void CameraController::SetEnabled(bool enable)
{
    if (enable != enabled_)
    {
        enabled_ = enable;
        if (enabled_)
        {
            // Subscribe HandleUpdate() method for processing update events
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(CameraController, HandleUpdate));
        }
        else
        {
            // Unsubscribe HandleUpdate() method from processing update events
            UnsubscribeFromEvent(E_UPDATE);
        }
    }
}
void CameraController::SetSpeed(float speed)
{
    speed_ = speed;
}

void CameraController::SetAcceleratedSpeed(float speed)
{
    acceleratedSpeed_ = speed;
}

Camera* CameraController::GetCamera() const
{
    return camera_;
}

void CameraController::MoveCamera(float timeStep, Camera* camera)
{
    if (!camera)
        return;

    auto cameraNode = camera->GetNode();
    if (!cameraNode)
        return;

    auto* input = GetSubsystem<Input>();

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    Vector3 eulerAngles = cameraNode->GetRotation().EulerAngles();
    IntVector2 mouseMove = input->GetMouseMove();
    eulerAngles.y_ += mouseSensitivity_ * mouseMove.x_;
    eulerAngles.x_ += mouseSensitivity_ * mouseMove.y_;
    eulerAngles.x_ = Clamp(eulerAngles.x_, -89.999f, 89.999f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode->SetRotation(Quaternion(eulerAngles));

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    float speed = input->GetKeyDown(KEY_SHIFT) ? acceleratedSpeed_ : speed_;
    if (input->GetKeyDown(KEY_W))
        cameraNode->Translate(Vector3::FORWARD * speed * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode->Translate(Vector3::BACK * speed * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode->Translate(Vector3::LEFT * speed * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode->Translate(Vector3::RIGHT * speed * timeStep);
}

void CameraController::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Get active camera
    Camera* camera = camera_;
    if (!camera)
    {
        Renderer* renderer = GetSubsystem<Renderer>();
        if (renderer)
        {
            auto* viewport = renderer->GetViewport(0);
            if (viewport)
            {
                camera = viewport->GetCamera();
                if (!camera)
                {
                    return;
                }
            }
        }
    }

    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();
    if (input->GetMouseMode() == MM_FREE)
    {
        if (input->GetMouseButtonDown(MOUSEB_RIGHT))
        {
            MoveCamera(timeStep, camera);
        }
    }
    else
    {
        MoveCamera(timeStep, camera);
    }
}

} // namespace Urho3D
