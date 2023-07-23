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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/MoveAndOrbitController.h>
#include <Urho3D/Math/RandomEngine.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Replica/PredictedKinematicController.h>
#include <Urho3D/Replica/ReplicatedAnimation.h>
#include <Urho3D/Replica/ReplicatedTransform.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Replica/TrackedAnimatedModel.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "AdvancedNetworking.h"
#include "AdvancedNetworkingPlayer.h"
#include "AdvancedNetworkingUI.h"
#include "AdvancedNetworkingRaycast.h"

#include <Urho3D/DebugNew.h>

static constexpr float CAMERA_DISTANCE = 5.0f;
static constexpr float CAMERA_OFFSET = 2.0f;
static constexpr float WALK_VELOCITY = 3.35f;
static constexpr float HIT_DISTANCE = 100.0f;

AdvancedNetworking::AdvancedNetworking(Context* context) :
    Sample(context)
{
}

void AdvancedNetworking::Start(const ea::vector<ea::string>& args)
{
    // Register sample types
    if (!context_->IsReflected<AdvancedNetworkingUI>())
        context_->AddFactoryReflection<AdvancedNetworkingUI>();

    if (!context_->IsReflected<AdvancedNetworkingPlayer>())
        context_->AddFactoryReflection<AdvancedNetworkingPlayer>();

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
    SetMouseMode(MM_FREE);
    SetMouseVisible(false);

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
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();
    scene_->CreateComponent<ReplicationManager>();

    // All static scene content and the camera are also created as local, so that they are unaffected by scene replication and are
    // not removed from the client upon connection. Create a Zone component first for ambient lighting & fog control.
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color::GRAY);
    zone->SetBackgroundBrightness(1.0f);
    zone->SetFogColor(Color::WHITE);
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);
    zone->SetZoneTexture(cache->GetResource<TextureCube>("Textures/Skybox.xml"));

    // Create skybox.
    Node* skyNode = scene_->CreateChild("Sky");
    skyNode->SetScale(500.0f); // The scale actually does not matter
    auto* skybox = skyNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));

    // Create a directional light without shadows
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.5f, -1.0f, -0.5f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor(Color::WHITE * 0.2f);
    light->SetCastShadows(true);
    light->SetShadowCascade(CascadeParameters{10.0f, 23.0f, 45.0f, 70.0f, 50.0f});

    // Create collection of hit markers
    hitMarkers_ = scene_->CreateChild("Hit Markers");

    // Create a "floor" consisting of several tiles. Make the tiles physical but leave small cracks between them
    for (int y = -20; y <= 20; ++y)
    {
        for (int x = -20; x <= 20; ++x)
        {
            Node* floorNode = scene_->CreateChild("FloorTile");
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
        Node* obstacleNode = scene_->CreateChild("Box");
        const float scale = re.GetFloat(1.5f, 4.0f);
        obstacleNode->SetPosition(re.GetVector3({-45.0f, scale / 2, -45.0f }, {45.0f, scale / 2, 45.0f }));
        obstacleNode->SetRotation(Quaternion(Vector3::UP, re.GetVector3({-0.4f, 1.0f, -0.4f}, {0.4f, 1.0f, 0.4f})));
        obstacleNode->SetScale(scale);

        auto* obstacleModel = obstacleNode->CreateComponent<StaticModel>();
        obstacleModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        obstacleModel->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));
        obstacleModel->SetViewMask(IMPORTANT_VIEW_MASK);
        obstacleModel->SetCastShadows(true);

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
    cameraNode_ = scene_->CreateChild("Camera");
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));

    inputMap_ = InputMap::Load(context_, "Input/MoveAndOrbit.inputmap");
}

void AdvancedNetworking::CreateUI()
{
    Node* node = scene_->CreateChild("UI");
    ui_ = node->CreateComponent<AdvancedNetworkingUI>();

    if (GetSubsystem<Engine>()->IsHeadless())
        return;

    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();
    UIElement* root = GetUIRoot();
    auto* uiStyle = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    auto* graphics = GetSubsystem<Graphics>();
    // Construct the instructions text element
    instructionsText_ = GetUIRoot()->CreateChild<Text>();
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

    statsText_ = GetUIRoot()->CreateChild<Text>();
    statsText_->SetText("No network stats");
    statsText_->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    statsText_->SetHorizontalAlignment(HA_LEFT);
    statsText_->SetVerticalAlignment(VA_CENTER);
    statsText_->SetPosition(10, -10);
}

void AdvancedNetworking::SetupViewport()
{
    if (GetSubsystem<Engine>()->IsHeadless())
        return;

    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}

