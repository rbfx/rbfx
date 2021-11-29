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
#include <Urho3D/Network/NetworkObject.h>
#include <Urho3D/Network/NetworkManager.h>
#include <Urho3D/Scene/Scene.h>

TEST_CASE("Time is synchronized between client and server")
{
    auto context = URHO3D_GET_TEST_CONTEXT(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Prepare test parameters
    const float frameErrorTolarance = 1.0f;
    const auto retry = GENERATE(range(0, 5));
    const auto quality = GENERATE(
        Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f },
        Tests::ConnectionQuality{ 0.22f, 0.28f, 0.50f, 0.10f, 0.10f }
    );

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
    sim.SimulateTime(24 / 1024.0f);
    REQUIRE_FALSE(clientNetworkManager.IsSynchronized());

    // Simulate the rest of a second, should be somehow synchronized
    sim.SimulateTime(1000 / 1024.0f);

    REQUIRE(clientNetworkManager.IsSynchronized());
    REQUIRE(clientNetworkManager.GetPingInMs() == RoundToInt((quality.maxPing_ + quality.minPing_) * 1000 / 2));

    const float syncError = ea::max(0.5f, (quality.maxPing_ - quality.minPing_) * Tests::NetworkSimulator::FramesInSecond);
    REQUIRE(serverNetworkManager.GetCurrentFrame() == 32);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(32)) < syncError);

    // Simulate 9 more seconds, should be precisely synchronized afterwards
    sim.SimulateTime(9.0f);
    REQUIRE(serverNetworkManager.GetCurrentFrame() == 32 * 10);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(32 * 10)) < frameErrorTolarance);

    // Simulate 50 more seconds, expect time to stay synchronized
    const unsigned syncFrame1 = clientNetworkManager.GetLastSynchronizationFrame();
    sim.SimulateTime(50.0f);

    REQUIRE(serverNetworkManager.GetCurrentFrame() == 32 * 60);
    REQUIRE(std::abs(clientNetworkManager.GetCurrentFrameDeltaRelativeTo(32 * 60)) < frameErrorTolarance);
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
    auto context = URHO3D_GET_TEST_CONTEXT(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);
    const float syncDelay = 0.25f;

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    auto serverScene = MakeShared<Scene>(context);
    auto clientScene = MakeShared<Scene>(context);

    {
        auto clientOnlyNode = clientScene->CreateChild("Client Only Node");
        auto serverOnlyNode = serverScene->CreateChild("Server Only Node");
        auto replicatedNodeA = serverScene->CreateChild("Replicated Node A");
        replicatedNodeA->CreateComponent<DefaultNetworkObject>();
        auto replicatedNodeB = serverScene->CreateChild("Replicated Node B");
        replicatedNodeB->CreateComponent<DefaultNetworkObject>();
        auto replicatedNodeChild1 = replicatedNodeA->CreateChild("Replicated Node Child 1");
        replicatedNodeChild1->CreateComponent<DefaultNetworkObject>();
        auto replicatedNodeChild2 = replicatedNodeChild1->CreateChild("Replicated Node Child 2");
        replicatedNodeChild2->CreateComponent<DefaultNetworkObject>();
        auto serverOnlyChild3 = replicatedNodeB->CreateChild("Server Only Child 3");
        auto replicatedNodeChild4 = serverOnlyChild3->CreateChild("Replicated Node Child 4");
        replicatedNodeChild4->CreateComponent<DefaultNetworkObject>();
    }

    // Spend some time alone
    Tests::NetworkSimulator sim(serverScene);
    sim.SimulateTime(10.0f);

    // Add client and wait for synchronization
    sim.AddClient(clientScene, quality);
    sim.SimulateTime(10.0f);

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
    }

    sim.SimulateTime(1.0f);

}
