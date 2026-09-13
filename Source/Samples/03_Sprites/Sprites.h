// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

/// Moving sprites example.
/// This sample demonstrates:
///     - Adding Sprite elements to the UI
///     - Storing custom data (sprite velocity) inside UI elements
///     - Handling frame update events in which the sprites are moved
class Sprites : public Sample
{
    // Enable type information.
    URHO3D_OBJECT(Sprites, Sample);

public:
    /// Construct.
    explicit Sprites(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    ea::string GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Construct the sprites.
    void CreateSprites();
    /// Move the sprites using the delta time step given.
    void MoveSprites(float timeStep);
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void Update(float timeStep) override;

    /// Vector to store the sprites for iterating through them.
    ea::vector<SharedPtr<Sprite> > sprites_;
};
