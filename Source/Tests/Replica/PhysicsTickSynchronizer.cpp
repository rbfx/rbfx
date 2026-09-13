// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../NetworkUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

TEST_CASE("Physics is synchronized with network updates")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Simulate some time before scene creation so network is not synchronized with scene
    Tests::NetworkSimulator::SimulateEngineFrame(context, 0.01234f);

    // Start simulation and track events
    auto serverScene = MakeShared<Scene>(context);
    auto serverPhysicsWorld = serverScene->CreateComponent<PhysicsWorld>();
    serverPhysicsWorld->SetFps(64);

    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    Tests::NetworkSimulator sim(serverScene);

    sim.SimulateTime(1.0f);

    // Add client and wait for synchronization
    SharedPtr<Scene> clientScene = MakeShared<Scene>(context);
    auto clientPhysicsWorld = clientScene->CreateComponent<PhysicsWorld>();
    clientPhysicsWorld->SetFps(64);

    sim.AddClient(clientScene, quality);
    sim.SimulateTime(10.0f);

    auto serverEventTracker = MakeShared<Tests::FrameEventTracker>(context);
    serverEventTracker->TrackEvent(serverPhysicsWorld, E_PHYSICSPRESTEP);
    serverEventTracker->TrackEvent(E_BEGINSERVERNETWORKFRAME);
    serverEventTracker->TrackEvent(E_ENDSERVERNETWORKFRAME);
    serverEventTracker->TrackEvent(serverScene, E_SCENENETWORKUPDATE);

    auto clientEventTracker = MakeShared<Tests::FrameEventTracker>(context);
    clientEventTracker->TrackEvent(clientPhysicsWorld, E_PHYSICSPRESTEP);
    clientEventTracker->TrackEvent(E_BEGINCLIENTNETWORKFRAME);
    clientEventTracker->TrackEvent(E_ENDCLIENTNETWORKFRAME);
    clientEventTracker->TrackEvent(clientScene, E_SCENENETWORKUPDATE);

    sim.SimulateTime(1.0f);
    serverEventTracker->SkipFramesUntilEvent(E_ENDSERVERNETWORKFRAME);
    clientEventTracker->SkipFramesUntilEvent(E_BEGINCLIENTNETWORKFRAME, 2);

    REQUIRE(serverEventTracker->GetNumFrames() > 4);
    REQUIRE(clientEventTracker->GetNumFrames() > 4);

    serverEventTracker->ValidatePattern({{E_BEGINSERVERNETWORKFRAME, E_SCENENETWORKUPDATE, E_PHYSICSPRESTEP, E_PHYSICSPRESTEP, E_ENDSERVERNETWORKFRAME}, {}, {}, {}});
    clientEventTracker->ValidatePattern({{E_BEGINCLIENTNETWORKFRAME, E_SCENENETWORKUPDATE, E_PHYSICSPRESTEP, E_ENDCLIENTNETWORKFRAME}, {E_SCENENETWORKUPDATE}, {E_SCENENETWORKUPDATE, E_PHYSICSPRESTEP}, {E_SCENENETWORKUPDATE}});
}
