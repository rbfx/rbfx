// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"
#include <Urho3D/Input/PointerAdapter.h>
#include <Urho3D/Graphics/OutlineGroup.h>

namespace Urho3D
{

class Node;
class Scene;

}

/// Pointer adapter sample.
/// This sample demonstrates how to control cursor on various platfroms:
/// - On PC with a mouse you can click on the cubes
/// - On mobile platforms you can touch the cubes
/// - On consoles you can move the cursor with gamepad
class PointerAdapterSample : public Sample
{
    URHO3D_OBJECT(PointerAdapterSample, Sample)

public:
    /// Construct.
    explicit PointerAdapterSample(Context* context);

    void Start() override;
    void Stop() override;

private:
    void HandleMouseMove(VariantMap& args);
    void HandleMouseButtonUp(VariantMap& args);
    void HandleMouseButtonDown(VariantMap& args);

    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();

    PointerAdapter pointerAdapter_;
    /// Octree.
    SharedPtr<Octree> octree_;
    /// Outline group.
    SharedPtr<OutlineGroup> outlineGroup_;
};
