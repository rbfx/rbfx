//
// Copyright (c) 2017-2020 the rbfx project.
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
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    /// Crowd agent.
    CrowdAgent* agent_{};
    /// Yaw angle.
    float yaw_{};
    /// Pitch angle.
    float pitch_{};
    /// Whether the character textures are enabled.
    bool texturesEnabled_{ true };
};
