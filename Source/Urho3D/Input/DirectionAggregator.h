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

#include "../Core/Object.h"
#include "InputConstants.h"
#include <EASTL/fixed_vector.h>

namespace Urho3D
{

/// Class to aggregate all movement inputs into a signle direction vector.
class URHO3D_API DirectionAggregator : public Object
{
    URHO3D_OBJECT(DirectionAggregator, Object)

private:
    enum class InputType
    {
        External = 0,
        Keyboard = 1,
        JoystickAxis = 100,
        JoystickDPad = 200,
    };

    struct AxisState
    {
        InputType input_;
        float value_;
    };
    typedef ea::fixed_vector<AxisState, 4> InputVector;

public:
    /// Construct.
    explicit DirectionAggregator(Context* context);

    /// Set enabled flag. The object subscribes for events when enabled.
    /// Input messages could be passed to the object manually when disabled.
    void SetEnabled(bool enabled);

    bool IsEnabled() const { return enabled_; }

    /// Get aggregated direction vector with X pointing right and Y pointing down (similar to gamepad axis).
    Vector2 GetDirection() const;

private:
    void SubscribeToEvents();

    void UnsubscribeFromEvents();

    void HandleKeyDown(StringHash eventType, VariantMap& args);
    void HandleKeyUp(StringHash eventType, VariantMap& args);
    void HandleJoystickAxisMove(StringHash eventType, VariantMap& args);
    void HandleJoystickHatMove(StringHash eventType, VariantMap& args);
    void HandleJoystickDisconnected(StringHash eventType, VariantMap& args);

    void UpdateAxis(ea::fixed_vector<AxisState, 4>& activeStates, AxisState state);

    bool enabled_{false};
    Input* input_{};
    InputVector verticalAxis_;
    InputVector horizontalAxis_;
    float axisDeadZone_{0.1f};
    int ignoreJoystickId_{-10};
};

} // namespace Urho3D
