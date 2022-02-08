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
#include <Urho3D/Graphics/AnimatedModel.h>
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
#include <Urho3D/IO/Log.h>
#include <Urho3D/Input/Input.h>
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
#include <Urho3D/Replica/ReplicatedAnimation.h>
#include <Urho3D/Replica/ReplicatedTransform.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Replica/TrackedAnimatedModel.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>

#include "AdvancedNetworking.h"

#include <Urho3D/DebugNew.h>

static constexpr unsigned IMPORTANT_VIEW_MASK = 0x1;
static constexpr unsigned UNIMPORTANT_VIEW_MASK = 0x2;
static constexpr float CAMERA_DISTANCE = 5.0f;
static constexpr float CAMERA_OFFSET = 2.0f;
static constexpr float WALK_VELOCITY = 3.35f;
static constexpr float HIT_DISTANCE = 100.0f;
static constexpr float AUTO_MOVEMENT_DURATION = 1.0f;
static constexpr float MAX_ROTATION_SPEED = 360.0f;

URHO3D_EVENT(E_ADVANCEDNETWORKING_RAYCAST, AdvancedNetworkingRaycast)
{
    URHO3D_PARAM(P_ORIGIN, Origin);
    URHO3D_PARAM(P_TARGET, Target);
    URHO3D_PARAM(P_REPLICA_FRAME, ReplicaFrame);
    URHO3D_PARAM(P_REPLICA_SUBFRAME, ReplicaSubFrame);
    URHO3D_PARAM(P_INPUT_FRAME, InputFrame);
    URHO3D_PARAM(P_INPUT_SUBFRAME, InputSubFrame);
}

URHO3D_EVENT(E_ADVANCEDNETWORKING_RAYHIT, AdvancedNetworkingRayhit)
{
    URHO3D_PARAM(P_ORIGIN, Origin);
    URHO3D_PARAM(P_POSITION, Position);
}

/// Return shortest angle between two angles.
float ShortestAngle(float lhs, float rhs)
{
    const float delta = Mod(rhs - lhs + 180.0f, 360.0f) - 180.0f;
    return delta < -180.0f ? delta + 360.0f : delta;
}

/// Helper function to smoothly transform one quaternion into another.
Quaternion TransformRotation(const Quaternion& base, const Quaternion& target, float maxAngularVelocity)
{
    const float baseYaw = base.YawAngle();
    const float targetYaw = target.YawAngle();
    if (Equals(baseYaw, targetYaw, 0.1f))
        return base;

    const float shortestAngle = ShortestAngle(baseYaw, targetYaw);
    const float delta = Sign(shortestAngle) * ea::min(Abs(shortestAngle), maxAngularVelocity);
    return Quaternion{baseYaw + delta, Vector3::UP};
}

/// Custom networking component that handles all sample-specific behaviors:
/// - Animation synchronization;
/// - Player rotation synchronization;
/// - View mask assignment for easy raycasting.
class AdvancedNetworkingPlayer : public ReplicatedAnimation
{
    URHO3D_OBJECT(AdvancedNetworkingPlayer, ReplicatedAnimation);

public:
    static constexpr unsigned RingBufferSize = 8;
    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallbackMask::UnreliableDelta | NetworkCallbackMask::Update | NetworkCallbackMask::InterpolateState;

    explicit AdvancedNetworkingPlayer(Context* context)
        : ReplicatedAnimation(context, CallbackMask)
    {
        auto cache = GetSubsystem<ResourceCache>();
        animations_ = {
            cache->GetResource<Animation>("Models/Mutant/Mutant_Idle0.ani"),
            cache->GetResource<Animation>("Models/Mutant/Mutant_Run.ani"),
            cache->GetResource<Animation>("Models/Mutant/Mutant_Jump1.ani"),
        };
    }

    /// Initialize component on the server.
    void InitializeOnServer() override
    {
        BaseClassName::InitializeOnServer();

        InitializeCommon();

        // On server, all players are unimportant because they are moving and need temporal raycasts
        auto animatedModel = GetComponent<AnimatedModel>();
        animatedModel->SetViewMask(UNIMPORTANT_VIEW_MASK);
    }

