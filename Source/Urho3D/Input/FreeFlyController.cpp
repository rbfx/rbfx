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

#include "../Precompiled.h"

#include "FreeFlyController.h"

#include "../Core/CoreEvents.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Renderer.h"
#include "../Input/Input.h"
#include "../Input/InputEvents.h"
#include "../Scene/Node.h"
#include "../UI/UI.h"
#include "../Engine/StateManager.h"

namespace Urho3D
{

FreeFlyController::FreeFlyController(Context* context)
    : Component(context)
    , multitouchAdapter_(context)
{
    ignoreJoystickId_ = GetSubsystem<Input>()->FindAccelerometerJoystickId();
    SubscribeToEvent(&multitouchAdapter_, E_MULTITOUCH, URHO3D_HANDLER(FreeFlyController, HandleMultitouch));
}

void FreeFlyController::OnSetEnabled() { UpdateEventSubscription(); }


void FreeFlyController::OnNodeSet(Node* previousNode, Node* currentNode)
{
    UpdateEventSubscription();
}

void FreeFlyController::UpdateEventSubscription()
{
    const bool enabled = IsEnabledEffective();

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
    URHO3D_ATTRIBUTE("Accelerated Speed", float, acceleratedSpeed_, 100.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Min Pitch", float, minPitch_, -90.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Pitch", float, maxPitch_, 90.0f, AM_DEFAULT);
}

void FreeFlyController::SetCameraRotation(Quaternion quaternion)
{
    lastKnownEulerAngles_ = quaternion.EulerAngles();
    lastKnownCameraRotation_ = quaternion;

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    node_->SetRotation(lastKnownCameraRotation_.value());
}

void FreeFlyController::SetCameraAngles(Vector3 eulerAngles)
{
    eulerAngles.x_ = Clamp(eulerAngles.x_, minPitch_, maxPitch_);
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
    }
}

FreeFlyController::Movement FreeFlyController::HandleWheel(const JoystickState* state, float timeStep)
{
    Movement movement;

    const unsigned numAxes = state->GetNumAxes();
    float speed = speed_;

    // Wheel
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(0));
        movement.rotation_.y_ += value * timeStep * axisSensitivity_;
    }
    // Acceleration
    if (numAxes > 1)
    {
        AxisAdapter pedalAdapter = axisAdapter_;
        pedalAdapter.SetInverted(true);
        pedalAdapter.SetNeutralValue(-1.0f);
        if (state->HasAxisPosition(1))
        {
            const float value = pedalAdapter.Transform(state->GetAxisPosition(1));
            movement.translation_.z_ += value * speed * timeStep;
        }
        // Brake
        if (state->HasAxisPosition(2))
        {
            const float value = pedalAdapter.Transform(state->GetAxisPosition(2));
            movement.translation_.z_ -= value * speed * timeStep;
        }
    }
    return movement;
}

FreeFlyController::Movement FreeFlyController::HandleFlightStick(const JoystickState* state, float timeStep)
{
    Movement movement;

    const unsigned numAxes = state->GetNumAxes();
    float speed = speed_;
    // Roll
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(0));
        movement.rotation_.z_ -= value * timeStep * axisSensitivity_;
    }
    // Pitch
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(1));
        movement.rotation_.x_ -= value * timeStep * axisSensitivity_;
    }
    // Yaw
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(3));
        movement.rotation_.y_ += value * timeStep * axisSensitivity_;
    }
    // Throttle
    if (state->HasAxisPosition(2))
    {
        AxisAdapter pedalAdapter = axisAdapter_;
        pedalAdapter.SetInverted(true);
        pedalAdapter.SetNeutralValue(-1.0f);
        const float value = pedalAdapter.Transform(state->GetAxisPosition(2));
        movement.translation_.z_ += value * speed * timeStep;
    }
    // Rocker
    if (state->HasAxisPosition(4))
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(4));
        movement.translation_.x_ += value * speed * timeStep;
    }
    return movement;
}

