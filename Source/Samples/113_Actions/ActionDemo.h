// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"
#include <Urho3D/Actions/BaseAction.h>

/// This first example, maintaining tradition, prints a "Hello World" message.
/// Furthermore it shows:
///     - Using the Sample / Application classes, which initialize the Urho3D engine and run the main loop
///     - Adding a Text element to the graphical user interface
///     - Subscribing to and handling of update events
class ActionDemo : public Sample
{
    URHO3D_OBJECT(ActionDemo, Sample);

    struct DemoElement
    {
        SharedPtr<UIElement> element_;
        SharedPtr<Actions::BaseAction> action_;
    };
public:
    /// Construct.
    explicit ActionDemo(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct a new Text instance, containing the 'Hello World' String, and add it to the UI root element.
    void CreateUI();
    /// Add element to UI to showcase an action.
    DemoElement& AddElement(const IntVector2& pos, const SharedPtr<Actions::BaseAction>& action);
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();

    void HandleMouseClick(StringHash eventType, VariantMap& eventData);

    void Deactivate() override;
   
    /// Handle the logic update event.
    void Update(float timestep) override;

    ea::vector<DemoElement> markers_;
};
