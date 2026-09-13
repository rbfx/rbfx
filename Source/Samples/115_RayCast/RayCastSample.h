// Copyright (c) 2023-20233 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/OctreeQuery.h>
#include <Urho3D/UI/DropDownList.h>

namespace Urho3D
{

class Node;
class Scene;

}

/// Ray cast sample.
/// This sample demonstrates how to run a ray cast and what results it produces.
class RayCastSample : public Sample
{
    URHO3D_OBJECT(RayCastSample, Sample)

public:
    /// Construct.
    explicit RayCastSample(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

    /// Handle frame update
    void Update(float timeStep) override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    void PlaceHitMarker(const Vector3& position, const Vector3& normal);
    void RemoveHitMarker();
    void PhysicalRayCast(const Ray& ray);
    void DrawableRayCast(const Ray& ray, RayQueryLevel level);

    /// Hit marker.
    SharedPtr<Node> hitMarkerNode_;
    SharedPtr<StaticModel> hitMarker_;
    bool isVisible_{};

    /// Drop down selection of ray cast type.
    SharedPtr<DropDownList> typeOfRayCast_;
};
