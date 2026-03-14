//
// Copyright (c) 2008-2022 the Urho3D project.
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
#include <Urho3D/Network/ReplicatedPeer.h>

namespace Urho3D
{

class Button;
class DataChannelConnection;
class DataChannelServer;
class LineEdit;
class NetworkConnection;
class Scene;
class Text;
class UIElement;

}

class ReplicatedDataChannelConnection;

/// Scene network replication example.
/// This sample demonstrates:
///     - Creating a scene in which network clients can join
///     - Giving each client an object to control and sending the controls from the clients to the server
///       where the authoritative simulation happens
///     - Controlling a physics object's movement by applying forces
class SceneReplication : public Sample
{
    URHO3D_OBJECT(SceneReplication, Sample);

public:
    /// Construct.
    explicit SceneReplication(Context* context);

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
    /// Create a text from the UI root.
    Text* CreateOverlayText(int& y);
    /// Update all the values on the info overlay.
    void UpdateOverlay(unsigned packetsIn, unsigned packetsOut, unsigned bytesIn, unsigned bytesOut, unsigned connections);
    /// Create a button to the button container.
    Button* CreateButton(const ea::string& text, int width);
    /// Update visibility of buttons according to connection and server status.
    void UpdateButtons();
    /// Create a controllable ball object and return its scene node.
    Node* CreateControllableObject(SharedPtr<ReplicatedPeer, RefCounted> owner);
    /// Find player object on client side.
    Node* GetPlayerObject();
    /// Read input and move the camera.
    void MoveCamera();
    /// Handle the physics world pre-step event.
    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);
    /// Handle the logic post-update event.
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the connect button.
    void HandleConnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the disconnect button.
    void HandleDisconnect(StringHash eventType, VariantMap& eventData);
    /// Handle pressing the start server button.
    void HandleStartServer(StringHash eventType, VariantMap& eventData);
    /// Handle server-side connection established.
    void HandleServerConnected(NetworkConnection* connection);
    /// Handle server-side connection closed.
    void HandleServerDisconnected(NetworkConnection* connection);

    /// Server-side connections.
    ea::unordered_set<NetworkConnection*> serverConnections_;
    /// Button container element.
    SharedPtr<UIElement> buttonContainer_;
    /// Server address line editor element.
    SharedPtr<LineEdit> textEdit_;
    /// Connect button.
    SharedPtr<Button> connectButton_;
    /// Disconnect button.
    SharedPtr<Button> disconnectButton_;
    /// Start server button.
    SharedPtr<Button> startServerButton_;
    /// Instructions text.
    SharedPtr<Text> instructionsText_;
    /// Packets in per second
    SharedPtr<Text> packetsIn_;
    /// Packets out per second
    SharedPtr<Text> packetsOut_;
    /// Bytes in per second
    SharedPtr<Text> bytesIn_;
    /// Bytes out per second
    SharedPtr<Text> bytesOut_;
    /// Number of connections
    SharedPtr<Text> connections_;
    /// IsServerRunning status
    SharedPtr<Text> serverRunning_;
    /// Local data channel server.
    SharedPtr<DataChannelServer> server_;
    /// Client connection to server.
    SharedPtr<ReplicatedDataChannelConnection> clientConnection_;
    /// Packet counter UI update timer
    Timer packetCounterTimer_;
};
