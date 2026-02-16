// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

namespace Urho3D
{
class FreeFlyController;
class Text;
}

class AdvancedNetworkingClientConnection;
class AdvancedNetworkingServer;

class AdvancedNetworkingUI;

/// Scene network replication example.
/// This sample demonstrates:
///     - Creating a scene in which network clients can join
///     - Giving each client an object to control and sending the controls from the clients to the server
///       where the authoritative simulation happens
///     - Controlling a physics object's movement by applying forces
class AdvancedNetworking : public Sample
{
    URHO3D_OBJECT(AdvancedNetworking, Sample)

public:
    /// Construct.
    explicit AdvancedNetworking(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start(const ea::vector<ea::string>& args) override;

    /// Stop the server and disconnect clients.
    void Stop() override;

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
    /// Construct the scene content.
    void CreateScene();
    /// Construct instruction text and the login / start server UI.
    void CreateUI();
    /// Set up viewport.
    void SetupViewport();
    /// Subscribe to update, UI and network events.
    void SubscribeToEvents();
    /// Update statistics text.
    void UpdateStats();
    /// Start local server.
    void StartServer(unsigned short port);
    /// Connect local client to server.
    void ConnectToServer(const ea::string& address, unsigned short port);
    /// Stop local networking.
    void StopNetworking();
    /// Return whether local server is running.
    bool IsServerRunning() const;
    /// Return whether local client is connected.
    bool IsClientConnected() const;
    /// Handle client-side connection state.
    void HandleClientConnectionState(bool connected);

    /// UI with client and server settings.
    AdvancedNetworkingUI* ui_{};

    /// Collection of temporary nodes used for hit markers.
    Node* hitMarkers_{};
    /// Free-fly camera controller used in server-only mode.
    FreeFlyController* freeFlyController_{};
    /// Instructions text.
    SharedPtr<Text> instructionsText_;

    /// Local network server wrapper.
    SharedPtr<AdvancedNetworkingServer> server_;
    /// Client connection to server.
    SharedPtr<AdvancedNetworkingClientConnection> clientConnection_;

    /// Text with statistics
    SharedPtr<Text> statsText_;
    /// Statistics UI update timer
    Timer statsTimer_;

};
