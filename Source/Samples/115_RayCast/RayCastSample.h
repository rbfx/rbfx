//
// Copyright (c) 2023-20233 the rbfx project.
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
