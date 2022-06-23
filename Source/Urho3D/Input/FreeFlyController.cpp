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

#include "FreeFlyController.h"

#include "../Core/CoreEvents.h"
#include "../UI/UI.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Camera.h"
#include "../Scene/Node.h"

namespace Urho3D
{
namespace
{

float ApplyDeadZone(float value, float deadZone)
{
    if (value >= -deadZone && value <= deadZone)
        return 0.0f;
    return (value - Sign(value) * deadZone) / (1.0f - deadZone);
}

}

FreeFlyController::FreeFlyController(Context* context)
    : Component(context)
    , multitouchAdapter_(context)
{
    SubscribeToEvent(&multitouchAdapter_, E_MULTITOUCH, URHO3D_HANDLER(FreeFlyController, HandleMultitouch));
}

void FreeFlyController::OnSetEnabled() { UpdateEventSubscription(); }


void FreeFlyController::OnNodeSet(Node* node)
{
    UpdateEventSubscription();
}

void FreeFlyController::UpdateEventSubscription()
{
    bool enabled = IsEnabledEffective();

    multitouchAdapter_.SetEnabled(enabled);

    if (enabled && !subscribed_)
    {
        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(FreeFlyController, HandleUpdate));
        subscribed_ = true;
    }
    else if (subscribed_)
    {
        UnsubscribeFromEvent(E_UPDATE);
        subscribed_ = false;
    }
}

void FreeFlyController::HandleMultitouch(StringHash /*eventType*/, VariantMap& eventData)
{
    using namespace Multitouch;

    const MultitouchEventType eventType = static_cast<MultitouchEventType>(eventData[P_EVENTTYPE].GetInt());

    if (eventType == MULTITOUCH_MOVE)
    {
        Graphics* graphics = GetSubsystem<Graphics>();
        Camera* camera = GetComponent<Camera>();
        float sensitivity = touchSensitivity_ * 90.0f / 1080.0f;
        if (graphics && camera)
        {
            sensitivity = touchSensitivity_ * camera->GetFov() / graphics->GetHeight();
        }
        const unsigned numFingers = eventData[P_NUMFINGERS].GetUInt();
        const int dx = eventData[P_DX].GetInt();
        const int dy = eventData[P_DY].GetInt();
        if (numFingers == 1)
        {
            UpdateCameraAngles();
            Vector3 eulerAngles = lastKnownEulerAngles_;
            eulerAngles.y_ -= sensitivity * dx;
            eulerAngles.x_ -= sensitivity * dy;
            SetCameraAngles(eulerAngles);
        }
        else if (numFingers == 2)
        {
            Vector3 pos = node_->GetPosition();
            const IntVector2 dsize = eventData[P_DSIZE].GetIntVector2();

            pos += -node_->GetRight() * (sensitivity * dx);
            pos += node_->GetUp() * (sensitivity * dy);
            pos += node_->GetDirection() * (sensitivity * (dsize.x_ + dsize.y_));

            node_->SetPosition(pos);
        }
    }
}

void FreeFlyController::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Then execute user-defined update function
    Update(eventData[P_TIMESTEP].GetFloat());
}


FreeFlyController::~FreeFlyController() = default;

