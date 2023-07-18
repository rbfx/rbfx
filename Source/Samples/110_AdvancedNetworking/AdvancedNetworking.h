//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022 the rbfx project.
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
#include "AdvancedNetworkingRaycast.h"

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Input/InputMap.h>

#include <EASTL/optional.h>

namespace Urho3D
{
class NetworkObject;
}

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
    /// Read input and move the camera.
    void MoveCamera();
    /// Update statistics text.
    void UpdateStats();
    /// Perform raycast against important geometries in the scene.
    ea::optional<Vector3> RaycastImportantGeometries(const Ray& ray) const;

    /// Process pending raycasts on the server.
    void ProcessRaycastsOnServer();
    /// Process single raycast on the server.
    void ProcessSingleRaycastOnServer(const ServerRaycastInfo& raycastInfo);
    /// Create a controllable ball object and return its scene node.
    Node* CreateControllableObject(Connection* owner);
    /// Handle a client connecting to the server.
    void HandleClientConnected(StringHash eventType, VariantMap& eventData);
    /// Handle a client disconnecting from the server.
    void HandleClientDisconnected(StringHash eventType, VariantMap& eventData);

    /// Process movement of the client on the client side.
    void ProcessClientMovement(NetworkObject* clientObject);
    /// Return aim position from a screen ray.
    Vector3 GetAimPosition(const Vector3& playerPosition, const Ray& screenRay) const;
    /// Perform a raycast request on the client side.
    void RequestClientRaycast(NetworkObject* clientObject, const Ray& screenRay);
    /// Add debug marker for ray hits.
    void AddHitMarker(const Vector3& position, bool isConfirmed);

    /// UI with client and server settings.
    AdvancedNetworkingUI* ui_{};

    /// Collection of temporary nodes used for hit markers.
    Node* hitMarkers_{};
    /// Mapping from client connections to controllable objects.
    ea::unordered_map<Connection*, WeakPtr<Node> > serverObjects_;
    /// Queue of pending raycast requests on the server.
    ea::vector<ServerRaycastInfo> serverRaycasts_;
    /// Instructions text.
    SharedPtr<Text> instructionsText_;
    /// Input map.
    SharedPtr<InputMap> inputMap_;

    /// Text with statistics
    SharedPtr<Text> statsText_;
    /// Statistics UI update timer
    Timer statsTimer_;

    /// Timer used for auto movement.
    Timer autoMovementTimer_{};
    /// Current phase of auto movement.
    unsigned autoMovementPhase_{};
    /// Timer used for auto clicker.
    Timer autoClickTimer_{};
};
