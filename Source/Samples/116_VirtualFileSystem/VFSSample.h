//
// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2023 the rbfx project.
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
