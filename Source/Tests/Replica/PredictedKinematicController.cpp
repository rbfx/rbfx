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
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/KinematicCharacterController.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Replica/ClientInputStatistics.h>
#include <Urho3D/Replica/BehaviorNetworkObject.h>
#include <Urho3D/Replica/PredictedKinematicController.h>
#include <Urho3D/Replica/ReplicatedTransform.h>
#include <Urho3D/Scene/PrefabResource.h>

namespace
{

SharedPtr<Scene> CreateTestScene(Context* context)
{
    auto serverScene = MakeShared<Scene>(context);
    auto physicsWorld = serverScene->CreateComponent<PhysicsWorld>();
    physicsWorld->SetFps(Tests::NetworkSimulator::FramesInSecond * 2);

    Node* floorNode = serverScene->CreateChild("Floor");

    auto floorShape = floorNode->CreateComponent<CollisionShape>();
    floorShape->SetStaticPlane();

    auto floorBody = floorNode->CreateComponent<RigidBody>();
    return serverScene;
}

SharedPtr<PrefabResource> CreateTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);

    auto kinematicController = node->CreateComponent<KinematicCharacterController>();
    kinematicController->SetHeight(2.0f);
    kinematicController->SetOffset({0.0f, 1.0f, 0.0f});

    node->CreateComponent<ReplicatedTransform>();
    node->CreateComponent<PredictedKinematicController>();

    return Tests::ConvertNodeToPrefab(node);
}

}

TEST_CASE("Client input quality is evaluated")
{
    ClientInputStatistics stats(10, 8);

    stats.OnInputReceived(NetworkFrame{1001});
    stats.OnInputReceived(NetworkFrame{1002});
    stats.OnInputReceived(NetworkFrame{1004});
    stats.OnInputReceived(NetworkFrame{1005});
    stats.OnInputReceived(NetworkFrame{1007});
    stats.OnInputReceived(NetworkFrame{1009});
    stats.OnInputReceived(NetworkFrame{1010});
    REQUIRE(stats.GetRecommendedBufferSize() == 1);

    stats.OnInputReceived(NetworkFrame{1020});
    REQUIRE(stats.GetRecommendedBufferSize() == 1);

    stats.OnInputReceived(NetworkFrame{1023});
    stats.OnInputReceived(NetworkFrame{1024});
    stats.OnInputReceived(NetworkFrame{1026});
    stats.OnInputReceived(NetworkFrame{1030});
    REQUIRE(stats.GetRecommendedBufferSize() == 2);
}

TEST_CASE("Client-side prediction is consistent with server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/PredictedKinematicController/Test.prefab", CreateTestPrefab);

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    //const auto quality = Tests::ConnectionQuality{ 0.01f, 0.01f, 0.01f, 0.0f, 0.0f };

    auto serverScene = CreateTestScene(context);
    auto clientScene = CreateTestScene(context);

    // Start simulation
    Tests::NetworkSimulator sim(serverScene);
    sim.AddClient(clientScene, quality);

    // Create nodes
    Node* serverNode = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Player", {0.0f, 10.0f, 0.0f});
    auto serverObject = serverNode->GetComponent<BehaviorNetworkObject>();
    auto serverController = serverNode->GetComponent<PredictedKinematicController>();
    serverObject->SetOwner(sim.GetServerToClientConnection(clientScene));

    // Wait for synchronization, expect controller on the ground
    sim.SimulateTime(10.0f);
    const auto& serverReplicator = *serverScene->GetComponent<ReplicationManager>()->GetServerReplicator();
    const unsigned inputDelay = serverReplicator.GetFeedbackDelay(sim.GetServerToClientConnection(clientScene));

    Node* clientNode = clientScene->GetChild("Player", true);
    auto clientController = clientNode->GetComponent<PredictedKinematicController>();

    REQUIRE(serverNode->GetWorldPosition().ToXZ() == Vector2::ZERO);
    REQUIRE(serverNode->GetWorldPosition().y_ == Catch::Approx(0.0f).margin(0.1f));

    REQUIRE(clientNode->GetWorldPosition().ToXZ() == Vector2::ZERO);
    REQUIRE(clientNode->GetWorldPosition().y_ == Catch::Approx(0.0f).margin(0.1f));

    // Start movement at some random point, move for about 5 seconds with velocity of 2 units/second
    sim.SimulateTime(0.01f);
    const float moveVelocity = 2.0f;
    clientController->SetWalkVelocity(Vector3::FORWARD * moveVelocity);
    sim.SimulateTime(0.99f);
    sim.SimulateTime(4.0f);

    // Expect specified velocity
    CHECK(clientController->GetVelocity().Equals(Vector3::FORWARD * moveVelocity, 0.02f));
    CHECK(serverController->GetVelocity().Equals(Vector3::FORWARD * moveVelocity, 0.02f));

    // Expect client node at about the specified position.
    const float networkError = 1 * moveVelocity * (1.0f / Tests::NetworkSimulator::FramesInSecond);
    {
        CHECK(clientNode->GetWorldPosition().x_ == 0.0f);
        CHECK(clientNode->GetWorldPosition().z_ == Catch::Approx(10.0f).margin(networkError));
    }

    // Expect server lagging behind, with max error about 1 + ping frames.
    {
        const float serverDelay = inputDelay * moveVelocity * (1.0f / Tests::NetworkSimulator::FramesInSecond);
        CHECK(serverNode->GetWorldPosition().x_ == 0.0f);
        CHECK(serverNode->GetWorldPosition().z_ == Catch::Approx(10.0f - serverDelay).margin(networkError));
        CHECK(serverNode->GetWorldPosition().z_ < clientNode->GetWorldPosition().z_);
    }

    // Stop movement and wait for a while
    clientController->SetWalkVelocity(Vector3::ZERO);
    sim.SimulateTime(1.0f);

    // Expect server and client positions to match
    const float positionError = ReplicatedTransform::DefaultMovementThreshold;
    CHECK(serverNode->GetWorldPosition().Equals(clientNode->GetWorldPosition(), positionError));

    // Remove client connection and simulate more movement
    sim.RemoveClient(clientScene);
    clientController->SetWalkVelocity(Vector3::FORWARD * moveVelocity);
    sim.SimulateTime(5.0f);

    // Expect client node at about the specified position.
    {
        const float transitionError = networkError / 2;
        CHECK(clientNode->GetWorldPosition().x_ == 0.0f);
        CHECK(clientNode->GetWorldPosition().z_ == Catch::Approx(20.0f).margin(transitionError + networkError));
    }
}

