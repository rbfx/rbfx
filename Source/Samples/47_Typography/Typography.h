// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/UI/CheckBox.h>
#include <Urho3D/UI/DropDownList.h>
#include "Sample.h"

/// Text rendering example.
/// Displays text at various sizes, with checkboxes to change the rendering parameters.
class Typography : public Sample
{
    URHO3D_OBJECT(Typography, Sample);

public:
    /// Construct.
    explicit Typography(Context* context);

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
    SharedPtr<UIElement> uielement_;

    void CreateText();
    template <class T> SharedPtr<CheckBox> CreateCheckbox(const ea::string& label, T handler);
    template <class T> SharedPtr<DropDownList> CreateMenu(const ea::string& label, const char** items, T handler);

    void HandleWhiteBackground(StringHash eventType, VariantMap& eventData);
    void HandleForceAutoHint(StringHash eventType, VariantMap& eventData);
    void HandleFontHintLevel(StringHash eventType, VariantMap& eventData);
    void HandleFontSubpixel(StringHash eventType, VariantMap& eventData);
    void HandleFontOversampling(StringHash eventType, VariantMap& eventData);
};
