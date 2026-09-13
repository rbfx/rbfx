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

/// Dynamic geometry example.
/// This sample demonstrates:
///     - Cloning a Model resource
///     - Modifying the vertex buffer data of the cloned models at runtime to efficiently animate them
///     - Creating a Model resource and its buffer data from scratch
class DynamicGeometry : public Sample
{
    URHO3D_OBJECT(DynamicGeometry, Sample);

public:
    /// Construct.
    explicit DynamicGeometry(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    ea::string GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Animation</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"SPACE\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();

    /// Animate the vertex data of the objects.
    void AnimateObjects(float timeStep);
    /// Handle the logic update event.
    void Update(float timeStep) override;

    /// Cloned models' vertex buffers that we will animate.
    ea::vector<SharedPtr<VertexBuffer> > animatingBuffers_;
    /// Original vertex positions for the sphere model.
    ea::vector<Vector3> originalVertices_;
    /// If the vertices are duplicates, indices to the original vertices (to allow seamless animation.)
    ea::vector<unsigned> vertexDuplicates_;
    /// Animation flag.
    bool animate_;
    /// Animation's elapsed time.
    float time_;
};