    /// Initialize component on the client.
    void InitializeFromSnapshot(unsigned frame, Deserializer& src, bool isOwned) override
    {
        BaseClassName::InitializeFromSnapshot(frame, src, isOwned);

        InitializeCommon();

        // Mark all players except ourselves as important for raycast.
        auto animatedModel = GetComponent<AnimatedModel>();
        animatedModel->SetViewMask(isOwned ? UNIMPORTANT_VIEW_MASK : IMPORTANT_VIEW_MASK);

        // Setup client-side containers
        ReplicationManager* replicationManager = GetNetworkObject()->GetReplicationManager();
        const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

        animationTrace_.Resize(traceDuration);
        rotationTrace_.Resize(traceDuration);

        // Smooth rotation a bit, but without extrapolation
        rotationSampler_.Setup(0, 15.0f, M_LARGE_VALUE);
    }

    /// Always send animation updates for simplicity.
    bool PrepareUnreliableDelta(unsigned frame) override
    {
        BaseClassName::PrepareUnreliableDelta(frame);

        return true;
    }

    /// Write current animation state and rotation on server.
    void WriteUnreliableDelta(unsigned frame, Serializer& dest) override
    {
        BaseClassName::WriteUnreliableDelta(frame, dest);

        const ea::string& currentAnimationName = animations_[currentAnimation_]->GetName();
        const float currentTime = animationController_->GetTime(currentAnimationName);

        dest.WriteVLE(currentAnimation_);
        dest.WriteFloat(currentTime);
        dest.WriteQuaternion(node_->GetWorldRotation());
    }

    /// Read current animation state on replicating client.
    void ReadUnreliableDelta(unsigned frame, Deserializer& src) override
    {
        BaseClassName::ReadUnreliableDelta(frame, src);

        const unsigned animationIndex = src.ReadVLE();
        const float animationTime = src.ReadFloat();
        const Quaternion rotation = src.ReadQuaternion();

        animationTrace_.Set(frame, {animationIndex, animationTime});
        rotationTrace_.Set(frame, rotation);
    }

