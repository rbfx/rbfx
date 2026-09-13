// Copyright (c) 2008-2017 the Urho3D project.
// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

#include <Urho3D/SystemUI/SystemMessageBox.h>

/// This example demonstrates creation and use of debug UIs using ImGui. Also it demonstrates Console and system message
/// box usage.
class HelloSystemUi : public Sample
{
    URHO3D_OBJECT(HelloSystemUi, Sample);

public:
    /// Construct.
    HelloSystemUi(Context* context);

    /// Setup after engine initialization and before running the main loop.
    virtual void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    virtual ea::string GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Assemble debug UI and handle UI events.
    void RenderUi(StringHash eventType, VariantMap& eventData);
    /// Process key events like opening a console window.
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    /// Creates a scene. Only required to provide background color that is not black.
    void CreateScene();

    /// Reference holding message box.
    SharedPtr<SystemMessageBox> messageBox_;
    /// Box node.
    SharedPtr<Node> boxNode_;
    /// Flag controlling display of imgui demo window.
    bool metricsOpen_ = false;
};
