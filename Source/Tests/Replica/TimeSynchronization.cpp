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
#include <Urho3D/Replica/ReplicationManager.h>

TEST_CASE("Time is synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    // Prepare test parameters
    const unsigned fps = Tests::NetworkSimulator::FramesInSecond;

    const float frameErrorTolarance = 0.1f;
    const auto retry = GENERATE(range(0, 5));
    const auto quality = GENERATE(
        Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f },
        Tests::ConnectionQuality{ 0.24f, 0.28f, 0.50f, 0.10f, 0.10f }
    );

    const unsigned initialSyncTime = 10;
    const unsigned initialWaitTime = 30;
    const unsigned forwardSyncTime = 10;
    const unsigned forwardWaitTime = 30;
    const unsigned backwardSyncTime = 10;
    const unsigned backwardWaitTime = 30;

    unsigned seed = retry;
    CombineHash(seed, MakeHash(quality.minPing_));
    CombineHash(seed, MakeHash(quality.maxPing_));
    CombineHash(seed, MakeHash(quality.spikePing_));

    // Setup scenes
    auto serverScene = MakeShared<Scene>(context);
    auto clientScene = MakeShared<Scene>(context);

    Tests::NetworkSimulator sim(serverScene, seed);
    sim.AddClient(clientScene, quality);

    auto& serverReplicator = *serverScene->GetComponent<ReplicationManager>()->GetServerReplicator();

    // Simulate a few millseconds, not enough for synchronization due to ping
    sim.SimulateTime(0.5f);
    REQUIRE_FALSE(clientScene->GetComponent<ReplicationManager>()->GetClientReplica());

    // Simulate a few more seconds, should be somehow synchronized
    sim.SimulateTime(0.5f);
    sim.SimulateTime(9.0f);

    REQUIRE(clientScene->GetComponent<ReplicationManager>()->GetClientReplica());
    const auto& clientReplica = *clientScene->GetComponent<ReplicationManager>()->GetClientReplica();

    const float syncError = ea::max(0.5f, (quality.maxPing_ - quality.minPing_) * Tests::NetworkSimulator::FramesInSecond);
    const auto startTime = fps * 10;
    REQUIRE(serverReplicator.GetCurrentFrame() == NetworkFrame{startTime});
    REQUIRE(std::abs(clientReplica.GetServerTime() - NetworkTime{NetworkFrame{startTime}}) < syncError);

    // Simulate some time, should be precisely synchronized afterwards
    sim.SimulateTime(initialSyncTime);
    REQUIRE(serverReplicator.GetCurrentFrame() == NetworkFrame{startTime + fps * initialSyncTime});
    REQUIRE(std::abs(clientReplica.GetServerTime() - NetworkTime{NetworkFrame{startTime + fps * initialSyncTime}}) < frameErrorTolarance);

    // Simulate more time, expect time to stay synchronized
    const NetworkFrame syncFrame1 = clientReplica.GetLatestScaledInputTime().Frame();
    sim.SimulateTime(initialWaitTime);

    REQUIRE(serverReplicator.GetCurrentFrame() == NetworkFrame{startTime + fps * (initialSyncTime + initialWaitTime)});
    REQUIRE(std::abs(clientReplica.GetServerTime() - NetworkTime{NetworkFrame{startTime + fps * (initialSyncTime + initialWaitTime)}}) < frameErrorTolarance);
    REQUIRE(clientReplica.GetLatestScaledInputTime().Frame() == syncFrame1);

    // Warp time close to 2^32 and simulate some time, expect time to be resynchronized
    const long long bigTime = M_MAX_UNSIGNED - fps * 30;
    serverReplicator.SetCurrentFrame(NetworkFrame{bigTime / 3});
    sim.SimulateTime(5.0f);
    serverReplicator.SetCurrentFrame(NetworkFrame{bigTime / 3 * 2});
    sim.SimulateTime(5.0f);
    serverReplicator.SetCurrentFrame(NetworkFrame{bigTime});
    sim.SimulateTime(forwardSyncTime);

    REQUIRE(serverReplicator.GetCurrentFrame() == NetworkFrame{bigTime + fps * forwardSyncTime});
    REQUIRE(std::abs(clientReplica.GetServerTime() - NetworkTime{NetworkFrame{bigTime + fps * forwardSyncTime}}) < frameErrorTolarance);

    // Simulate more time, expect time to stay synchronized
    const NetworkFrame syncFrame2 = clientReplica.GetLatestScaledInputTime().Frame();
    sim.SimulateTime(forwardWaitTime);

    REQUIRE(serverReplicator.GetCurrentFrame() == NetworkFrame{bigTime + fps * (forwardSyncTime + forwardWaitTime)});
    REQUIRE(std::abs(clientReplica.GetServerTime() - NetworkTime{NetworkFrame{bigTime + fps * (forwardSyncTime + forwardWaitTime)}}) < frameErrorTolarance);
    REQUIRE(clientReplica.GetLatestScaledInputTime().Frame() == syncFrame2);

    // Warp time 1 second back and simulate some time, expect time to be resynchronized
    const long long baseTime = bigTime + fps * (forwardSyncTime + forwardWaitTime);
    serverReplicator.SetCurrentFrame(NetworkFrame{baseTime - fps * 1});
    sim.SimulateTime(backwardSyncTime + 1);

    REQUIRE(serverReplicator.GetCurrentFrame() == NetworkFrame{baseTime + fps * backwardSyncTime});
    REQUIRE(std::abs(clientReplica.GetServerTime() - NetworkTime{NetworkFrame{baseTime + fps * backwardSyncTime}}) < frameErrorTolarance);

    // Simulate more time, expect time to stay synchronized
    const NetworkFrame syncFrame3 = clientReplica.GetLatestScaledInputTime().Frame();
    sim.SimulateTime(backwardWaitTime);

    REQUIRE(serverReplicator.GetCurrentFrame() == NetworkFrame{baseTime + fps * (backwardSyncTime + backwardWaitTime)});
    REQUIRE(std::abs(clientReplica.GetServerTime() - NetworkTime{NetworkFrame{baseTime + fps * (backwardSyncTime + backwardWaitTime)}}) < frameErrorTolarance);
    //REQUIRE(clientReplica.GetLatestScaledInputTime().Frame() == syncFrame3);
}
