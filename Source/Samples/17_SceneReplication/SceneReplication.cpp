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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Replica/BehaviorNetworkObject.h>
#include <Urho3D/Replica/ClientReplica.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/LineEdit.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "SceneReplication.h"

#include <Urho3D/DebugNew.h>

// UDP port we will use
static const unsigned short SERVER_PORT = 2345;
// Identifier for the node ID parameter in the event data
static const StringHash P_ID("ID");

// Control bits we define
static const unsigned CTRL_FORWARD = 1;
static const unsigned CTRL_BACK = 2;
static const unsigned CTRL_LEFT = 4;
static const unsigned CTRL_RIGHT = 8;

/// Controls data.
struct PlayerControls
{
    float yaw_{};
    unsigned buttons_{};
};

/// Simple controller that implements sample networking logic:
/// - Synchronize light color on setup;
/// - Deliver client input to server.
class SceneReplicationPlayer : public NetworkBehavior
{
    URHO3D_OBJECT(SceneReplicationPlayer, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::UnreliableFeedback;

    explicit SceneReplicationPlayer(Context* context)
        : NetworkBehavior(context, CallbackMask)
    {
    }

    /// Set current controls on client side.
    void SetControls(const PlayerControls& controls) { controls_ = controls; }

    /// Return latest received controls on server side.
    const PlayerControls& GetControls() const { return controls_; }

    /// Write object color on the server.
    void WriteSnapshot(NetworkFrame frame, Serializer& dest) override
    {
        auto* light = GetComponent<Light>();
        dest.WriteColor(light->GetColor());
    }

    /// Read object color on the client.
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override
    {
        auto* light = GetComponent<Light>();
        light->SetColor(src.ReadColor());
    }

    /// Always send controls.
    bool PrepareUnreliableFeedback(NetworkFrame frame) override { return true; }

    /// Write controls on the client.
    void WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest) override
    {
        dest.WriteFloat(controls_.yaw_);
        dest.WriteVLE(controls_.buttons_);
    }

    /// Read controls on the server.
    void ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src) override
    {
        // Skip outdated controls
        if (lastFeedbackFrame_ && (*lastFeedbackFrame_ >= feedbackFrame))
            return;

        controls_.yaw_ = src.ReadFloat();
        controls_.buttons_ = src.ReadVLE();
        lastFeedbackFrame_ = feedbackFrame;
    }

private:
    /// Most recent player controls.
    PlayerControls controls_;
    /// Time when latest player controls were received.
    ea::optional<NetworkFrame> lastFeedbackFrame_;
};

SceneReplication::SceneReplication(Context* context) :
    Sample(context)
{
}

void SceneReplication::Start(const ea::vector<ea::string>& args)
{
    // Register sample types
    if (!context_->IsReflected<SceneReplicationPlayer>())
        context_->AddFactoryReflection<SceneReplicationPlayer>();

    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateUI();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to necessary events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    SetMouseMode(MM_RELATIVE);
    SetMouseVisible(false);

    // Process command line
    if (args.size() >= 2)
    {
        if (args[1] == "StartServer")
            HandleStartServer({}, GetEventDataMap());
        else if (args[1] == "Connect")
            HandleConnect({}, GetEventDataMap());
    }
}