FreeFlyController::Movement FreeFlyController::HandleController(const JoystickState* state, float timeStep)
{
    Movement movement;

    float speed = speed_;
    // Apply acceleration
    if (state->HasAxisPosition(CONTROLLER_AXIS_TRIGGERLEFT))
    {
        AxisAdapter triggerAdapter = axisAdapter_;
        triggerAdapter.SetNeutralValue(-1.0f);
        const float value = axisAdapter_.Transform(state->GetAxisPosition(CONTROLLER_AXIS_TRIGGERLEFT));
        speed = Lerp(speed_, acceleratedSpeed_, Clamp(value, 0.0f, 1.0f));
    }
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(CONTROLLER_AXIS_LEFTX));
        movement.translation_.x_ += value * speed * timeStep;
    }
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(CONTROLLER_AXIS_LEFTY));
        movement.translation_.z_ -= value * speed * timeStep;
    }
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(CONTROLLER_AXIS_RIGHTX));
        movement.rotation_.y_ += value * timeStep * axisSensitivity_;
    }
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(CONTROLLER_AXIS_RIGHTY));
        movement.rotation_.x_ += value * timeStep * axisSensitivity_;
    }

    const unsigned numHats = state->GetNumHats();
    if (numHats > 0)
    {
        const int value = state->GetHatPosition(0);
        if (0 != (value & HAT_UP))
            movement.translation_.z_ += speed * timeStep;
        if (0 != (value & HAT_DOWN))
            movement.translation_.z_ -= speed * timeStep;
        if (0 != (value & HAT_LEFT))
            movement.translation_.x_ -= speed * timeStep;
        if (0 != (value & HAT_RIGHT))
            movement.translation_.x_ += speed * timeStep;
    }

    return movement;
}

FreeFlyController::Movement FreeFlyController::HandleGenericJoystick(const JoystickState* state, float timeStep)
{
    Movement movement;

    const unsigned numAxes = state->GetNumAxes();
    float speed = speed_;
    // Apply acceleration
    if (state->HasAxisPosition(4))
    {
        AxisAdapter triggerAdapter = axisAdapter_;
        triggerAdapter.SetNeutralValue(-1.0f);
        const float value = (1.0f + axisAdapter_.Transform(state->GetAxisPosition(4))) * 0.5f;
        speed = Lerp(speed_, acceleratedSpeed_, Clamp(value, 0.0f, 1.0f));
    }

    if (state->HasAxisPosition(0))
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(0));
        if (value != 0)
        {
            movement.translation_.x_ += value * speed * timeStep;
        }
    }
    if (state->HasAxisPosition(1))
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(1));
        movement.translation_.z_ -= value * speed * timeStep;
    }
    if (state->HasAxisPosition(2))
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(2));
        movement.rotation_.y_ += value * timeStep * axisSensitivity_;
    }
    if (state->HasAxisPosition(3))
    {
        const float value = axisAdapter_.Transform(state->GetAxisPosition(3));
        movement.rotation_.x_ += value * timeStep * axisSensitivity_;
    }

    const unsigned numHats = state->GetNumHats();
    if (numHats > 0)
    {
        const int value = state->GetHatPosition(0);
        if (0 != (value & HAT_UP))
            movement.translation_.z_ += speed * timeStep;
        if (0 != (value & HAT_DOWN))
            movement.translation_.z_ -= speed * timeStep;
        if (0 != (value & HAT_LEFT))
            movement.translation_.x_ -= speed * timeStep;
        if (0 != (value & HAT_RIGHT))
            movement.translation_.x_ += speed * timeStep;
    }

    return movement;
}
FreeFlyController::Movement FreeFlyController::HandleMouse() const
{
    const auto* input = GetSubsystem<Input>();
    const IntVector2 mouseMove = input->GetMouseMove();
    Movement movement;
    movement.rotation_.y_ += mouseSensitivity_ * mouseMove.x_;
    movement.rotation_.x_ += mouseSensitivity_ * mouseMove.y_;
    return movement;
}

