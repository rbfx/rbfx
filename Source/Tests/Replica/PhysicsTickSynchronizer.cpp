//
// Copyright (c) 2017-2021 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

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
