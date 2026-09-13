// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"
#include "Urho3D/Scene/ShakeComponent.h"

namespace Urho3D
{

class Node;
class Scene;

}

/// Static 3D scene example.
/// This sample demonstrates:
///     - Creating a 3D scene with static content
///     - Displaying the scene using the Renderer subsystem
///     - Handling keyboard and mouse input to move a freelook camera
class CameraShake : public Sample
{
    URHO3D_OBJECT(CameraShake, Sample);

public:
    /// Construct.
    explicit CameraShake(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

    void Update(float timeStep) override;
private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    ShakeComponent* shakeComponent_{};
};
