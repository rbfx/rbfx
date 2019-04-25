//
// Copyright (c) 2008-2019 the Urho3D project.
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

class Button;
class LineEdit;
class Text;
class UIElement;

}

const int SERVER_PORT = 54654;

/// Chat example
/// This sample demonstrates:
///     - Starting up a network server or connecting to it
///     - Implementing simple chat functionality with network messages
class NATPunchtrough : public Sample
{
    URHO3D_OBJECT(NATPunchtrough, Sample);

public:
    /// Construct.
    explicit NATPunchtrough(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    stl::string GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button2']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Create the UI.
    void CreateUI();
    /// Subscribe to log message, UI and network events.
    void SubscribeToEvents();
    /// Create a button to the button container.
    Button* CreateButton(const stl::string& text, int width, IntVector2 position);
    /// Create label
    void CreateLabel(const stl::string& text, IntVector2 pos);
    /// Create input field
    LineEdit* CreateLineEdit(const stl::string& placeholder, int width, IntVector2 pos);
    /// Print log message.
    void ShowLogMessage(const stl::string& row);
    /// Save NAT server config
    void HandleSaveNatSettings(StringHash eventType, VariantMap& eventData);
    /// Handle server connection message
    void HandleServerConnected(StringHash eventType, VariantMap& eventData);
    /// Handle server disconnect message
    void HandleServerDisconnected(StringHash eventType, VariantMap& eventData);
    /// Handle failed connection
    void HandleConnectFailed(StringHash eventType, VariantMap& eventData);
    /// Start server
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Attempt connecting using NAT punchtrough
    void HandleConnect(StringHash eventType, VariantMap& eventData);
    /// Handle NAT master server failed connection
    void HandleNatConnectionFailed(StringHash eventType, VariantMap& eventData);
    /// Handle NAT master server succesfull connection
    void HandleNatConnectionSucceeded(StringHash eventType, VariantMap& eventData);
    /// Handle NAT punchtrough success message
    void HandleNatPunchtroughSucceeded(StringHash eventType, VariantMap& eventData);
    /// Handle failed NAT punchtrough message
    void HandleNatPunchtroughFailed(StringHash eventType, VariantMap& eventData);
    /// Handle client connecting
    void HandleClientConnected(StringHash eventType, VariantMap& eventData);
    /// Handle client disconnecting
    void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);

    /// NAT master server address
    stl::shared_ptr<LineEdit> natServerAddress_;
    /// NAT master server port
    stl::shared_ptr<LineEdit> natServerPort_;
    /// Save NAT settings button
    stl::shared_ptr<Button> saveNatSettingsButton_;
    /// Start server button
    stl::shared_ptr<Button> startServerButton_;
    /// Remote server GUID input field
    stl::shared_ptr<LineEdit> serverGuid_;
    /// Connect button
    stl::shared_ptr<Button> connectButton_;
    /// Log history text element
    stl::shared_ptr<Text> logHistoryText_;
    /// Log messages
    stl::vector<stl::string> logHistory_;
    /// Created server GUID field
    stl::shared_ptr<LineEdit> guid_;
};
