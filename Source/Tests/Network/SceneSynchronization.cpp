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

#include <Urho3D/Network/DefaultNetworkObject.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkManager.h>
#include <Urho3D/Network/NetworkObject.h>
#include <Urho3D/Network/NetworkValue.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

TEST_CASE("Time is synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Prepare test parameters
    const float frameErrorTolarance = 1.0f;
    const auto retry = GENERATE(range(0, 5));
    const auto quality = GENERATE(
        Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f },
        Tests::ConnectionQuality{ 0.22f, 0.28f, 0.50f, 0.10f, 0.10f }
    );
    const float averagePingSec = (quality.maxPing_ + quality.minPing_) / 2;

    unsigned seed = retry;
    CombineHash(seed, MakeHash(quality.minPing_));
    CombineHash(seed, MakeHash(quality.maxPing_));
    CombineHash(seed, MakeHash(quality.spikePing_));

    // Setup scenes
    auto serverScene = MakeShared<Scene>(context);
    auto clientScene = MakeShared<Scene>(context);

    Tests::NetworkSimulator sim(serverScene, seed);
    sim.AddClient(clientScene, quality);

    auto& serverNetworkManager = serverScene->GetNetworkManager()->AsServer();
    auto& clientNetworkManager = clientScene->GetNetworkManager()->AsClient();

    // Simulate a few millseconds, not enough for synchronization due to ping
    sim.SimulateTime(504 / 1024.0f);
    REQUIRE_FALSE(clientNetworkManager.IsSynchronized());

    // Simulate a few more seconds, should be somehow synchronized
    sim.SimulateTime(520 / 1024.0f);
    sim.SimulateTime(9.0f);

    REQUIRE(clientNetworkManager.IsSynchronized());
    REQUIRE(clientNetworkManager.GetPingInMs() == RoundToInt(averagePingSec * 1000));

    const float syncError = ea::max(0.5f, (quality.maxPing_ - quality.minPing_) * Tests::NetworkSimulator::FramesInSecond);
    const auto startTime = 32 * 10;
    REQUIRE(serverNetworkManager.GetCurrentFrame() == startTime);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(startTime)) < syncError);

    // Simulate 10 more seconds, should be precisely synchronized afterwards
    sim.SimulateTime(10.0f);
    REQUIRE(serverNetworkManager.GetCurrentFrame() == startTime + 32 * 10);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(startTime + 32 * 10)) < frameErrorTolarance);

    // Simulate 50 more seconds, expect time to stay synchronized
    const unsigned syncFrame1 = clientNetworkManager.GetLastSynchronizationFrame();
    sim.SimulateTime(50.0f);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == startTime + 32 * 60);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(startTime + 32 * 60)) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() == syncFrame1);

    // Warp time close to 2^32 and simulate 10 more seconds, expect time to be resynchronized
    const auto bigTime = M_MAX_UNSIGNED - 32 * 20;
    serverNetworkManager.SetCurrentFrame(bigTime / 3);
    sim.SimulateTime(1.0f);
    serverNetworkManager.SetCurrentFrame(bigTime / 3 * 2);
    sim.SimulateTime(1.0f);
    serverNetworkManager.SetCurrentFrame(bigTime);
    sim.SimulateTime(10.0f);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == bigTime + 32 * 10);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(bigTime + 32 * 10)) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() >= bigTime);

    // Simulate 50 more seconds, expect time to stay synchronized
    const unsigned syncFrame2 = clientNetworkManager.GetLastSynchronizationFrame();
    sim.SimulateTime(50.0f);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == bigTime + 32 * 60);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(bigTime + 32 * 60)) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() == syncFrame2);

    // Warp time 1 second back and simulate 11 seconds, expect time to be resynchronized
    serverNetworkManager.SetCurrentFrame(bigTime + 32 * 59);
    sim.SimulateTime(11.0f);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == bigTime + 32 * 70);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(bigTime + 32 * 70)) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() >= bigTime + 32 * 59);

    // Simulate 10 more seconds, expect time to stay synchronized
    const unsigned syncFrame3 = clientNetworkManager.GetLastSynchronizationFrame();
    sim.SimulateTime(50.0f);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == bigTime + 32 * 120);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(bigTime + 32 * 120)) < frameErrorTolarance);
    REQUIRE(clientNetworkManager.GetLastSynchronizationFrame() == syncFrame3);
}

