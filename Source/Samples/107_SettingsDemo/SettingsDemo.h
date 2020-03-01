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

class Window;
class DropDownList;
class CheckBox;

}

/// A simple 'HelloWorld' GUI created purely from code.
/// This sample demonstrates:
///     - Creation of controls and building a UI hierarchy
///     - Loading UI style from XML and applying it to controls
///     - Handling of global and per-control events
/// For more advanced users (beginners can skip this section):
///     - Dragging UIElements
///     - Displaying tooltips
///     - Accessing available Events data (eventData)
class SettingsDemo : public Sample
{
    URHO3D_OBJECT(SettingsDemo, Sample);

public:
    /// Construct.
    explicit SettingsDemo(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    ea::string GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Create window with settings.
    void InitSettings();
    /// Synchronize settings with current state of the engine.
    void SynchronizeSettings();

    /// The Window.
    Window* window_{};
    /// The UI's root UIElement.
    UIElement* uiRoot_{};

    /// Monitor control.
    DropDownList* monitorControl_{};
    /// Resolution control.
    DropDownList* resolutionControl_{};
    /// Fullscreen control.
    CheckBox* fullscreenControl_{};
    /// Borderless flag control.
    CheckBox* borderlessControl_{};
    /// Resizable flag control.
    CheckBox* resizableControl_{};
};


