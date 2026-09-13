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
class CameraOperator;

} // namespace Urho3D

/// Camera operator sample.
/// This sample demonstrates:
///     - How camera operator tracks objects in space to keep them in camera view
///     - How camera operator parameters work
class CameraOperatorSample : public Sample
{
    URHO3D_OBJECT(CameraOperatorSample, Sample);

public:
    /// Construct.
    explicit CameraOperatorSample(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

    void Update(float timeStep) override;

    void TrackCubeAToggled(VariantMap& args);
    void TrackCubeBToggled(VariantMap& args);
    void OrthographicToggled(VariantMap& args);

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();

    CameraOperator* cameraOperator_{};
    Node* cubeA_{};
    Node* cubeB_{};
    float angle_{};
};