void SceneReplication::CreateScene()
{
    scene_ = new Scene(context_);

    auto* cache = GetSubsystem<ResourceCache>();

    // Create octree and physics world with default settings. Create them as local so that they are not needlessly replicated
    // when a client connects
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();
    scene_->CreateComponent<ReplicationManager>();

    // All static scene content and the camera are also created as local, so that they are unaffected by scene replication and are
    // not removed from the client upon connection. Create a Zone component first for ambient lighting & fog control.
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light without shadows
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.5f, -1.0f, 0.5f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color(0.2f, 0.2f, 0.2f));
    light->SetSpecularIntensity(1.0f);

    // Create a "floor" consisting of several tiles. Make the tiles physical but leave small cracks between them
    for (int y = -20; y <= 20; ++y)
    {
        for (int x = -20; x <= 20; ++x)
        {
            Node* floorNode = scene_->CreateChild("FloorTile");
            floorNode->SetPosition(Vector3(x * 20.2f, -0.5f, y * 20.2f));
            floorNode->SetScale(Vector3(20.0f, 1.0f, 20.0f));
            auto* floorObject = floorNode->CreateComponent<StaticModel>();
            floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
            floorObject->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

            auto* body = floorNode->CreateComponent<RigidBody>();
            body->SetFriction(1.0f);
            auto* shape = floorNode->CreateComponent<CollisionShape>();
            shape->SetBox(Vector3::ONE);
        }
    }

    // Create the camera. Limit far clip distance to match the fog
    // The camera needs to be created into a local node so that each client can retain its own camera, that is unaffected by
    // network messages. Furthermore, because the client removes all replicated scene nodes when connecting to a server scene,
    // the screen would become blank if the camera node was replicated (as only the locally created camera is assigned to a
    // viewport in SetupViewports() below)
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void SceneReplication::CreateUI()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();
    UIElement* root = GetUIRoot();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it can interact with the login UI
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto(uiStyle);
    SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    auto* graphics = GetSubsystem<Graphics>();
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

    // Construct the instructions text element
    instructionsText_ = GetUIRoot()->CreateChild<Text>();
    instructionsText_->SetText(
        "Use WASD keys to move and RMB to rotate view"
    );
    instructionsText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // Position the text relative to the screen center
    instructionsText_->SetHorizontalAlignment(HA_CENTER);
    instructionsText_->SetVerticalAlignment(VA_CENTER);
    instructionsText_->SetPosition(0, graphics->GetHeight() / 4);
    // Hide until connected
    instructionsText_->SetVisible(false);

    packetsIn_ = GetUIRoot()->CreateChild<Text>();
    packetsIn_->SetText("Packets in : 0");
    packetsIn_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    packetsIn_->SetHorizontalAlignment(HA_LEFT);
    packetsIn_->SetVerticalAlignment(VA_CENTER);
    packetsIn_->SetPosition(10, -10);

    packetsOut_ = GetUIRoot()->CreateChild<Text>();
    packetsOut_->SetText("Packets out: 0");
    packetsOut_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    packetsOut_->SetHorizontalAlignment(HA_LEFT);
    packetsOut_->SetVerticalAlignment(VA_CENTER);
    packetsOut_->SetPosition(10, 10);

    buttonContainer_ = root->CreateChild<UIElement>();
    buttonContainer_->SetFixedSize(500, 20);
    buttonContainer_->SetPosition(20, 20);
    buttonContainer_->SetLayoutMode(LM_HORIZONTAL);

    textEdit_ = buttonContainer_->CreateChild<LineEdit>();
    textEdit_->SetStyleAuto();

    connectButton_ = CreateButton("Connect", 90);
    disconnectButton_ = CreateButton("Disconnect", 100);
    startServerButton_ = CreateButton("Start Server", 110);

    UpdateButtons();
}

void SceneReplication::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}

void SceneReplication::SubscribeToEvents()
{
    // Subscribe to fixed timestep physics updates for setting or applying controls
    SubscribeToEvent(E_PHYSICSPRESTEP, URHO3D_HANDLER(SceneReplication, HandlePhysicsPreStep));

    // Subscribe HandlePostUpdate() method for processing update events. Subscribe to PostUpdate instead
    // of the usual Update so that physics simulation has already proceeded for the frame, and can
    // accurately follow the object with the camera
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(SceneReplication, HandlePostUpdate));

    // Subscribe to button actions
    SubscribeToEvent(connectButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleConnect));
    SubscribeToEvent(disconnectButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleDisconnect));
    SubscribeToEvent(startServerButton_, E_RELEASED, URHO3D_HANDLER(SceneReplication, HandleStartServer));

    // Subscribe to network events
    SubscribeToEvent(E_SERVERCONNECTED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_SERVERDISCONNECTED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_CONNECTFAILED, URHO3D_HANDLER(SceneReplication, HandleConnectionStatus));
    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(SceneReplication, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(SceneReplication, HandleClientDisconnected));
}

Button* SceneReplication::CreateButton(const ea::string& text, int width)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");

    auto* button = buttonContainer_->CreateChild<Button>();
    button->SetStyleAuto();
    button->SetFixedWidth(width);

    auto* buttonText = button->CreateChild<Text>();
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

void SceneReplication::UpdateButtons()
{
    auto* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();
    bool serverRunning = network->IsServerRunning();

    // Show and hide buttons so that eg. Connect and Disconnect are never shown at the same time
    connectButton_->SetVisible(!serverConnection && !serverRunning);
    disconnectButton_->SetVisible(serverConnection || serverRunning);
    startServerButton_->SetVisible(!serverConnection && !serverRunning);
    textEdit_->SetVisible(!serverConnection && !serverRunning);
}