    /// Perform interpolation of received data on replicated client.
    void InterpolateState(float timeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
    {
        BaseClassName::InterpolateState(timeStep, replicaTime, inputTime);

        if (!GetNetworkObject()->IsReplicatedClient())
            return;

        // Interpolate player rotation
        if (auto newRotation = rotationSampler_.UpdateAndSample(rotationTrace_, replicaTime, timeStep))
            node_->SetWorldRotation(*newRotation);

        // Extrapolate animation time from trace
        if (const auto animationSample = animationTrace_.GetRawOrPrior(replicaTime.GetFrame()))
        {
            const auto& [value, sampleFrame] = *animationSample;
            const auto& [animationIndex, animationTime] = value;

            // Just play most recent received animation
            Animation* animation = animations_[animationIndex];
            animationController_->PlayExclusive(animation->GetName(), 0, false, fadeTime_);

            // Adjust time to stay synchronized with server
            const float delay = (replicaTime - NetworkTime{sampleFrame}) * networkFrameDuration_;
            const float time = Mod(animationTime + delay, animation->GetLength());
            animationController_->SetTime(animation->GetName(), time);
        }
    }

    void Update(float replicaTimeStep, float inputTimeStep) override
    {
        if (!GetNetworkObject()->IsReplicatedClient())
            UpdateAnimations(inputTimeStep);

        BaseClassName::Update(replicaTimeStep, inputTimeStep);
    }

private:
    void InitializeCommon()
    {
        ReplicationManager* replicationManager = GetNetworkObject()->GetReplicationManager();
        networkFrameDuration_ = 1.0f / replicationManager->GetUpdateFrequency();

        // Resolve dependencies
        replicatedTransform_ = GetNetworkObject()->GetNetworkBehavior<ReplicatedTransform>();
        networkController_ = GetNetworkObject()->GetNetworkBehavior<PredictedKinematicController>();
        kinematicController_ = networkController_->GetComponent<KinematicCharacterController>();
    }

    void UpdateAnimations(float timeStep)
    {
        // Get current state of the controller to deduce animation from
        const bool isGrounded = kinematicController_->OnGround();
        const Vector3 velocity = networkController_->GetVelocity();
        const Vector3 walkDirection = Vector3::FromXZ(velocity.ToXZ().NormalizedOrDefault(Vector2::ZERO, 0.1f));

        // Start jump if has high vertical velocity and hasn't jumped yet
        if ((currentAnimation_ != ANIM_JUMP) && velocity.y_ > jumpThreshold_)
        {
            animationController_->PlayExclusive(animations_[ANIM_JUMP]->GetName(), 0, false, fadeTime_);
            animationController_->SetTime(animations_[ANIM_JUMP]->GetName(), 0);
            currentAnimation_ = ANIM_JUMP;
        }

        // Rotate player both on ground and in the air
        if (walkDirection != Vector3::ZERO)
        {
            const Quaternion targetRotation{Vector3::BACK, walkDirection};
            const float maxAngularVelocity = MAX_ROTATION_SPEED * timeStep;
            node_->SetWorldRotation(TransformRotation(node_->GetWorldRotation(), targetRotation, maxAngularVelocity));
        }

        // If on the ground, either walk or stay idle
        if (isGrounded)
        {
            currentAnimation_ = walkDirection != Vector3::ZERO ? ANIM_WALK : ANIM_IDLE;
            animationController_->PlayExclusive(animations_[currentAnimation_]->GetName(), 0, true, fadeTime_);
        }
    }

    /// Enum that represents current animation state and corresponding animations.
    enum
    {
        ANIM_IDLE,
        ANIM_WALK,
        ANIM_JUMP,
    };
    ea::vector<Animation*> animations_;

    /// Animation parameters.
    const float moveThreshold_{0.1f};
    const float jumpThreshold_{0.2f};
    const float fadeTime_{0.1f};

    /// Dependencies of this behaviors.
    WeakPtr<ReplicatedTransform> replicatedTransform_;
    WeakPtr<PredictedKinematicController> networkController_;
    WeakPtr<KinematicCharacterController> kinematicController_;

    /// Index of current animation tracked on server for simplicity.
    unsigned currentAnimation_{};

    /// Client-side containers that keep aggregated data from the server for the future sampling.
    float networkFrameDuration_{};
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
        constructor.Bind("cheatAutoMovement", &cheatAutoMovement_);

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
    // Register sample types
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

    // Create collection of hit markers
    hitMarkers_ = scene_->CreateChild("Hit Markers", LOCAL);

    // Create a "floor" consisting of several tiles. Make the tiles physical but leave small cracks between them
    for (int y = -20; y <= 20; ++y)
    {
        for (int x = -20; x <= 20; ++x)
        {
            Node* floorNode = scene_->CreateChild("FloorTile", LOCAL);
            floorNode->SetPosition(Vector3(x * 20.2f, -0.5f, y * 20.2f));
            floorNode->SetScale(Vector3(20.0f, 1.0f, 20.0f));

            auto* floorModel = floorNode->CreateComponent<StaticModel>();
            floorModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
            floorModel->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
            floorModel->SetViewMask(IMPORTANT_VIEW_MASK);

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
        obstacleModel->SetViewMask(IMPORTANT_VIEW_MASK);

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
    // Subscribe to raycast events.
    SubscribeToEvent(E_ADVANCEDNETWORKING_RAYCAST,
        [this](StringHash, VariantMap& eventData)
    {
        using namespace AdvancedNetworkingRaycast;
        ServerRaycastInfo info;

        info.clientConnection_.Reset(static_cast<Connection*>(eventData[RemoteEventData::P_CONNECTION].GetPtr()));
        info.origin_ = eventData[P_ORIGIN].GetVector3();
        info.target_ = eventData[P_TARGET].GetVector3();
        info.replicaTime_ = NetworkTime{eventData[P_REPLICA_FRAME].GetUInt(), eventData[P_REPLICA_SUBFRAME].GetFloat()};
        info.inputTime_ = NetworkTime{eventData[P_INPUT_FRAME].GetUInt(), eventData[P_INPUT_SUBFRAME].GetFloat()};

        serverRaycasts_.push_back(info);
    });

    // Subscribe to rayhit events.
    SubscribeToEvent(E_ADVANCEDNETWORKING_RAYHIT,
        [this](StringHash, VariantMap& eventData)
    {
        using namespace AdvancedNetworkingRayhit;
        const Variant& position = eventData[P_POSITION];
        if (!position.IsEmpty())
            AddHitMarker(position.GetVector3(), true);
    });

    // Subscribe HandlePostUpdate() method for processing update events. Subscribe to PostUpdate instead
    // of the usual Update so that physics simulation has already proceeded for the frame, and can
    // accurately follow the object with the camera
    SubscribeToEvent(E_POSTUPDATE,
        [this](StringHash, VariantMap& eventData)
    {
        ProcessRaycastsOnServer();
        MoveCamera();
        UpdateStats();
    });

    // Subscribe to network events
    SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(AdvancedNetworking, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(AdvancedNetworking, HandleClientDisconnected));

    auto* network = GetSubsystem<Network>();
    network->RegisterRemoteEvent(E_ADVANCEDNETWORKING_RAYCAST);
    network->RegisterRemoteEvent(E_ADVANCEDNETWORKING_RAYHIT);
}

void AdvancedNetworking::ProcessRaycastsOnServer()
{
    auto* replicationManager = scene_->GetComponent<ReplicationManager>();
    ServerReplicator* serverReplicator = replicationManager ? replicationManager->GetServerReplicator() : nullptr;
    if (!serverReplicator)
        return;

    // Process and dequeue raycasts when possible.
    const NetworkTime serverTime = serverReplicator->GetServerTime();

    ea::erase_if(serverRaycasts_,
        [&](const ServerRaycastInfo& raycastInfo)
    {
        if (!raycastInfo.clientConnection_)
            return true;

        // Don't process too early, server time should be greater than input time on client.
        if (serverTime - raycastInfo.inputTime_ < 0.0f)
            return false;

        ProcessSingleRaycastOnServer(raycastInfo);
        return true;
    });
}

void AdvancedNetworking::ProcessSingleRaycastOnServer(const ServerRaycastInfo& raycastInfo)
{
    auto* replicationManager = scene_->GetComponent<ReplicationManager>();
    ServerReplicator* serverReplicator = replicationManager->GetServerReplicator();

    // Get reliable origin from server data, not trusting client with this
    NetworkObject* clientObject = serverReplicator->GetNetworkObjectOwnedByConnection(raycastInfo.clientConnection_);
    auto replicatedTransform = clientObject->GetNode()->GetComponent<ReplicatedTransform>(true);
    const Vector3 origin = replicatedTransform->SampleTemporalPosition(raycastInfo.inputTime_).value_ + Vector3::UP * CAMERA_OFFSET;

    // Perform raycast using target position instead of ray direction
    // to get better precision on origin mismatch.
    auto* octree = scene_->GetComponent<Octree>();
    const Ray ray{origin, raycastInfo.target_ - origin};

    // Query static scene geometry
    RayOctreeQuery query(ray, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY, IMPORTANT_VIEW_MASK);
    octree->RaycastSingle(query);

    // Query dynamic network objects
    for (NetworkObject* networkObject : replicationManager->GetNetworkObjects())
    {
        // Ignore caster
        if (networkObject == clientObject)
            continue;

        auto behaviorNetworkObject = dynamic_cast<BehaviorNetworkObject*>(networkObject);
        if (!behaviorNetworkObject)
            continue;

        auto trackedModelAnimation = behaviorNetworkObject->GetNetworkBehavior<TrackedAnimatedModel>();
        if (!trackedModelAnimation)
            continue;

        trackedModelAnimation->ProcessTemporalRayQuery(raycastInfo.replicaTime_, query, query.result_);
    }

    // Sort by distance
    ea::quick_sort(query.result_.begin(), query.result_.end(),
        [](const RayQueryResult& lhs, const RayQueryResult& rhs) { return lhs.distance_ < rhs.distance_; });

    // Send result to the client
    using namespace AdvancedNetworkingRayhit;
    auto& eventData = GetEventDataMap();
    eventData[P_ORIGIN] = origin;
    if (!query.result_.empty())
        eventData[P_POSITION] = query.result_[0].position_;
    raycastInfo.clientConnection_->SendRemoteEvent(E_ADVANCEDNETWORKING_RAYHIT, false, eventData);
}

Node* AdvancedNetworking::CreateControllableObject(Connection* owner)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto prefab = cache->GetResource<XMLFile>("Objects/AdvancedNetworkingPlayer.xml");

    // Instantiate most of the components from prefab so they will be replicated on the client.
    const Vector3 position{Random(20.0f) - 10.0f, 5.0f, Random(20.0f) - 10.0f};
    Node* playerNode = scene_->InstantiateXML(prefab->GetRoot(), position, Quaternion::IDENTITY, LOCAL);

    // NetworkObject should never be a part of client prefab
    auto networkObject = playerNode->CreateComponent<BehaviorNetworkObject>();
    networkObject->SetClientPrefab(prefab);
    networkObject->SetOwner(owner);

    // Change light color on the server only
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

    // Process client-side input
    auto replicationManager = scene_->GetComponent<ReplicationManager>();
    auto clientReplica = replicationManager ? replicationManager->GetClientReplica() : nullptr;
    if (clientReplica && clientReplica->HasOwnedNetworkObjects())
    {
        NetworkObject* clientObject = clientReplica->GetOwnedNetworkObject();

        ProcessClientMovement(clientObject);

        if (input->GetMouseButtonPress(MOUSEB_LEFT))
        {
            auto* renderer = GetSubsystem<Renderer>();
            Viewport* viewport = renderer->GetViewport(0);
            const IntVector2 mousePos = input->GetMousePosition();
            const Ray screenRay = viewport->GetScreenRay(mousePos.x_, mousePos.y_);

            RequestClientRaycast(clientObject, screenRay);
        }

        showInstructions = true;
    }

    instructionsText_->SetVisible(showInstructions);
}

void AdvancedNetworking::ProcessClientMovement(NetworkObject* clientObject)
{
    auto* input = GetSubsystem<Input>();

    Node* clientNode = clientObject->GetNode();
    auto clientController = clientNode->GetComponent<PredictedKinematicController>();

    // Process auto movement cheat
    const bool autoMovement = ui_->GetCheatAutoMovement();
    if (!autoMovement)
    {
        autoMovementTimer_ = AUTO_MOVEMENT_DURATION;
        autoMovementPhase_ = 0;
    }
    else
    {
        autoMovementTimer_ -= GetSubsystem<Time>()->GetTimeStep();
        if (autoMovementTimer_ < 0.0f)
        {
            autoMovementTimer_ = AUTO_MOVEMENT_DURATION;
            autoMovementPhase_ = (autoMovementPhase_ + 1) % 4;
        }
    }

    // Calculate movement direction
    const Quaternion rotation(0.0f, yaw_, 0.0f);
    Vector3 direction;
    if (input->GetKeyDown(KEY_W) || (autoMovement && autoMovementPhase_ == 3))
        direction += rotation * Vector3::FORWARD;
    if (input->GetKeyDown(KEY_S) || (autoMovement && autoMovementPhase_ == 1))
        direction += rotation * Vector3::BACK;
    if (input->GetKeyDown(KEY_A) || (autoMovement && autoMovementPhase_ == 2))
        direction += rotation * Vector3::LEFT;
    if (input->GetKeyDown(KEY_D) || (autoMovement && autoMovementPhase_ == 0))
        direction += rotation * Vector3::RIGHT;
    direction = direction.NormalizedOrDefault();

    // Ability to jump is checked inside of PredictedKinematicController
    const bool needJump = input->GetKeyDown(KEY_SPACE);

    // Apply user input. It may happen at any point in game cycle.
    // Note that this input will not take effect immediately.
    clientController->SetWalkVelocity(direction * WALK_VELOCITY);
    if (needJump)
        clientController->SetJump();

    // Focus camera on client node
    cameraNode_->SetPosition(clientNode->GetPosition()
        + cameraNode_->GetRotation() * Vector3::BACK * CAMERA_DISTANCE + Vector3::UP * CAMERA_OFFSET);
}

void AdvancedNetworking::RequestClientRaycast(NetworkObject* clientObject, const Ray& screenRay)
{
    auto* network = GetSubsystem<Network>();
    Connection* serverConnection = network->GetServerConnection();

    // Get current client times so server knows when the raycast was performed
    auto* replicationManager = scene_->GetComponent<ReplicationManager>();
    ClientReplica* clientReplica = replicationManager->GetClientReplica();

    const NetworkTime replicaTime = clientReplica->GetReplicaTime();
    const NetworkTime inputTime = clientReplica->GetInputTime();

    // First, check if user even clicked on anything, using raw screen ray.
    // If user didn't click anything, just use far position of the ray.
    const Vector3 defaultAimPosition = screenRay.origin_ + screenRay.direction_ * HIT_DISTANCE;

    // Second, perform an actual raycast from the player model to the aim point.
    const Vector3 aimPosition = RaycastImportantGeometries(screenRay).value_or(defaultAimPosition);
    const Vector3 origin = clientObject->GetNode()->GetWorldPosition() + Vector3::UP * CAMERA_OFFSET;
    const Ray castRay{origin, aimPosition - origin};

    // If hit on client, add marker
    if (auto hitPosition = RaycastImportantGeometries(castRay))
        AddHitMarker(*hitPosition, false);

    // Send event to the server regardless
    using namespace AdvancedNetworkingRaycast;
    VariantMap& eventData = GetEventDataMap();
    eventData[P_ORIGIN] = origin;
    eventData[P_TARGET] = aimPosition;
    eventData[P_REPLICA_FRAME] = replicaTime.GetFrame();
    eventData[P_REPLICA_SUBFRAME] = replicaTime.GetSubFrame();
    eventData[P_INPUT_FRAME] = inputTime.GetFrame();
    eventData[P_INPUT_SUBFRAME] = inputTime.GetSubFrame();
    serverConnection->SendRemoteEvent(E_ADVANCEDNETWORKING_RAYCAST, false, eventData);
}

void AdvancedNetworking::AddHitMarker(const Vector3& position, bool isConfirmed)
{
    auto* cache = GetSubsystem<ResourceCache>();

    Node* markerNode = hitMarkers_->CreateChild("Client Hit");
    markerNode->SetPosition(position);

    auto markerModel = markerNode->CreateComponent<StaticModel>();
    markerModel->SetViewMask(UNIMPORTANT_VIEW_MASK);
    if (isConfirmed)
    {
        markerNode->SetScale(0.15f);
        markerModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        markerModel->SetMaterial(cache->GetResource<Material>("Materials/Constant/GlowingGreen.xml"));
    }
    else
    {
        markerNode->SetScale(0.2f);
        markerModel->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
        markerModel->SetMaterial(cache->GetResource<Material>("Materials/Constant/MattRed.xml"));
    }
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

ea::optional<Vector3> AdvancedNetworking::RaycastImportantGeometries(const Ray& ray) const
{
    auto* octree = scene_->GetComponent<Octree>();

    RayOctreeQuery query(ray, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY, IMPORTANT_VIEW_MASK);
    octree->RaycastSingle(query);

    if (!query.result_.empty())
        return query.result_[0].position_;
    return ea::nullopt;
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