TEST_CASE("Scene is synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);
    const float syncDelay = 0.25f;

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    // Reference transforms, should stay the same
    Matrix3x4 transformReplicatedNodeA;
    Matrix3x4 transformReplicatedNodeB;
    Matrix3x4 transformReplicatedNodeChild1;
    Matrix3x4 transformReplicatedNodeChild2;
    Matrix3x4 transformReplicatedNodeChild4;

    {
        for (Scene* clientScene : clientScenes)
            clientScene->CreateChild("Client Only Node");
        auto serverOnlyNode = serverScene->CreateChild("Server Only Node");

        auto replicatedNodeA = serverScene->CreateChild("Replicated Node A");
        replicatedNodeA->CreateComponent<DefaultNetworkObject>();
        replicatedNodeA->SetScale(2.0f);
        transformReplicatedNodeA = replicatedNodeA->GetWorldTransform();

        auto replicatedNodeB = serverScene->CreateChild("Replicated Node B");
        replicatedNodeB->CreateComponent<DefaultNetworkObject>();
        replicatedNodeB->SetPosition({ -1.0f, 2.0f, 0.5f });
        transformReplicatedNodeB = replicatedNodeB->GetWorldTransform();

        auto replicatedNodeChild1 = replicatedNodeA->CreateChild("Replicated Node Child 1");
        replicatedNodeChild1->CreateComponent<DefaultNetworkObject>();
        replicatedNodeChild1->SetPosition({ -2.0f, 3.0f, 1.5f });
        transformReplicatedNodeChild1 = replicatedNodeChild1->GetWorldTransform();

        auto replicatedNodeChild2 = replicatedNodeChild1->CreateChild("Replicated Node Child 2");
        replicatedNodeChild2->CreateComponent<DefaultNetworkObject>();
        replicatedNodeChild2->SetRotation({ 90.0f, Vector3::UP });
        transformReplicatedNodeChild2 = replicatedNodeChild2->GetWorldTransform();

        auto serverOnlyChild3 = replicatedNodeB->CreateChild("Server Only Child 3");
        serverOnlyChild3->SetPosition({ -1.0f, 0.0f, 0.0f });

        auto replicatedNodeChild4 = serverOnlyChild3->CreateChild("Replicated Node Child 4");
        replicatedNodeChild4->CreateComponent<DefaultNetworkObject>();
        transformReplicatedNodeChild4 = replicatedNodeChild4->GetWorldTransform();
    }

    // Spend some time alone
    Tests::NetworkSimulator sim(serverScene);
    sim.SimulateTime(10.0f);

    // Add clients and wait for synchronization
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);
    sim.SimulateTime(10.0f);

    for (Scene* clientScene : clientScenes)
    {
        auto clientOnlyNode = clientScene->GetChild("Client Only Node", true);
        auto replicatedNodeA = clientScene->GetChild("Replicated Node A", true);
        auto replicatedNodeB = clientScene->GetChild("Replicated Node B", true);
        auto replicatedNodeChild1 = clientScene->GetChild("Replicated Node Child 1", true);
        auto replicatedNodeChild2 = clientScene->GetChild("Replicated Node Child 2", true);
        auto replicatedNodeChild4 = clientScene->GetChild("Replicated Node Child 4", true);

        REQUIRE(clientScene->GetNumChildren() == 3);
        REQUIRE(clientScene == clientOnlyNode->GetParent());
        REQUIRE(clientScene == replicatedNodeA->GetParent());
        REQUIRE(clientScene == replicatedNodeB->GetParent());

        REQUIRE(clientOnlyNode->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetNumChildren() == 1);
        REQUIRE(replicatedNodeA == replicatedNodeChild1->GetParent());

        REQUIRE(replicatedNodeChild1->GetNumChildren() == 1);
        REQUIRE(replicatedNodeChild1 == replicatedNodeChild2->GetParent());

        REQUIRE(replicatedNodeChild2->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetNumChildren() == 1);
        REQUIRE(replicatedNodeB == replicatedNodeChild4->GetParent());

        REQUIRE(replicatedNodeChild4->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetWorldTransform().Equals(transformReplicatedNodeA));
        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2));
        REQUIRE(replicatedNodeChild4->GetWorldTransform().Equals(transformReplicatedNodeChild4));
    }

    // Re-parent "Server Only Child 3" to "Replicated Node A"
    // Re-parent "Replicated Node Child 1" to Scene
    // Wait for synchronization
    {
        auto serverOnlyChild3 = serverScene->GetChild("Server Only Child 3", true);
        auto replicatedNodeA = serverScene->GetChild("Replicated Node A", true);
        auto replicatedNodeChild1 = serverScene->GetChild("Replicated Node Child 1", true);

        serverOnlyChild3->SetParent(replicatedNodeA);
        replicatedNodeChild1->SetParent(serverScene);
    }

    sim.SimulateTime(syncDelay);

    for (Scene* clientScene : clientScenes)
    {
        auto clientOnlyNode = clientScene->GetChild("Client Only Node", true);
        auto replicatedNodeA = clientScene->GetChild("Replicated Node A", true);
        auto replicatedNodeB = clientScene->GetChild("Replicated Node B", true);
        auto replicatedNodeChild1 = clientScene->GetChild("Replicated Node Child 1", true);
        auto replicatedNodeChild2 = clientScene->GetChild("Replicated Node Child 2", true);
        auto replicatedNodeChild4 = clientScene->GetChild("Replicated Node Child 4", true);

        REQUIRE(clientScene->GetNumChildren() == 4);
        REQUIRE(clientScene == clientOnlyNode->GetParent());
        REQUIRE(clientScene == replicatedNodeA->GetParent());
        REQUIRE(clientScene == replicatedNodeB->GetParent());
        REQUIRE(clientScene == replicatedNodeChild1->GetParent());

        REQUIRE(clientOnlyNode->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetNumChildren() == 1);
        REQUIRE(replicatedNodeA == replicatedNodeChild4->GetParent());

        REQUIRE(replicatedNodeChild4->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetNumChildren() == 0);

        REQUIRE(replicatedNodeChild1->GetNumChildren() == 1);
        REQUIRE(replicatedNodeChild1 == replicatedNodeChild2->GetParent());

        REQUIRE(replicatedNodeChild2->GetNumChildren() == 0);

        REQUIRE(replicatedNodeA->GetWorldTransform().Equals(transformReplicatedNodeA));
        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2));
        REQUIRE(replicatedNodeChild4->GetWorldTransform().Equals(transformReplicatedNodeChild4));
    }

    // Remove "Replicated Node A"
    // Add "Replicated Node C"
    {
        auto replicatedNodeA = serverScene->GetChild("Replicated Node A", true);
        replicatedNodeA->Remove();
        auto replicatedNodeC = serverScene->CreateChild("Replicated Node C");
        replicatedNodeC->CreateComponent<DefaultNetworkObject>();
    }

    sim.SimulateTime(syncDelay);

    for (Scene* clientScene : clientScenes)
    {
        auto clientOnlyNode = clientScene->GetChild("Client Only Node", true);
        auto replicatedNodeB = clientScene->GetChild("Replicated Node B", true);
        auto replicatedNodeC = clientScene->GetChild("Replicated Node C", true);
        auto replicatedNodeChild1 = clientScene->GetChild("Replicated Node Child 1", true);
        auto replicatedNodeChild2 = clientScene->GetChild("Replicated Node Child 2", true);

        REQUIRE(clientScene->GetNumChildren() == 4);
        REQUIRE(clientScene == clientOnlyNode->GetParent());
        REQUIRE(clientScene == replicatedNodeB->GetParent());
        REQUIRE(clientScene == replicatedNodeC->GetParent());
        REQUIRE(clientScene == replicatedNodeChild1->GetParent());

        REQUIRE(clientOnlyNode->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetNumChildren() == 0);

        REQUIRE(replicatedNodeChild1->GetNumChildren() == 1);
        REQUIRE(replicatedNodeChild1 == replicatedNodeChild2->GetParent());

        REQUIRE(replicatedNodeChild2->GetNumChildren() == 0);

        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB));
        REQUIRE(replicatedNodeC->GetWorldTransform().Equals(Matrix3x4::IDENTITY));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2));
    }

    sim.SimulateTime(1.0f);
}