Node* SceneReplication::CreateControllableObject(Connection* owner)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto prefab = cache->GetResource<PrefabResource>("Prefabs/SceneReplicationPlayer.prefab");

    // Instantiate common components from prefab so they will be replicated on the client.
    const Vector3 position{Vector3(Random(40.0f) - 20.0f, 5.0f, Random(40.0f) - 20.0f)};
    Node* playerNode = scene_->InstantiatePrefab(prefab->GetNodePrefab(), position, Quaternion::IDENTITY);
    playerNode->SetName("Ball");

    // NetworkObject should never be a part of client prefab
    auto networkObject = playerNode->CreateComponent<BehaviorNetworkObject>();
    networkObject->SetClientPrefab(prefab);
    networkObject->SetOwner(owner);

    // Create the physics components on server only
    auto* body = playerNode->CreateComponent<RigidBody>();
    body->SetMass(1.0f);
    body->SetFriction(1.0f);
    // In addition to friction, use motion damping so that the ball can not accelerate limitlessly
    body->SetLinearDamping(0.5f);
    body->SetAngularDamping(0.5f);
    auto* shape = playerNode->CreateComponent<CollisionShape>();
    shape->SetSphere(1.0f);

    // Assign a random color to the point light at the ball
    auto* light = playerNode->GetComponent<Light>();
    light->SetColor(
        Color(0.5f + ((unsigned)Rand() & 1u) * 0.5f, 0.5f + ((unsigned)Rand() & 1u) * 0.5f, 0.5f + ((unsigned)Rand() & 1u) * 0.5f));

    return playerNode;
}

Node* SceneReplication::GetPlayerObject()
{
    if (auto* replicationManager = scene_->GetComponent<ReplicationManager>())
    {
        if (ClientReplica* clientReplica = replicationManager->GetClientReplica())
        {
            if (NetworkObject* networkObject = clientReplica->GetOwnedNetworkObject())
                return networkObject->GetNode();
        }
    }
    return nullptr;
}

void SceneReplication::MoveCamera()
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    auto* ui = GetSubsystem<UI>();
    auto* input = GetSubsystem<Input>();
    ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));

    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch and only move the camera
    // when the cursor is hidden
    if (!ui->GetCursor()->IsVisible())
    {
        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, 1.0f, 90.0f);
    }

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    // Only move the camera / show instructions if we have a controllable object
    bool showInstructions = false;
    if (Node* playerNode = GetPlayerObject())
    {
        const float CAMERA_DISTANCE = 5.0f;

        // Move camera some distance away from the ball
        cameraNode_->SetPosition(playerNode->GetPosition() + cameraNode_->GetRotation() * Vector3::BACK * CAMERA_DISTANCE);
        showInstructions = true;
    }

    instructionsText_->SetVisible(showInstructions);
}

void SceneReplication::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // We only rotate the camera according to mouse movement since last frame, so do not need the time step
    MoveCamera();

    if (packetCounterTimer_.GetMSec(false) > 1000 && GetSubsystem<Network>()->GetServerConnection())
    {
        packetsIn_->SetText("Packets  in: " + ea::to_string(GetSubsystem<Network>()->GetServerConnection()->GetPacketsInPerSec()));
        packetsOut_->SetText("Packets out: " + ea::to_string(GetSubsystem<Network>()->GetServerConnection()->GetPacketsOutPerSec()));
        packetCounterTimer_.Reset();
    }
    if (packetCounterTimer_.GetMSec(false) > 1000 && GetSubsystem<Network>()->GetClientConnections().size())
    {
        int packetsIn = 0;
        int packetsOut = 0;
        auto connections = GetSubsystem<Network>()->GetClientConnections();
        for (auto it = connections.begin(); it != connections.end(); ++it ) {
            packetsIn += (*it)->GetPacketsInPerSec();
            packetsOut += (*it)->GetPacketsOutPerSec();
        }
        packetsIn_->SetText("Packets  in: " + ea::to_string(packetsIn));
        packetsOut_->SetText("Packets out: " + ea::to_string(packetsOut));
        packetCounterTimer_.Reset();
    }
}

