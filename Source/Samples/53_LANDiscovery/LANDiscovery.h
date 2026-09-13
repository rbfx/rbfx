// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

namespace Urho3D
{

class Button;
class LineEdit;
class Text;
class UIElement;
class LANDiscoveryManager;

}

/// Chat example
/// This sample demonstrates:
///     - Starting up a network server or connecting to it
///     - Implementing simple chat functionality with network messages
class LANDiscovery : public Sample
{
    URHO3D_OBJECT(LANDiscovery, Sample);

public:
    /// Construct.
    explicit LANDiscovery(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    ea::string GetScreenJoystickPatchString() const override { return
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
    struct ServerListEntry
    {
        ea::string name_;
        int players_;
        ea::string address_;
        unsigned short port_;
        unsigned lastSeen_;
    };

    /// Create the UI.
    void CreateUI();
    /// Subscribe to log message, UI and network events.
    void SubscribeToEvents();
    /// Create a button to the button container.
    Button* CreateButton(const ea::string& text, int width, IntVector2 position);
    /// Create label
    Text* CreateLabel(const ea::string& text, IntVector2 pos);
    /// Update server list UI.
    void FormatServerListUI();

    /// Handle found LAN server
    void HandleNetworkHostDiscovered(StringHash eventType, VariantMap& eventData);
    /// Start server
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Stop server
    void HandleStopServer(StringHash eventType, VariantMap& eventData);
    /// Start network discovery
    void HandleDoNetworkDiscovery(StringHash eventType, VariantMap& eventData);
    /// Expire servers that did not reannouce themselves
    void HandleExpireServers(StringHash eventType, VariantMap& eventData);
    /// Start server
    SharedPtr<Button> startServer_;
    /// Stop server
    SharedPtr<Button> stopServer_;
    /// Redo LAN discovery
    SharedPtr<Button> refreshServerList_;
    /// Found server list
    SharedPtr<Text> serverList_;
    /// List of currently active servers
    ea::map<ea::string, ServerListEntry> serverListItems_;
    /// LAN discovery manager
    SharedPtr<LANDiscoveryManager> lanDiscovery_;
};
