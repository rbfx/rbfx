// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

namespace Urho3D
{

class Window;

}

/// Sample that demonstrates different texture formats.
class TextureFormatsSample : public Sample
{
    URHO3D_OBJECT(TextureFormatsSample, Sample);

public:
    /// Construct.
    explicit TextureFormatsSample(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();

    /// Create box with texture.
    Node* CreateTexturedBox(Texture2D* texture);
    /// Create text label in 3D.
    Node* CreateLabel(const ea::string& text);
};


