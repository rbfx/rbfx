//
// Copyright (c) 2008-2020 the Urho3D project.
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
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Math/RandomEngine.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/KinematicCharacterController.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Replica/PredictedKinematicController.h>
#include <Urho3D/Replica/ReplicatedTransform.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "AdvancedNetworking.h"

#include <Urho3D/DebugNew.h>

class AdvancedNetworkingPlayer : public NetworkBehavior
{
    URHO3D_OBJECT(AdvancedNetworkingPlayer, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallbackMask::UnreliableDelta | NetworkCallbackMask::Update | NetworkCallbackMask::InterpolateState;

    explicit AdvancedNetworkingPlayer(Context* context)
        : NetworkBehavior(context, CallbackMask)
    {
    }

    void InitializeOnServer() override
    {
        InitializeCommon();

        animationController_->SetEnabled(false);
    }

    void InitializeFromSnapshot(unsigned frame, Deserializer& src) override
    {
        InitializeCommon();

        // TODO(network): Revisit
        //rotationSampler_.Setup(0, 15.0f, M_LARGE_VALUE);
    }

    bool PrepareUnreliableDelta(unsigned frame) override
    {
        return true;
    }

    void WriteUnreliableDelta(unsigned frame, Serializer& dest) override
    {
        const ea::string& currentAnimationName = animations_[currentAnimation_]->GetName();
        const float currentTime = animationController_->GetTime(currentAnimationName);

        dest.WriteVLE(currentAnimation_);
        dest.WriteFloat(currentTime);
        dest.WriteQuaternion(node_->GetWorldRotation());
    }

    void ReadUnreliableDelta(unsigned frame, Deserializer& src) override
    {
        const unsigned animationIndex = src.ReadVLE();
        const float animationTime = src.ReadFloat();
        const Quaternion rotation = src.ReadQuaternion();

        animationTrace_.Set(frame, {animationIndex, animationTime});
        rotationTrace_.Set(frame, rotation);
    }

    void InterpolateState(float timeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
    {
        NetworkObject* networkObject = GetNetworkObject();

        if (networkObject->IsReplicatedClient())
        {
            if (auto newRotation = rotationSampler_.UpdateAndSample(rotationTrace_, replicaTime, timeStep))
                node_->SetWorldRotation(*newRotation);

            // TODO(network): Extract this?
            ReplicationManager* replicationManager = networkObject->GetReplicationManager();
            if (const auto animationSample = animationTrace_.GetRawOrPrior(replicaTime.GetFrame()))
            {
                const auto& [value, confirmedFrame] = *animationSample;
                const auto& [animationIndex, animationTime] = value;

                const ea::string& animationName = animations_[animationIndex]->GetName();
                animationController_->PlayExclusive(animationName, 0, false, fadeTime_);

                const float networkTimeStep = 1.0f / replicationManager->GetUpdateFrequency();
                const float delay = (replicaTime - NetworkTime{confirmedFrame}) * networkTimeStep;
                animationController_->SetTime(animationName, Mod(animationTime + delay, animations_[animationIndex]->GetLength()));
            }
        }
    }

    void Update(float replicaTimeStep, float inputTimeStep) override
    {
        NetworkObject* networkObject = GetNetworkObject();
        if (networkObject->IsServer())
        {
            UpdateAnimations();
            animationController_->Update(replicaTimeStep);
        }
        else if (networkObject->IsOwnedByThisClient() || networkObject->IsStandalone())
        {
            UpdateAnimations();
        }
    }

private:
    void InitializeCommon()
    {
        NetworkObject* networkObject = GetNetworkObject();
        ReplicationManager* replicationManager = networkObject->GetReplicationManager();
        const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

        animationTrace_.Resize(traceDuration);
        rotationTrace_.Resize(traceDuration);

        replicatedTransform_ = GetNetworkObject()->GetNetworkBehavior<ReplicatedTransform>();
        networkController_ = GetNetworkObject()->GetNetworkBehavior<PredictedKinematicController>();

        kinematicController_ = networkController_->GetComponent<KinematicCharacterController>();
        animationController_ = GetNode()->GetComponent<AnimationController>();

        auto cache = GetSubsystem<ResourceCache>();
        animations_ = {
            cache->GetResource<Animation>("Models/Mutant/Mutant_Idle0.ani"),
            cache->GetResource<Animation>("Models/Mutant/Mutant_Run.ani"),
            cache->GetResource<Animation>("Models/Mutant/Mutant_Jump1.ani"),
        };
    }

    void UpdateAnimations()
    {
        const bool isGrounded = kinematicController_->OnGround();
        const Vector3 velocity = networkController_->GetVelocity();
        const Vector3 walkDirection = Vector3::FromXZ(velocity.ToXZ().NormalizedOrDefault(Vector2::ZERO, 0.1f));

        if ((currentAnimation_ != ANIM_JUMP) && velocity.y_ > jumpThreshold_)
        {
            animationController_->PlayExclusive(animations_[ANIM_JUMP]->GetName(), 0, false, fadeTime_);
            animationController_->SetTime(animations_[ANIM_JUMP]->GetName(), 0);
            currentAnimation_ = ANIM_JUMP;
        }

        // TODO(network): Smooth this
        if (walkDirection != Vector3::ZERO)
            node_->SetWorldRotation(Quaternion{Vector3::BACK, walkDirection});

        if (isGrounded)
        {
            if (walkDirection != Vector3::ZERO)
            {
                node_->SetWorldRotation(Quaternion{Vector3::BACK, walkDirection});
                animationController_->PlayExclusive(animations_[ANIM_WALK]->GetName(), 0, true, fadeTime_);
                currentAnimation_ = ANIM_WALK;
            }
            else
            {
                animationController_->PlayExclusive(animations_[ANIM_IDLE]->GetName(), 0, true, fadeTime_);
                currentAnimation_ = ANIM_IDLE;
            }
        }
    }

    enum
    {
        ANIM_IDLE,
        ANIM_WALK,
        ANIM_JUMP,
    };

    const float moveThreshold_{0.1f};
    const float jumpThreshold_{0.2f};
    const float fadeTime_{0.1f};

    WeakPtr<ReplicatedTransform> replicatedTransform_;
    WeakPtr<PredictedKinematicController> networkController_;

    WeakPtr<KinematicCharacterController> kinematicController_;
    WeakPtr<AnimationController> animationController_;

    ea::vector<Animation*> animations_;

    unsigned currentAnimation_{};
    NetworkValue<ea::pair<unsigned, float>> animationTrace_;
    NetworkValue<Quaternion> rotationTrace_;
    NetworkValueSampler<Quaternion> rotationSampler_;
};

void AdvancedNetworkingUI::StartServer()
{
    Stop();

    Network* network = GetSubsystem<Network>();
    network->StartServer(serverPort_);
}

void AdvancedNetworkingUI::ConnectToServer(const ea::string& address)
{
    Stop();

    Network* network = GetSubsystem<Network>();
    network->Connect(address, serverPort_, GetScene());
}

void AdvancedNetworkingUI::Stop()
{
    Network* network = GetSubsystem<Network>();
    network->Disconnect();
    network->StopServer();
}

void AdvancedNetworkingUI::OnNodeSet(Node* node)
{
    BaseClassName::OnNodeSet(node);
    RmlUI* rmlUI = GetUI();
    Network* network = GetSubsystem<Network>();
    Rml::Context* rmlContext = rmlUI->GetRmlContext();

    if (node != nullptr && !model_)
    {
        Rml::DataModelConstructor constructor = rmlContext->CreateDataModel("AdvancedNetworkingUI_model");
        if (!constructor)
            return;

        constructor.Bind("port", &serverPort_);
        constructor.Bind("connectionAddress", &connectionAddress_);
        constructor.BindFunc("isServer", [=](Rml::Variant& result) { result = network->IsServerRunning(); });
        constructor.BindFunc("isClient", [=](Rml::Variant& result) { result = network->GetServerConnection() != nullptr; });

        constructor.BindEventCallback("onStartServer",
            [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { StartServer(); });
        constructor.BindEventCallback("onConnectToServer",
            [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { ConnectToServer(connectionAddress_); });
        constructor.BindEventCallback("onStop",
            [=](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Stop(); });

        model_ = constructor.GetModelHandle();

        SetResource("UI/AdvancedNetworkingUI.rml");
        SetOpen(true);
    }
    else if (node == nullptr && model_)
    {
        rmlContext->RemoveDataModel("AdvancedNetworkingUI_model");
        model_ = nullptr;
    }
}

void AdvancedNetworkingUI::Update(float timeStep)
{
    model_.DirtyVariable("isServer");
    model_.DirtyVariable("isClient");
}

AdvancedNetworking::AdvancedNetworking(Context* context) :
    Sample(context)
{
}

void AdvancedNetworking::Start(const ea::vector<ea::string>& args)
{
    if (!context_->IsReflected<AdvancedNetworkingUI>())
        context_->RegisterFactory<AdvancedNetworkingUI>();

    if (!context_->IsReflected<AdvancedNetworkingPlayer>())
        context_->RegisterFactory<AdvancedNetworkingPlayer>();

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
    Sample::InitMouseMode(MM_ABSOLUTE);

    // Process command line
    if (args.size() >= 2)
    {
        if (args[1] == "StartServer")
            ui_->StartServer();
        else if (args[1] == "Connect")
            ui_->ConnectToServer("localhost");
    }
}

void AdvancedNetworking::CreateScene()
{
    scene_ = new Scene(context_);

    auto* cache = GetSubsystem<ResourceCache>();

    // Create octree and physics world with default settings. Create them as local so that they are not needlessly replicated
    // when a client connects
    scene_->CreateComponent<Octree>(LOCAL);
    scene_->CreateComponent<PhysicsWorld>(LOCAL);
    scene_->CreateComponent<ReplicationManager>(LOCAL);

    // All static scene content and the camera are also created as local, so that they are unaffected by scene replication and are
    // not removed from the client upon connection. Create a Zone component first for ambient lighting & fog control.
    Node* zoneNode = scene_->CreateChild("Zone", LOCAL);
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.1f, 0.1f, 0.1f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light without shadows
    Node* lightNode = scene_->CreateChild("DirectionalLight", LOCAL);
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
            Node* floorNode = scene_->CreateChild("FloorTile", LOCAL);
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

    // Create "random" boxes
    RandomEngine re(0);
    const unsigned NUM_OBJECTS = 200;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* obstacleNode = scene_->CreateChild("Box", LOCAL);
        const float scale = re.GetFloat(1.5f, 4.0f);
        obstacleNode->SetPosition(re.GetVector3({-45.0f, scale / 2, -45.0f }, {45.0f, scale / 2, 45.0f }));
        obstacleNode->SetRotation(Quaternion(Vector3::UP, re.GetVector3({-0.4f, 1.0f, -0.4f}, {0.4f, 1.0f, 0.4f})));
        obstacleNode->SetScale(scale);

        auto* obstacleModel = obstacleNode->CreateComponent<StaticModel>();
        obstacleModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        obstacleModel->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

        auto* body = obstacleNode->CreateComponent<RigidBody>();
        body->SetFriction(1.0f);
        auto* shape = obstacleNode->CreateComponent<CollisionShape>();
        shape->SetBox(Vector3::ONE);
    }

    // Create the camera. Limit far clip distance to match the fog
    // The camera needs to be created into a local node so that each client can retain its own camera, that is unaffected by
    // network messages. Furthermore, because the client removes all replicated scene nodes when connecting to a server scene,
    // the screen would become blank if the camera node was replicated (as only the locally created camera is assigned to a
    // viewport in SetupViewports() below)
    cameraNode_ = scene_->CreateChild("Camera", LOCAL);
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void AdvancedNetworking::CreateUI()
{
    Node* node = scene_->CreateChild("UI", LOCAL);
    ui_ = node->CreateComponent<AdvancedNetworkingUI>();

    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();
    UIElement* root = ui->GetRoot();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it can interact with the login UI
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto(uiStyle);
    ui->SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    auto* graphics = GetSubsystem<Graphics>();
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

    // Construct the instructions text element
    instructionsText_ = ui->GetRoot()->CreateChild<Text>();
    instructionsText_->SetText(
        "Use WASD and Space to move and RMB to rotate view"
    );
    instructionsText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // Position the text relative to the screen center
    instructionsText_->SetHorizontalAlignment(HA_CENTER);
    instructionsText_->SetVerticalAlignment(VA_CENTER);
    instructionsText_->SetPosition(0, graphics->GetHeight() / 4);
    // Hide until connected
    instructionsText_->SetVisible(false);

    statsText_ = ui->GetRoot()->CreateChild<Text>();
    statsText_->SetText("No network stats");
    statsText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    statsText_->SetHorizontalAlignment(HA_LEFT);
    statsText_->SetVerticalAlignment(VA_CENTER);
    statsText_->SetPosition(10, -10);
}

void AdvancedNetworking::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void AdvancedNetworking::SubscribeToEvents()
{
    // Subscribe HandlePostUpdate() method for processing update events. Subscribe to PostUpdate instead
    // of the usual Update so that physics simulation has already proceeded for the frame, and can
    // accurately follow the object with the camera
    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(AdvancedNetworking, HandlePostUpdate));

    // Subscribe to network events
    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(AdvancedNetworking, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(AdvancedNetworking, HandleClientDisconnected));
}

Node* AdvancedNetworking::CreateControllableObject(Connection* owner)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto prefab = cache->GetResource<XMLFile>("Objects/AdvancedNetworkingPlayer.xml");

    const Vector3 position{Random(20.0f) - 10.0f, 5.0f, Random(20.0f) - 10.0f};
    Node* playerNode = scene_->InstantiateXML(prefab->GetRoot(), position, Quaternion::IDENTITY, LOCAL);

    auto networkObject = playerNode->CreateComponent<BehaviorNetworkObject>();
    networkObject->SetClientPrefab(prefab);
    networkObject->SetOwner(owner);

    Light* playerLight = playerNode->GetComponent<Light>(true);
    playerLight->SetColor(Color::GREEN);

    return playerNode;
}

void AdvancedNetworking::MoveCamera()
{
    auto* ui = GetSubsystem<UI>();
    auto* input = GetSubsystem<Input>();

    // Right mouse button controls mouse cursor visibility: hide when pressed
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
    auto replicationManager = scene_->GetComponent<ReplicationManager>();
    auto clientReplica = replicationManager ? replicationManager->GetClientReplica() : nullptr;
    if (clientReplica && !clientReplica->GetOwnedNetworkObjects().empty())
    {
        NetworkObject* clientObject = *clientReplica->GetOwnedNetworkObjects().begin();
        Node* clientNode = clientObject->GetNode();
        auto clientController = clientNode->GetComponent<PredictedKinematicController>();

        // Set desired movement
        const Quaternion rotation(0.0f, yaw_, 0.0f);
        Vector3 direction;
        if (input->GetKeyDown(KEY_W))
            direction += rotation * Vector3::FORWARD;
        if (input->GetKeyDown(KEY_S))
            direction += rotation * Vector3::BACK;
        if (input->GetKeyDown(KEY_A))
            direction += rotation * Vector3::LEFT;
        if (input->GetKeyDown(KEY_D))
            direction += rotation * Vector3::RIGHT;

        direction = direction.NormalizedOrDefault();
        const bool needJump = input->GetKeyDown(KEY_SPACE);

        const float walkVelocity = 3.35f;
        clientController->SetWalkVelocity(direction * walkVelocity);
        if (needJump)
            clientController->SetJump();

        // Move camera some distance away from the player
        const float CAMERA_DISTANCE = 5.0f;
        const float CAMERA_OFFSET = 2.0f;
        cameraNode_->SetPosition(clientNode->GetPosition()
            + cameraNode_->GetRotation() * Vector3::BACK * CAMERA_DISTANCE + Vector3::UP * CAMERA_OFFSET);
        showInstructions = true;
    }

    instructionsText_->SetVisible(showInstructions);
}

void AdvancedNetworking::UpdateStats()
{
    auto network = GetSubsystem<Network>();
    ea::string stats;

    int packetsIn = 0;
    int packetsOut = 0;
    ea::hash_set<ReplicationManager*> networkManagers;
    if (Connection* serverConnection = network->GetServerConnection())
    {
        packetsIn = serverConnection->GetPacketsInPerSec();
        packetsOut = serverConnection->GetPacketsOutPerSec();
        if (Scene* scene = serverConnection->GetScene())
        {
            if (auto replicationManager = scene->GetComponent<ReplicationManager>())
                networkManagers.insert(replicationManager);
        }
    }
    else
    {
        for (Connection* clientConnection : network->GetClientConnections())
        {
            packetsIn += clientConnection->GetPacketsInPerSec();
            packetsOut += clientConnection->GetPacketsOutPerSec();
            if (Scene* scene = clientConnection->GetScene())
            {
                if (auto replicationManager = scene->GetComponent<ReplicationManager>())
                    networkManagers.insert(replicationManager);
            }
        }
    }

    stats += Format("{:4} packets/s in\n", packetsIn);
    stats += Format("{:4} packets/s out\n", packetsOut);
    for (ReplicationManager* replicationManager : networkManagers)
        stats += replicationManager->GetDebugInfo();

    statsText_->SetText(stats);
}

void AdvancedNetworking::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    // TODO(network): Remove
    //scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(scene_->GetOrCreateComponent<DebugRenderer>(), false);

    // We only rotate the camera according to mouse movement since last frame, so do not need the time step
    MoveCamera();
    UpdateStats();
}

void AdvancedNetworking::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client connects, assign to scene to begin scene replication
    auto* newConnection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    newConnection->SetScene(scene_);

    // Then create a controllable object for that client
    Node* newObject = CreateControllableObject(newConnection);
    serverObjects_[newConnection] = newObject;
}

void AdvancedNetworking::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    using namespace ClientConnected;

    // When a client disconnects, remove the controlled object
    auto* connection = static_cast<Connection*>(eventData[P_CONNECTION].GetPtr());
    Node* object = serverObjects_[connection];
    if (object)
        object->Remove();

    serverObjects_.erase(connection);
}
