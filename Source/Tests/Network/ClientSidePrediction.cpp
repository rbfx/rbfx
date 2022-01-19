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

#include <Urho3D/Network/ClientInputStatistics.h>
#include <Urho3D/Network/KinematicPlayerNetworkObject.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/KinematicCharacterController.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/XMLFile.h>

namespace
{

SharedPtr<Scene> CreateTestScene(Context* context)
{
    auto serverScene = MakeShared<Scene>(context);
    auto physicsWorld = serverScene->CreateComponent<PhysicsWorld>();
    physicsWorld->SetFps(64);

    Node* floorNode = serverScene->CreateChild("Floor", LOCAL);

    auto floorShape = floorNode->CreateComponent<CollisionShape>();
    floorShape->SetStaticPlane();

    auto floorBody = floorNode->CreateComponent<RigidBody>();
    return serverScene;
}

SharedPtr<XMLFile> CreateTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);
    node->CreateComponent<ReplicatedNetworkTransform>();
    node->CreateComponent<KinematicPlayerNetworkObject>();

    auto kinematicController = node->CreateComponent<KinematicCharacterController>();
    kinematicController->SetHeight(2.0f);

    auto prefab = MakeShared<XMLFile>(context);
    XMLElement prefabRootElement = prefab->CreateRoot("node");
    node->SaveXML(prefabRootElement);
    return prefab;
}

}

TEST_CASE("Client input quality is evaluated")
{
    ClientInputStatistics stats(10, 8);

    stats.OnInputReceived(1001);
    stats.OnInputReceived(1002);
    stats.OnInputReceived(1004);
    stats.OnInputReceived(1005);
    stats.OnInputReceived(1007);
    stats.OnInputReceived(1009);
    stats.OnInputReceived(1010);
    REQUIRE(stats.GetRecommendedBufferSize() == 1);

    stats.OnInputReceived(1020);
    REQUIRE(stats.GetRecommendedBufferSize() == 1);

    stats.OnInputReceived(1023);
    stats.OnInputReceived(1024);
    stats.OnInputReceived(1026);
    stats.OnInputReceived(1030);
    REQUIRE(stats.GetRecommendedBufferSize() == 2);
}

TEST_CASE("Client-side prediction is consistent with server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<XMLFile>(
        context, "@/ClientSidePrediction/TestPrefab.xml", [&] { return CreateTestPrefab(context); });

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
    serverObject->SetOwner(sim.GetServerToClientConnection(clientScene));

    // Wait for synchronization, expect controller on the ground
    sim.SimulateTime(10.0f);
    const auto& serverNetworkManager = serverScene->GetNetworkManager()->AsServer();
    const unsigned inputDelay = serverNetworkManager.GetFeedbackDelay(sim.GetServerToClientConnection(clientScene));

    Node* clientNode = clientScene->GetChild("Player", true);
    auto clientObject = clientNode->GetComponent<KinematicPlayerNetworkObject>();

    REQUIRE(serverNode->GetWorldPosition().ToXZ() == Vector2::ZERO);
    REQUIRE(serverNode->GetWorldPosition().y_ == Catch::Approx(1.0f).margin(0.1f));

    REQUIRE(clientNode->GetWorldPosition().ToXZ() == Vector2::ZERO);
    REQUIRE(clientNode->GetWorldPosition().y_ == Catch::Approx(1.0f).margin(0.1f));

    // Start movement at some random point, move for about 5 seconds with velocity of 2 units/second
    sim.SimulateTime(8.0f / Tests::NetworkSimulator::MillisecondsInSecond);
    const float moveVelocity = 2.0f;
    clientObject->SetWalkVelocity(Vector3::FORWARD * moveVelocity);
    sim.SimulateTime(1016.0f / Tests::NetworkSimulator::MillisecondsInSecond);
    sim.SimulateTime(4.0f);

    // Expect client node at about the specified position.
    const float physicsError = 0.05f;
    const float networkError = 1 * moveVelocity * (1.0f / Tests::NetworkSimulator::FramesInSecond);
    {
        CHECK(clientNode->GetWorldPosition().x_ == 0.0f);
        CHECK(clientNode->GetWorldPosition().z_ == Catch::Approx(10.0f - physicsError).margin(networkError));
    }

    // Expect server lagging behind, with max error about 1 + ping frames.
    {
        const float serverDelay = inputDelay * moveVelocity * (1.0f / Tests::NetworkSimulator::FramesInSecond);
        CHECK(serverNode->GetWorldPosition().x_ == 0.0f);
        CHECK(serverNode->GetWorldPosition().z_ == Catch::Approx(10.0f - physicsError - serverDelay).margin(networkError));
        CHECK(serverNode->GetWorldPosition().z_ < clientNode->GetWorldPosition().z_);
    }

    // Stop movement and wait for a while
    clientObject->SetWalkVelocity(Vector3::ZERO);
    sim.SimulateTime(1.0f);

    // Expect server and client positions to match
    CHECK(serverNode->GetWorldPosition().Equals(clientNode->GetWorldPosition(), 0.001)); // TODO(network): Reuse constant
}

TEST_CASE("Client-side prediction is stable when latency is stable")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<XMLFile>(
        context, "@/ClientSidePrediction/TestPrefab.xml", [&] { return CreateTestPrefab(context); });

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
    auto clientObject = clientNode->GetComponent<KinematicPlayerNetworkObject>();

    Tests::AttributeTracker serverPosition(context);
    serverPosition.Track(serverNode, "Position");
    Tests::AttributeTracker clientPosition(context);
    clientPosition.Track(clientNode, "Position");
    sim.SimulateTime(1.0f);

    // Start random movement.
    // 1 physics tick is 1/64, so with velocity of 6.4 object should move for 0.1 units per tick.
    Vector3 direction = Vector3::LEFT * 6.4;
    for (unsigned actionIndex = 0; actionIndex < 100; ++actionIndex)
    {
        clientObject->SetWalkVelocity(direction);
        direction *= -1.0f;

        const float duration = sim.GetRandom().GetFloat(0.01f, 0.25f);
        const unsigned quantizedDuration = CeilToInt(duration * Tests::NetworkSimulator::MillisecondsInSecond / 8);
        sim.SimulateTime(quantizedDuration * 8 / 1024.0f);
    }

    serverPosition.SkipUntilChanged();
    clientPosition.SkipUntilChanged();

    const auto& serverPositionValues = serverPosition.GetValues();
    const auto& clientPositionValues = clientPosition.GetValues();
    const unsigned numValues = ea::min(serverPositionValues.size(), clientPositionValues.size());

    // Compare every 4th element because client and server are synchronized only on frames
    for (unsigned i = 0; i < numValues; i += 4)
        REQUIRE(serverPositionValues[i].GetVector3() == clientPositionValues[i].GetVector3());
}
