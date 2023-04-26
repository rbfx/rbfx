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

#include "Sample.h"
#include <Urho3D/Input/DirectionalPadAdapter.h>
#include <Urho3D/Input/DirectionAggregator.h>
#include <EASTL/queue.h>

/// Aggregated input example to compare raw input events to aggregated events.
/// Demonstrates how
/// - DirectionAggregator evaluates aggregated direction from all input devices.
/// - DirectionalPadAdapter translates input events into simple directional events similar to d-pad.
class AggregatedInput : public Sample
{
    URHO3D_OBJECT(AggregatedInput, Sample);

public:
    /// Construct.
    explicit AggregatedInput(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct a new Text instance, containing the 'Hello World' String, and add it to the UI root element.
    void CreateUI();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();

    void Deactivate() override;

    void AddFilteredEvent(const ea::string& str);
    void AddRawEvent(const ea::string& str);

    void HandleDPadKeyDown(StringHash eventType, VariantMap& args);
    void HandleDPadKeyUp(StringHash eventType, VariantMap& args);
    void HandleKeyDown(StringHash eventType, VariantMap& args);
    void HandleKeyUp(StringHash eventType, VariantMap& args);
    void HandleJoystickAxisMove(StringHash eventType, VariantMap& args);
    void HandleJoystickHatMove(StringHash eventType, VariantMap& args);
    void HandleJoystickDisconnected(StringHash eventType, VariantMap& args);
    void HandleTouchBegin(StringHash eventType, VariantMap& args);
    void HandleTouchMove(StringHash eventType, VariantMap& args);
    void HandleTouchEnd(StringHash eventType, VariantMap& args);
    
    /// Handle the logic update event.
    void Update(float timestep) override;

    DirectionAggregator aggregatedInput_;
    DirectionalPadAdapter dpadInput_;

    SharedPtr<Sprite> analogPivot_;
    SharedPtr<Sprite> analogMarker_;
    SharedPtr<Sprite> upMarker_;
    SharedPtr<Sprite> leftMarker_;
    SharedPtr<Sprite> rightMarker_;
    SharedPtr<Sprite> downMarker_;
    SharedPtr<Text> rawEventsLog_;
    SharedPtr<Text> filteredEventsLog_;
    ea::string rawEventsText_;
    ea::string filteredEventsText_;

    ea::queue<ea::string> rawEvents_;
    ea::queue<ea::string> filteredEvents_;
};
