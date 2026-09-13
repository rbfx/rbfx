// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

namespace Urho3D
{

class Node;
class Scene;
class CrowdAgent;

}

/// Baked lighting example.
class BakedLighting : public Sample
{
    URHO3D_OBJECT(BakedLighting, Sample);

public:
    /// Construct.
    explicit BakedLighting(Context* context);
    /// Destruct.
    ~BakedLighting() override;

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Create scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Subscribe to necessary events.
    void SubscribeToEvents();
    /// Handle application update. Set controls to character.
    void Update(float timeStep) override;

    /// Crowd agent.
    CrowdAgent* agent_{};
    /// Yaw angle.
    float yaw_{};
    /// Pitch angle.
    float pitch_{};
    /// Whether the character textures are enabled.
    bool texturesEnabled_{ true };
};