void AdvancedNetworking::SubscribeToEvents()
{
    // Subscribe to raycast events.
    SubscribeToEvent(E_ADVANCEDNETWORKING_RAYCAST,
        [this](VariantMap& eventData)
    {
        using namespace AdvancedNetworkingRaycast;
        ServerRaycastInfo info;

        info.clientConnection_.Reset(static_cast<Connection*>(eventData[RemoteEventData::P_CONNECTION].GetPtr()));
        info.origin_ = eventData[P_ORIGIN].GetVector3();
        info.target_ = eventData[P_TARGET].GetVector3();
        info.replicaTime_ = NetworkTime{static_cast<NetworkFrame>(eventData[P_REPLICA_FRAME].GetInt64()), eventData[P_REPLICA_SUBFRAME].GetFloat()};
        info.inputTime_ = NetworkTime{static_cast<NetworkFrame>(eventData[P_INPUT_FRAME].GetInt64()), eventData[P_INPUT_SUBFRAME].GetFloat()};

        serverRaycasts_.push_back(info);
    });

    // Subscribe to rayhit events.
    SubscribeToEvent(E_ADVANCEDNETWORKING_RAYHIT,
        [this](VariantMap& eventData)
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
        [this]
    {
        ProcessRaycastsOnServer();
        if (!GetSubsystem<Engine>()->IsHeadless())
        {
            MoveCamera();
            UpdateStats();
        }
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
    auto prefab = cache->GetResource<PrefabResource>("Prefabs/AdvancedNetworkingPlayer.prefab");

    // Instantiate most of the components from prefab so they will be replicated on the client.
    const Vector3 position{Random(20.0f) - 10.0f, 5.0f, Random(20.0f) - 10.0f};
    Node* playerNode = scene_->InstantiatePrefab(prefab->GetNodePrefab(), position, Quaternion::IDENTITY);

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
    const bool isCameraMoving = input->GetMouseButtonDown(MOUSEB_RIGHT);
    SetMouseMode(isCameraMoving ? MM_RELATIVE : MM_FREE);
    SetMouseVisible(!isCameraMoving);

    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch and only move the camera
    // when the cursor is hidden
    if (isCameraMoving)
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

        bool autoClick = false;
        if (ui_->GetCheatAutoClick())
        {
            autoClick = autoClickTimer_.GetMSec(false) >= 250;
            if (autoClick)
                autoClickTimer_.Reset();
        }

        if (input->GetMouseButtonPress(MOUSEB_LEFT) || autoClick)
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
    const bool autoMovement = ui_->GetCheatAutoMovementCircle();
    if (autoMovement && autoMovementTimer_.GetMSec(false) >= 1000)
    {
        autoMovementTimer_.Reset();
        autoMovementPhase_ = (autoMovementPhase_ + 1) % 4;
    }

    // Calculate movement direction
    const Quaternion rotation(0.0f, yaw_, 0.0f);
    Vector3 direction;
    if (inputMap_)
    {
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_FORWARD) > 0.5f
            || (autoMovement && autoMovementPhase_ == 3))
            direction += rotation * Vector3::FORWARD;
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_BACK) > 0.5f
            || (autoMovement && autoMovementPhase_ == 1))
            direction += rotation * Vector3::BACK;
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_LEFT) > 0.5f
            || (autoMovement && autoMovementPhase_ == 2))
            direction += rotation * Vector3::LEFT;
        if (inputMap_->Evaluate(MoveAndOrbitController::ACTION_RIGHT) > 0.5f
            || (autoMovement && autoMovementPhase_ == 0))
            direction += rotation * Vector3::RIGHT;
    }
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

Vector3 AdvancedNetworking::GetAimPosition(const Vector3& playerPosition, const Ray& screenRay) const
{
    if (ui_->GetCheatAutoAimHand())
    {
        ea::vector<AnimatedModel*> models;
        scene_->GetComponents(models, true);

        AnimatedModel* closestModel{};
        float closestDistance{};
        for (AnimatedModel* model : models)
        {
            if (model->GetViewMask() == UNIMPORTANT_VIEW_MASK)
                continue;

            const float distance = model->GetNode()->GetWorldPosition().DistanceToPoint(playerPosition);
            if (!closestModel || distance < closestDistance)
            {
                closestModel = model;
                closestDistance = distance;
            }
        }

        if (closestModel)
        {
            Node* aimNode = closestModel->GetSkeleton().GetBone("Mutant:RightHandIndex2")->node_;
            return aimNode->GetWorldPosition();
        }
    }

    const Vector3 defaultAimPosition = screenRay.origin_ + screenRay.direction_ * HIT_DISTANCE;
    return RaycastImportantGeometries(screenRay).value_or(defaultAimPosition);
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

    // Second, perform an actual raycast from the player model to the aim point.
    const Vector3 aimPosition = GetAimPosition(clientObject->GetNode()->GetWorldPosition(), screenRay);
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
    eventData[P_REPLICA_FRAME] = static_cast<long long>(replicaTime.Frame());
    eventData[P_REPLICA_SUBFRAME] = replicaTime.Fraction();
    eventData[P_INPUT_FRAME] = static_cast<long long>(inputTime.Frame());
    eventData[P_INPUT_SUBFRAME] = inputTime.Fraction();
    serverConnection->SendRemoteEvent(E_ADVANCEDNETWORKING_RAYCAST, false, eventData);
}

void AdvancedNetworking::AddHitMarker(const Vector3& position, bool isConfirmed)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Prevent overflow
    if (hitMarkers_->GetNumChildren() >= 200)
        hitMarkers_->GetChild(0u)->Remove();

    // Add new marker: red sphere for client hits, green cube for server confirmations
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
        markerModel->SetMaterial(cache->GetResource<Material>("Materials/Constant/GlowingRed.xml"));
    }
}

void AdvancedNetworking::UpdateStats()
{
    if (statsTimer_.GetMSec(false) >= 333)
    {
        statsTimer_.Reset();
        auto network = GetSubsystem<Network>();
        statsText_->SetText(network->GetDebugInfo());
    }
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