void FreeFlyController::RegisterObject(Context* context)
{
    context->AddFactoryReflection<FreeFlyController>();

    URHO3D_ATTRIBUTE("Speed", float, speed_, 20.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("AcceleratedSpeed", float, acceleratedSpeed_, 100.0f, AM_DEFAULT);
}

void FreeFlyController::SetSpeed(float speed)
{
    speed_ = speed;
}

void FreeFlyController::SetAcceleratedSpeed(float speed)
{
    acceleratedSpeed_ = speed;
}

void FreeFlyController::SetCameraAngles(Vector3 eulerAngles)
{
    eulerAngles.x_ = Clamp(eulerAngles.x_, -90.0f, 90.0f);
    lastKnownEulerAngles_ = eulerAngles;
    lastKnownCameraRotation_ = Quaternion(eulerAngles);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    node_->SetRotation(lastKnownCameraRotation_.value());
}

void FreeFlyController::UpdateCameraAngles()
{
    auto rotation = node_->GetRotation();
    if (!lastKnownCameraRotation_.has_value() || !lastKnownCameraRotation_.value().Equals(rotation))
    {
        lastKnownCameraRotation_ = rotation;
        lastKnownEulerAngles_ = rotation.EulerAngles();
        // Detect gimbal lock and restore from it.
        if (Abs(lastKnownEulerAngles_.x_) > 89.999f && Abs(lastKnownEulerAngles_.y_) < 0.001f)
        {
            ea::swap(lastKnownEulerAngles_.y_, lastKnownEulerAngles_.z_);
        }
    }
}

void FreeFlyController::HandleKeyboardAndMouse(float timeStep)
{
    auto* input = GetSubsystem<Input>();

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    UpdateCameraAngles();
    Vector3 eulerAngles = lastKnownEulerAngles_;
    IntVector2 mouseMove = input->GetMouseMove();
    eulerAngles.y_ += mouseSensitivity_ * mouseMove.x_;
    eulerAngles.x_ += mouseSensitivity_ * mouseMove.y_;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    {
        float speed = input->GetKeyDown(KEY_SHIFT) ? acceleratedSpeed_ : speed_;
        if (input->GetScancodeDown(SCANCODE_W))
            node_->Translate(Vector3::FORWARD * speed * timeStep);
        if (input->GetScancodeDown(SCANCODE_S))
            node_->Translate(Vector3::BACK * speed * timeStep);
        if (input->GetScancodeDown(SCANCODE_A))
            node_->Translate(Vector3::LEFT * speed * timeStep);
        if (input->GetScancodeDown(SCANCODE_D))
            node_->Translate(Vector3::RIGHT * speed * timeStep);
        if (input->GetScancodeDown(SCANCODE_Q))
            node_->Translate(Vector3::DOWN * speed * timeStep);
        if (input->GetScancodeDown(SCANCODE_E))
            node_->Translate(Vector3::UP * speed * timeStep);
    }

    if (input->GetNumJoysticks() > 0)
    {
        auto state = input->GetJoystickByIndex(0);
        if (state)
        {
            unsigned numAxes = state->GetNumAxes();

            float speed = speed_;
            // Apply acceleration
            if (numAxes > 4)
            {
                float value = ApplyDeadZone(state->GetAxisPosition(4), axisDeadZone_);
                speed = Lerp(speed_, acceleratedSpeed_, value);
            }

            if (numAxes > 0)
            {
                float value = ApplyDeadZone(state->GetAxisPosition(0), axisDeadZone_);
                node_->Translate(Vector3::RIGHT * speed * timeStep * value);
            }
            if (numAxes > 1)
            {
                float value =  ApplyDeadZone(-state->GetAxisPosition(1), axisDeadZone_);
                node_->Translate(Vector3::FORWARD * speed * timeStep * value);
            }
            if (numAxes > 2)
            {
                float value = ApplyDeadZone(state->GetAxisPosition(2), axisDeadZone_);
                eulerAngles.y_ += value * timeStep * axisSensitivity_;
            }
            if (numAxes > 3)
            {
                float value = ApplyDeadZone(state->GetAxisPosition(3), axisDeadZone_);
                eulerAngles.x_ += value * timeStep * axisSensitivity_;
            }

            unsigned numHats = state->GetNumHats();
            if (numHats > 0)
            {
                int value = state->GetHatPosition(0);
                if (0 != (value & HAT_UP))
                    node_->Translate(Vector3::FORWARD * speed * timeStep);
                if (0 != (value & HAT_DOWN))
                    node_->Translate(Vector3::BACK * speed * timeStep);
                if (0 != (value & HAT_LEFT))
                    node_->Translate(Vector3::LEFT * speed * timeStep);
                if (0 != (value & HAT_RIGHT))
                    node_->Translate(Vector3::RIGHT * speed * timeStep);
            }
        }
    }

    SetCameraAngles(eulerAngles);
}

void FreeFlyController::Update(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();
    if (input->GetMouseMode() == MM_FREE)
    {
        if (input->GetMouseButtonDown(MOUSEB_RIGHT))
        {
            HandleKeyboardAndMouse(timeStep);
        }
    }
    else
    {
        HandleKeyboardAndMouse(timeStep);
    }
}

} // namespace Urho3D
