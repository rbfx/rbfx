// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

#include <Urho3D/IO/FileIdentifier.h>

/// This first example, maintaining tradition, prints a "Hello World" message.
/// Furthermore it shows:
///     - Using the Sample / Application classes, which initialize the Urho3D engine and run the main loop
///     - Adding a Text element to the graphical user interface
///     - Subscribing to and handling of update events
class VFSSample : public Sample
{
    URHO3D_OBJECT(VFSSample, Sample);

public:
    /// Construct.
    explicit VFSSample(Context* context);

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
    /// Construct a new Text instance, containing the 'Hello World' String, and add it to the UI root element.
    void CreateText();
    /// Render interactive UI for testing.
    void RenderUi();

    /// Input URI string.
    ea::string uri_{"Models/Box.mdl"};

    /// Parsed URI.
    FileIdentifier fileIdentifier_;
    /// Canonical form of URI.
    FileIdentifier canonicalForm_;
    /// Whether the file exists.
    bool exists_{};
    /// Absolute path to the file.
    ea::string absoluteFileName_;
    /// File opened for reading.
    AbstractFilePtr readOnlyFile_;
    /// File modification time.
    FileTime modificationTime_{};
    /// URI reversed from the file name.
    ea::string reversedUri_;

    /// Scan path and scheme.
    FileIdentifier scanPath_{"", "Materials"};
    /// Scan filter
    ea::string scanFilter_{"*.*"};
    /// Whether to scan recursively.
    bool scanRecursive_{true};
    /// Whether to scan for files.
    bool scanFiles_{true};
    /// Whether to scan for directories.
    bool scanDirectories_{false};
    /// Scan results.
    ea::vector<ea::string> scanResults_;
};
