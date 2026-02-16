// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/FreeFlyController.h>
#include <Urho3D/Math/RandomEngine.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Replica/ReplicatedAnimation.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "AdvancedNetworking.h"
#include "AdvancedNetworkingClientConnection.h"
#include "AdvancedNetworkingPlayer.h"
#include "AdvancedNetworkingServer.h"
#include "AdvancedNetworkingUI.h"

#include <Urho3D/DebugNew.h>

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

void AdvancedNetworking::Stop()
{
    StopNetworking();
    BaseClassName::Stop();
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

    freeFlyController_ = cameraNode_->CreateComponent<FreeFlyController>();
    freeFlyController_->SetEnabled(false);
}

void AdvancedNetworking::CreateUI()
{
    Node* node = scene_->CreateChild("UI");
    ui_ = node->CreateComponent<AdvancedNetworkingUI>();
    ui_->OnStartServer.Subscribe(this, &AdvancedNetworking::StartServer);
    ui_->OnConnectToServer.Subscribe(this, &AdvancedNetworking::ConnectToServer);
    ui_->OnStop.Subscribe(this, &AdvancedNetworking::StopNetworking);
    ui_->SetServerRunning(false);
    ui_->SetClientConnected(false);

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
    // Subscribe to PostUpdate instead of the usual Update so that physics simulation has
    // already proceeded for the frame, and can accurately follow the object with the camera
    if (!GetSubsystem<Engine>()->IsHeadless())
        SubscribeToEvent(E_POSTUPDATE, &AdvancedNetworking::UpdateStats);
}

void AdvancedNetworking::UpdateStats()
{
    if (statsTimer_.GetMSec(false) >= 333)
    {
        statsTimer_.Reset();
        if (IsClientConnected())
            statsText_->SetText("Client connected");
        else if (IsServerRunning())
            statsText_->SetText(Format("Server running, clients: {}", server_->GetClientCount()));
        else
            statsText_->SetText("No network stats");
    }
}

void AdvancedNetworking::StartServer(unsigned short port)
{
    StopNetworking();

    server_ = MakeShared<AdvancedNetworkingServer>(scene_);
    server_->onListenStart_.Subscribe(this, [this]() { HandleClientConnectionState(false); });
    server_->onListenStop_.Subscribe(this, [this]() { HandleClientConnectionState(false); });
    auto* replicationManager = scene_->GetComponent<ReplicationManager>();
    replicationManager->StartServer();
    ui_->SetServerRunning(server_->Start(port));
}

void AdvancedNetworking::ConnectToServer(const ea::string& address, unsigned short port)
{
    StopNetworking();

    clientConnection_ = MakeShared<AdvancedNetworkingClientConnection>(scene_, ui_);
    clientConnection_->onConnected_.Subscribe(this, [this]() { HandleClientConnectionState(true); });
    clientConnection_->onDisconnected_.Subscribe(this, [this]() { HandleClientConnectionState(false); });
    freeFlyController_->SetEnabled(false);
    ui_->SetClientConnected(clientConnection_->Connect(address, port));
}

void AdvancedNetworking::StopNetworking()
{
    auto* replicationManager = scene_->GetComponent<ReplicationManager>();

    if (clientConnection_)
    {
        clientConnection_->Disconnect();
        clientConnection_ = nullptr;
    }

    if (server_)
    {
        server_->Stop();
        server_ = nullptr;
        replicationManager->StartStandalone();
    }

    ui_->SetServerRunning(false);
    ui_->SetClientConnected(false);
    freeFlyController_->SetEnabled(false);
}

bool AdvancedNetworking::IsServerRunning() const
{
    return server_ && server_->IsListening();
}

bool AdvancedNetworking::IsClientConnected() const
{
    return clientConnection_ && clientConnection_->IsConnected();
}

void AdvancedNetworking::HandleClientConnectionState(bool connected)
{
    ui_->SetClientConnected(connected);
    freeFlyController_->SetEnabled(IsServerRunning() && !connected);
}