TEST_CASE("Position and rotation are synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0, 0 };
    const float moveSpeedNodeA = 1.0f;
    const float rotationSpeedNodeA = 10.0f;
    const float moveSpeedNodeB = 0.1f;
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    auto serverNodeA = serverScene->CreateChild("Node");
    auto serverObjectA = serverNodeA->CreateComponent<DefaultNetworkObject>();

    auto serverNodeB = serverNodeA->CreateChild("Node Child");
    auto serverObjectB = serverNodeB->CreateComponent<DefaultNetworkObject>();
    serverNodeB->SetPosition({ 0.0f, 0.0f, 1.0f });

    // Animate objects forever
    serverScene->SubscribeToEvent(serverScene, E_SCENEUPDATE, [&](StringHash, VariantMap& eventData)
    {
        const float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        serverNodeA->Translate(timeStep * moveSpeedNodeA * Vector3::LEFT, TS_PARENT);
        serverNodeA->Rotate({ timeStep * rotationSpeedNodeA, Vector3::UP }, TS_PARENT);
        serverNodeB->Translate(timeStep * moveSpeedNodeB * Vector3::FORWARD, TS_PARENT);
    });

    // Spend some time alone
    Tests::NetworkSimulator sim(serverScene);
    const auto& serverNetworkManager = serverScene->GetNetworkManager()->AsServer();
    sim.SimulateTime(9.0f);

    // Add clients and wait for synchronization
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);
    sim.SimulateTime(9.0f);

    // Expect positions and rotations to be precisely synchronized
    const float expectedDelay = 0.2;
    for (Scene* clientScene : clientScenes)
    {
        const NetworkTime clientTime = clientScene->GetNetworkManager()->AsClient().GetClientTime();
        const double delay = serverNetworkManager.GetServerTime() - clientTime;

        auto clientNodeA = clientScene->GetChild("Node", true);
        auto clientNodeB = clientScene->GetChild("Node Child", true);

        REQUIRE(delay / Tests::NetworkSimulator::FramesInSecond == Catch::Approx(expectedDelay).margin(0.01));

        REQUIRE(serverObjectA->GetTemporalWorldPosition(clientTime).Equals(clientNodeA->GetWorldPosition(), M_LARGE_EPSILON));
        REQUIRE(serverObjectA->GetTemporalWorldRotation(clientTime).Equals(clientNodeA->GetWorldRotation(), M_LARGE_EPSILON));

        REQUIRE(serverObjectB->GetTemporalWorldPosition(clientTime).Equals(clientNodeB->GetWorldPosition(), M_LARGE_EPSILON));
        REQUIRE(serverObjectB->GetTemporalWorldRotation(clientTime).Equals(clientNodeB->GetWorldRotation(), M_LARGE_EPSILON));
    }
}
