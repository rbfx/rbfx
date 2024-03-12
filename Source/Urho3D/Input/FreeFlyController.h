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
#pragma once

#include "../Scene/Component.h"
#include "../Input/Input.h"
#include "../Input/MultitouchAdapter.h"
#include "../Input/AxisAdapter.h"

namespace Urho3D
{

class URHO3D_API FreeFlyController : public Component
{
    URHO3D_OBJECT(FreeFlyController, Component)

    static inline constexpr float DEFAULT_MOUSE_SENSITIVITY{0.1f};
    // 90' motion per inch
    static inline constexpr float DEFAULT_TOUCH_ROTATION_SENSITIVITY{1.0f};
    // Degrees per second
    static inline constexpr float DEFAULT_AXIS_ROTATION_SENSITIVITY{100.0f};

private:
    struct Movement
    {
        Vector3 rotation_{Vector3::ZERO};
        Vector3 translation_{Vector3::ZERO};
        Movement& operator+=(const Movement& rhs)
        {
            rotation_ += rhs.rotation_;
            translation_ += rhs.translation_;
            return *this;
        }
    };

public:
    /// Construct.
    explicit FreeFlyController(Context* context);
    /// Destruct.
    ~FreeFlyController() override;

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change. Changes update event subscription.
    void OnSetEnabled() override;

    /// Attributes
    /// @{
    void SetSpeed(float value) { speed_ = value; }
    float GetSpeed() const { return speed_; }
    void SetAcceleratedSpeed(float value) { acceleratedSpeed_ = value; }
    float GetAcceleratedSpeed() const { return acceleratedSpeed_; }
    float GetMinPitch() const { return minPitch_; }
    void SetMinPitch(float value) { minPitch_ = value; }
    float GetMaxPitch() const { return maxPitch_; }
    void SetMaxPitch(float value) { maxPitch_ = value; }
    float GetMouseSensitivity() const { return mouseSensitivity_; }
    void SetMouseSensitivity(float value) { mouseSensitivity_ = value; }
    float GetTouchSensitivity() const { return touchSensitivity_; }
    void SetTouchSensitivity(float value) { touchSensitivity_ = value; }
    float GetAxisSensitivity() const { return axisSensitivity_; }
    void SetAxisSensitivity(float value) { axisSensitivity_ = value; }
    /// @}

private:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;

    /// Subscribe/unsubscribe to update events based on current enabled state and update event mask.
    void UpdateEventSubscription();

    /// Handle scene update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle scene update. Called by LogicComponent base class.
    void Update(float timeStep);
    /// Handle mouse input.
    Movement HandleMouse() const;
    /// Handle keyboard input.
    Movement HandleKeyboard(float timeStep) const;
    /// Handle controller input.
    Movement HandleController(const JoystickState* state, float timeStep);
    /// Handle controller input.
    Movement HandleGenericJoystick(const JoystickState* state, float timeStep);
    /// Handle wheel input.
    Movement HandleWheel(const JoystickState* state, float timeStep);
    /// Handle flight stick input.
    Movement HandleFlightStick(const JoystickState* state, float timeStep);
    /// Handle keyboard and mouse input.
    void HandleKeyboardMouseAndJoysticks(float timeStep);
    /// Handle multitouch input event.
    void HandleMultitouch(StringHash eventType, VariantMap& eventData);
    /// Detect camera angles if camera has changed.
    void UpdateCameraAngles();
    /// Set camera rotation.
    void SetCameraRotation(Quaternion quaternion);
    /// Update camera rotation.
    void SetCameraAngles(Vector3 eulerAngles);

private:
    /// Camera speed.
    float speed_{20.0f};
    /// Camera accelerated speed.
    float acceleratedSpeed_{100.0f};
    /// Mouse sensitivity
    float mouseSensitivity_{DEFAULT_MOUSE_SENSITIVITY};
    /// Touch sensitivity
    float touchSensitivity_{DEFAULT_TOUCH_ROTATION_SENSITIVITY};
    /// Axis sensitivity
    float axisSensitivity_{DEFAULT_AXIS_ROTATION_SENSITIVITY};
    /// Pitch range
    float minPitch_{-90.0f};
    float maxPitch_{90.0f};
    /// Gamepad default axis adapter
    AxisAdapter axisAdapter_{};
    /// Is subscribed to update
    bool subscribed_{false};
    /// Multitouch input adapter
    MultitouchAdapter multitouchAdapter_;
    /// Last known camera rotation to keep track of yaw and pitch.
    ea::optional<Quaternion> lastKnownCameraRotation_;
    /// Last known yaw, pitch and roll to prevent gimbal lock.
    Vector3 lastKnownEulerAngles_;
    /// Joystick to ignore (SDL gyroscope virtual joystick)
    int ignoreJoystickId_{-1};

    /// Whether the rotation is performing now.
    bool isActive_{};
    bool oldMouseVisible_{};
    MouseMode oldMouseMode_{};
};

} // namespace Urho3D
