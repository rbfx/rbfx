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

#include "../Container/FlagSet.h"
#include "../Core/Object.h"
#include "../UI/UIElement.h"
#include "../Input/AxisAdapter.h"
#include <EASTL/fixed_vector.h>

namespace Urho3D
{

enum class DirectionAggregatorMask : unsigned
{
    None = 0,
    Keyboard = 1 << 0,
    Joystick = 1 << 1,
    Touch = 1 << 2,
    All = Keyboard | Joystick | Touch,
};
URHO3D_FLAGSET(DirectionAggregatorMask, DirectionAggregatorFlags);

/// Class to aggregate all movement inputs into a single direction vector.
class URHO3D_API DirectionAggregator : public Object
{
    URHO3D_OBJECT(DirectionAggregator, Object)

private:
    /// Type of input source.
    enum class InputType
    {
        // External input
        External = 0,
        // Keyboard input (WASD and arrows)
        Keyboard = 1,
        // Touch input
        Touch = 2,
        // Joystick Axis
        JoystickAxis = 3,
        // Joystick DPad (hat)
        JoystickDPad = 4,
    };

    // State of an axis
    struct AxisState
    {
        // Type of input device
        InputType input_;
        // Additional input information: Key scan code, joystick id etc.
        unsigned key_ {};
        // Value to accumulate
        float value_ {};
    };

    /// Type definition for active input sources
    typedef ea::fixed_vector<AxisState, 4> InputVector;

public:
    /// Construct.
    explicit DirectionAggregator(Context* context);
    /// Destruct.
    ~DirectionAggregator();

    /// Set enabled flag. The object subscribes for events when enabled.
    void SetEnabled(bool enabled);
    /// Set input device subscription mask.
    void SetSubscriptionMask(DirectionAggregatorFlags mask);
    /// Set UI element to filter touch events. Only touch events originated in the element going to be handled.
    void SetUIElement(UIElement* element);
    /// Set dead zone to mitigate axis drift.
    void SetDeadZone(float deadZone);

    /// Get enabled flag.
    bool IsEnabled() const { return enabled_; }
    /// Get input device subscription mask.
    DirectionAggregatorFlags GetSubscriptionMask() const { return enabledSubscriptions_; }
    /// Get UI element to filter touch events.
    UIElement* GetUIElement() const { return uiElement_; }
    /// Get dead zone.
    float GetDeadZone() const { return axisAdapter_.GetDeadZone(); }

    /// Get aggregated direction vector with X pointing right and Y pointing down (similar to gamepad axis).
    Vector2 GetDirection() const;

private:
    void UpdateSubscriptions(DirectionAggregatorFlags flags);

    void HandleInputFocus(StringHash eventType, VariantMap& args);
    void HandleKeyDown(StringHash eventType, VariantMap& args);
    void HandleKeyUp(StringHash eventType, VariantMap& args);
    void HandleJoystickAxisMove(StringHash eventType, VariantMap& args);
    void HandleJoystickHatMove(StringHash eventType, VariantMap& args);
    void HandleJoystickDisconnected(StringHash eventType, VariantMap& args);
    void HandleTouchBegin(StringHash eventType, VariantMap& args);
    void HandleTouchMove(StringHash eventType, VariantMap& args);
    void HandleTouchEnd(StringHash eventType, VariantMap& args);

    void UpdateAxis(ea::fixed_vector<AxisState, 4>& activeStates, AxisState state);

    /// Is aggregator enabled
    bool enabled_{false};
    /// Enabled subscriptions
    DirectionAggregatorFlags enabledSubscriptions_{DirectionAggregatorMask::All};
    /// Active subscriptions bitmask
    DirectionAggregatorFlags subscriptionFlags_{DirectionAggregatorMask::None};
    /// Cached input subsystem pointer
    Input* input_{};
    /// Collection of active vertical axis inputs
    InputVector verticalAxis_;
    /// Collection of active horizontal axis inputs
    InputVector horizontalAxis_;
    /// Joystick axis adapter
    AxisAdapter axisAdapter_{};
    /// Joystick to ignore (SDL gyroscope virtual joystick)
    int ignoreJoystickId_{-1};
    /// UI element to filter touch events
    WeakPtr<UIElement> uiElement_{};
    /// Identifier of active touch
    ea::optional<int> activeTouchId_{};
    /// Origin of the touch
    IntVector2 touchOrigin_{};
    /// Touch sensitivity
    float touchSensitivity_{};
};

} // namespace Urho3D