void SceneReplication::HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData)
{
    // This function is different on the client and server. The client collects controls (WASD controls + yaw angle)
    // and sets them to its server connection object, so that they will be sent to the server automatically at a
    // fixed rate, by default 30 FPS. The server will actually apply the controls (authoritative simulation.)
    auto* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();

    // Client: collect controls
    if (serverConnection)
    {
        auto* ui = GetSubsystem<UI>();
        auto* input = GetSubsystem<Input>();
        if (Node* playerNode = GetPlayerObject())
        {
            PlayerControls controls;

            // Copy mouse yaw
            controls.yaw_ = yaw_;

            // Only apply WASD controls if there is no focused UI element
            if (!ui->GetFocusElement())
            {
                controls.buttons_ |= input->GetKeyDown(KEY_W) ? CTRL_FORWARD : 0;
                controls.buttons_ |= input->GetKeyDown(KEY_S) ? CTRL_BACK : 0;
                controls.buttons_ |= input->GetKeyDown(KEY_A) ? CTRL_LEFT : 0;
                controls.buttons_ |= input->GetKeyDown(KEY_D) ? CTRL_RIGHT : 0;
            }

            auto* player = playerNode->GetComponent<SceneReplicationPlayer>();
            player->SetControls(controls);
        }
    }
    // Server: apply controls to client objects
    else if (network->IsServerRunning())
    {
        const ea::vector<SharedPtr<Connection> >& connections = network->GetClientConnections();

        for (unsigned i = 0; i < connections.size(); ++i)
        {
            Connection* connection = connections[i];
            // Get the object this connection is controlling
            Node* playerNode = serverObjects_[connection];
            if (!playerNode)
                continue;

            auto* body = playerNode->GetComponent<RigidBody>();
            auto* player = playerNode->GetComponent<SceneReplicationPlayer>();

            // Get the last controls sent by the client
            const PlayerControls& controls = player->GetControls();
            // Torque is relative to the forward vector
            Quaternion rotation(0.0f, controls.yaw_, 0.0f);

            const float MOVE_TORQUE = 3.0f;

            // Movement torque is applied before each simulation step, which happen at 60 FPS. This makes the simulation
            // independent from rendering framerate. We could also apply forces (which would enable in-air control),
            // but want to emphasize that it's a ball which should only control its motion by rolling along the ground
            if (controls.buttons_ & CTRL_FORWARD)
                body->ApplyTorque(rotation * Vector3::RIGHT * MOVE_TORQUE);
            if (controls.buttons_ & CTRL_BACK)
                body->ApplyTorque(rotation * Vector3::LEFT * MOVE_TORQUE);
            if (controls.buttons_ & CTRL_LEFT)
                body->ApplyTorque(rotation * Vector3::FORWARD * MOVE_TORQUE);
            if (controls.buttons_ & CTRL_RIGHT)
                body->ApplyTorque(rotation * Vector3::BACK * MOVE_TORQUE);
        }
    }
}

void SceneReplication::HandleConnect(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    ea::string address = textEdit_->GetText();
    address.trim();
    if (address.empty())
        address = "localhost"; // Use localhost to connect if nothing else specified

    // Connect to server, specify scene to use as a client for replication
    network->Connect(address, SERVER_PORT, scene_);

    UpdateButtons();
}

void SceneReplication::HandleDisconnect(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();
    // If we were connected to server, disconnect. Or if we were running a server, stop it. In both cases clear the
    // scene of all replicated content, but let the local nodes & components (the static world + camera) stay
    if (serverConnection)
    {
        serverConnection->Disconnect();
    }
    // Or if we were running a server, stop it
    else if (network->IsServerRunning())
    {
        network->StopServer();
    }

    UpdateButtons();
}

void SceneReplication::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
    auto* network = GetSubsystem<Network>();
    network->StartServer(SERVER_PORT);

    UpdateButtons();
}

void SceneReplication::HandleConnectionStatus(StringHash eventType, VariantMap& eventData)
{
    UpdateButtons();
}

void SceneReplication::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client connects, assign to scene to begin scene replication
    auto* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    newConnection->SetScene(scene_);

    // Then create a controllable object for that client
    Node* newObject = CreateControllableObject(newConnection);
    serverObjects_[newConnection] = newObject;
}

void SceneReplication::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client disconnects, remove the controlled object
    auto* connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    Node* object = serverObjects_[connection];
    if (object)
        object->Remove();

    serverObjects_.erase(connection);
}
