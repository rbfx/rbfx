// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

namespace Urho3D
{
    class Node;
    class Scene;
}

class Vehicle2;

/// Vehicle example.
/// This sample demonstrates:
///     - Creating a heightmap terrain with collision
///     - Constructing 100 raycast vehicles
///     - Defining attributes (including node and component references) of a custom component
///     (Saving and loading is broken now)

class RaycastVehicleDemo : public Sample
{
    URHO3D_OBJECT(RaycastVehicleDemo, Sample);

public:
    /// Construct.
    explicit RaycastVehicleDemo(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create static scene content.
    void CreateScene();

    /// Create the vehicle.
    void CreateVehicle();

    /// Construct an instruction text to the UI.
    void CreateInstructions();

    /// Subscribe to necessary events.
    void SubscribeToEvents();

    /// Handle application update. Set controls to vehicle.
    void Update(float timeStep) override;

    /// Handle application post-update. Update camera position after vehicle has moved.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle application post-render-update. Renders debug geometry.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    /// The controllable vehicle component.
    WeakPtr<Vehicle2> vehicle_;

    /// Draw debug geometry.
    bool drawDebug_{};
};
