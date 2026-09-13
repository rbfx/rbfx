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
class RibbonTrail;
class AnimationController;

}

/// Ribbon trail demo.
/// This sample demonstrates how to use both trail types of RibbonTrail component.
class RibbonTrailDemo : public Sample
{
    URHO3D_OBJECT(RibbonTrailDemo, Sample);

public:
    /// Construct.
    explicit RibbonTrailDemo(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Trail that emitted from sword.
    SharedPtr<RibbonTrail> swordTrail_;
    /// Animation controller of the ninja.
    SharedPtr<AnimationController> ninjaAnimCtrl_;
    /// The time sword start emitting trail.
    float swordTrailStartTime_;
    /// The time sword stop emitting trail.
    float swordTrailEndTime_;
    /// Box node 1.
    SharedPtr<Node> boxNode1_;
    /// Box node 2.
    SharedPtr<Node> boxNode2_;
    /// Sum of timestep.
    float timeStepSum_;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Handle the logic update event.
    void Update(float timeStep) override;
};