TEST_CASE("Client-side prediction is stable when latency is stable")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/PredictedKinematicController/Test.prefab", CreateTestPrefab);

    // Setup scenes
    const unsigned seed = GENERATE(0, 1, 2);
    const auto quality = Tests::ConnectionQuality{ 0.21f, 0.23f, 0.23f, 0.0f, 0.0f };

    auto serverScene = CreateTestScene(context);
    auto clientScene = CreateTestScene(context);

    // Start simulation
    Tests::NetworkSimulator sim(serverScene, seed);
    sim.AddClient(clientScene, quality);
    clientScene->GetComponent<PhysicsWorld>()->SetInterpolation(false);

    // Create nodes
    Node* serverNode = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Player", {0.0f, 0.96f, 0.0f});
    auto serverObject = serverNode->GetComponent<BehaviorNetworkObject>();
    serverObject->SetClientPrefab(prefab);
    serverObject->SetOwner(sim.GetServerToClientConnection(clientScene));

    // Wait for synchronization and start tracking
    sim.SimulateTime(9.0f);
    Node* clientNode = clientScene->GetChild("Player", true);
    auto clientObject = clientNode->GetComponent<PredictedKinematicController>();

    Tests::AttributeTracker serverPosition(context);
    serverPosition.Track(serverNode, "Position");
    Tests::AttributeTracker serverRotation(context);
    serverRotation.Track(serverNode, "Rotation");
    Tests::AttributeTracker clientPosition(context);
    clientPosition.Track(clientNode, "Position");
    Tests::AttributeTracker clientRotation(context);
    clientRotation.Track(clientNode, "Rotation");
    sim.SimulateTime(1.0f);

    // Start random movement.
    // 1 physics tick is 1/50, so with velocity of 5.0 object should move for 0.1 units per tick.
    Vector3 direction = Vector3::LEFT * 5.0;
    for (unsigned actionIndex = 0; actionIndex < 100; ++actionIndex)
    {
        const float rotation = sim.GetRandom().GetFloat(0.0f, 360.0f);
        clientNode->SetWorldRotation(Quaternion{rotation, Vector3::UP});
        clientObject->SetWalkVelocity(direction);
        const bool needJump = sim.GetRandom().GetBool(0.1f);
        if (needJump)
            clientObject->SetJump();

        direction *= -1.0f;

        const float duration = sim.GetRandom().GetFloat(0.01f, 0.25f);
        sim.SimulateTime(sim.QuantizeDuration(duration));
    }

    serverPosition.SkipUntilChanged();
    serverRotation.SkipUntilChanged();
    clientPosition.SkipUntilChanged();
    clientRotation.SkipUntilChanged();

    const unsigned numValues = ea::min({serverPosition.Size(), serverRotation.Size(), clientPosition.Size(), clientRotation.Size()});

    // Compare every 4th element because client and server are synchronized only on frames
    for (unsigned i = 0; i < numValues; i += 4)
    {
        REQUIRE(serverPosition[i].GetVector3() == clientPosition[i].GetVector3());
        REQUIRE(serverRotation[i].GetQuaternion() == clientRotation[i].GetQuaternion());
    }
}

TEST_CASE("PredictedKinematicController works standalone")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/PredictedKinematicController/Test.prefab", CreateTestPrefab);

    auto standaloneScene = CreateTestScene(context);
    standaloneScene->CreateComponent<ReplicationManager>();

    Node* standaloneNode = Tests::SpawnOnServer<BehaviorNetworkObject>(standaloneScene, prefab, "Player", {0.0f, 0.96f, 0.0f});
    auto standaloneObject = standaloneNode->GetComponent<PredictedKinematicController>();

    const float moveVelocity = 2.0f;
    standaloneObject->SetWalkVelocity(Vector3::FORWARD * moveVelocity);
    Tests::NetworkSimulator::SimulateTime(context, 5.0f);

    CHECK(standaloneObject->GetVelocity().Equals(Vector3::FORWARD * moveVelocity, 0.02f));

    CHECK(standaloneNode->GetWorldPosition().x_ == 0.0f);
    CHECK(standaloneNode->GetWorldPosition().z_ == Catch::Approx(10.0f).margin(0.1f));
}
