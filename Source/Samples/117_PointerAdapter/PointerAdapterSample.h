//
// Copyright (c) 2023-20233 the rbfx project.
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