FreeFlyController::Movement FreeFlyController::HandleKeyboard(float timeStep) const
{
    Movement movement{};
    const auto* input = GetSubsystem<Input>();

    const float speed = input->GetKeyDown(KEY_SHIFT) ? acceleratedSpeed_ : speed_;
    if (input->GetScancodeDown(SCANCODE_W))
        movement.translation_.z_ += speed * timeStep;
    if (input->GetScancodeDown(SCANCODE_S))
        movement.translation_.z_ -= speed * timeStep;
    if (input->GetScancodeDown(SCANCODE_A))
        movement.translation_.x_ -= speed * timeStep;
    if (input->GetScancodeDown(SCANCODE_D))
        movement.translation_.x_ += speed * timeStep;
    if (input->GetScancodeDown(SCANCODE_Q))
        movement.translation_.y_ -= speed * timeStep;
    if (input->GetScancodeDown(SCANCODE_E))
        movement.translation_.y_ += speed * timeStep;

    return movement;
}

void FreeFlyController::HandleKeyboardMouseAndJoysticks(float timeStep)
{
    auto* input = GetSubsystem<Input>();

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    UpdateCameraAngles();

    // World space rotation (first person shooter)
    Movement worldMovement{};
    // Local space rotation (flight sim)
    Movement localMovement{};

    worldMovement += HandleMouse();
    worldMovement += HandleKeyboard(timeStep);

    const unsigned numJoysticks = input->GetNumJoysticks();
    for (unsigned joystickIndex = 0; joystickIndex < numJoysticks; ++joystickIndex)
    {
        const auto state = input->GetJoystickByIndex(joystickIndex);
        if (state)
        {
            if (state->joystickID_ == ignoreJoystickId_)
                continue;

            switch (state->type_)
            {
            // Ignore odd devices
            case JOYSTICK_TYPE_GUITAR: break;
            case JOYSTICK_TYPE_DRUM_KIT: break;
            case JOYSTICK_TYPE_THROTTLE: break;
            // Handle known devices
            case JOYSTICK_TYPE_WHEEL: worldMovement += HandleWheel(state, timeStep); break;
            case JOYSTICK_TYPE_FLIGHT_STICK: localMovement += HandleFlightStick(state, timeStep); break;
            case JOYSTICK_TYPE_GAMECONTROLLER: worldMovement += HandleController(state, timeStep); break;
            default: worldMovement += HandleGenericJoystick(state, timeStep); break;
            }
        }
    }

    if (localMovement.rotation_ == Vector3::ZERO)
    {
        Vector3 eulerAngles = lastKnownEulerAngles_ + worldMovement.rotation_;
        SetCameraAngles(eulerAngles);
    }
    else
    {
        Vector3 eulerAngles = lastKnownEulerAngles_ + worldMovement.rotation_;
        eulerAngles.x_ = Clamp(eulerAngles.x_, minPitch_, maxPitch_);
        SetCameraRotation(Quaternion(eulerAngles) * Quaternion(localMovement.rotation_));
    }

    const Vector3 translation = localMovement.translation_ + worldMovement.translation_;
    if (!translation.Equals(Vector3::ZERO))
    {
        node_->Translate(translation, TS_LOCAL);
    }
}

void FreeFlyController::Update(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();
    if (isActive_ || input->GetMouseMode() == MM_FREE)
    {
        if (!isActive_ && input->GetMouseButtonPress(MOUSEB_RIGHT))
        {
            isActive_ = true;
            oldMouseVisible_ = input->IsMouseVisible();
            oldMouseMode_ = input->GetMouseMode();
            input->SetMouseVisible(false);
            input->SetMouseMode(MM_RELATIVE);
        }
        else if (isActive_ && !input->GetMouseButtonDown(MOUSEB_RIGHT))
        {
            isActive_ = false;
            input->SetMouseVisible(oldMouseVisible_);
            input->SetMouseMode(oldMouseMode_);
        }

        if (isActive_)
        {
            HandleKeyboardMouseAndJoysticks(timeStep);
        }
    }
    else
    {
        HandleKeyboardMouseAndJoysticks(timeStep);
    }
}

} // namespace Urho3D
