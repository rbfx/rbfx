// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

#include <Urho3D/UI/Text.h>
#include <Urho3D/XR/VirtualReality.h>

namespace Urho3D
{

class Node;
class Scene;

} // namespace Urho3D

/// Simple VR
/// This sample demonstrates:
///     - Initializing XR
///     - Displaying XR with a StereoRenderPipeline
///     - Simple teleportation locomotion
class HelloVR : public Sample
{
    URHO3D_OBJECT(HelloVR, Sample);

public:
    /// Construct.
    explicit HelloVR(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;
    /// Shutdown OpenXR
    void Stop() override;

private:
    /// Periodically update the scene.
    void Update();
    /// Return VR status string.
    ea::string GetStatus() const;

    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Update hand components.
    void SetupHandComponents(Node* handPoseNode, Node* handAimNode);
    /// Grab dynamic object with hand.
    void GrabDynamicObject(Node* handNode, VRHand hand);
    /// Release dynamic object from hand.
    void ReleaseDynamicObject(Node* handNode);

    /// Container of all interactive dynamic objects
    SharedPtr<Node> dynamicObjects_;
    /// Text element displayed on the flat screen.
    SharedPtr<Text> statusText_;
};
