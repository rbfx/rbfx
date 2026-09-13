// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
