// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

namespace Urho3D
{

class Drawable;
class Node;
class Scene;
class StaticModel;
class Zone;

}

/// Scene rendering showcase
class RenderingShowcase : public Sample
{
    URHO3D_OBJECT(RenderingShowcase, Sample);

public:
    /// Construct.
    explicit RenderingShowcase(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Setup shared scene objects.
    void CreateScene();
    /// Setup currently selected scene.
    void SetupSelectedScene(bool resetCamera = true);
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Handle the logic update event.
    void Update(float timeStep) override;
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    ea::string GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Next Mode</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"Q\" />"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Next Scene</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"TAB\" />"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button2']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button2']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Toggle Object</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button2']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"F\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }

    /// Scene that owns camera.
    SharedPtr<Scene> cameraScene_;
    /// Probe object.
    SharedPtr<StaticModel> probeObject_;
    /// Index of currently rendered scene, i.e. outer index of sceneNames_.
    unsigned sceneIndex_{};
    /// Index of current scene rendering mode, i.e. inner index of sceneNames_.
    unsigned sceneMode_{};
    /// Index of probe object material. 0 corresponds to the disabled probe object.
    unsigned probeMaterialIndex_{};
    /// List of all available scenes.
    ea::vector<ea::vector<ea::string>> sceneNames_;
};
