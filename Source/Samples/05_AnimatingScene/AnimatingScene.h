// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

namespace Urho3D
{

class Node;
class Scene;

}

/// Animating 3D scene example.
/// This sample demonstrates:
///     - Creating a 3D scene and using a custom component to animate the objects
///     - Controlling scene ambience with the Zone component
///     - Attaching a light to an object (the camera)
class AnimatingScene : public Sample
{
    URHO3D_OBJECT(AnimatingScene, Sample);

public:
    /// Construct.
    explicit AnimatingScene(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
};
