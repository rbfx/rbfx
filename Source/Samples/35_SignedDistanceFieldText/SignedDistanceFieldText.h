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

/// Signed distance field text example.
/// This sample demonstrates:
///     - Creating a 3D scene with static content
///     - Creating a 3D text use SDF Font
///     - Displaying the scene using the Renderer subsystem
///     - Handling keyboard and mouse input to move a freelook camera
class SignedDistanceFieldText : public Sample
{
    URHO3D_OBJECT(SignedDistanceFieldText, Sample);

public:
    /// Construct.
    explicit SignedDistanceFieldText(Context* context);

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
