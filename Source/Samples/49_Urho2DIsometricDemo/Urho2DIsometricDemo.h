// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"
#include "Utilities2D/Sample2D.h"

class CharacterIsometric;
class Sample2D;

/// Tile map example.
/// This sample demonstrates:
///    - Creating an isometric 2D scene with tile map
///    - Displaying the scene using the Renderer subsystem
///    - Handling keyboard to move a character and zoom 2D camera
///    - Generating physics shapes from the tmx file's objects
///    - Displaying debug geometry for physics and tile map
/// Note that this sample uses some functions from Sample2D utility class.
class Urho2DIsometricDemo : public Sample
{
    URHO3D_OBJECT(Urho2DIsometricDemo, Sample);

public:
    /// Construct.
    explicit Urho2DIsometricDemo(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle the logic update event.
    void Update(float timeStep) override;
    /// Handle the logic post update event.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle the post render update event.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle the end rendering event.
    void HandleSceneRendered(StringHash eventType, VariantMap& eventData);
    /// Handle reloading the scene.
    void ReloadScene(bool reInit);
    /// Handle the contact begin event (Box2D contact listener).
    void HandleCollisionBegin(StringHash eventType, VariantMap& eventData);
    /// Handle 'PLAY' button released event.
    void HandlePlayButton(StringHash eventType, VariantMap& eventData);

    /// The controllable character component.
    WeakPtr<CharacterIsometric> character2D_;
    /// Camera's zoom (used to scale movement speed based on camera zoom).
    float zoom_;
    /// Flag for drawing debug geometry.
    bool drawDebug_;

    /// Sample2D utility object.
    SharedPtr<Sample2D> sample2D_;
};
