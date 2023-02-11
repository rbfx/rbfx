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

#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Replica/BehaviorNetworkObject.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Replica/NetworkObject.h>
#include <Urho3D/Replica/NetworkValue.h>
#include <Urho3D/Replica/ReplicatedTransform.h>

namespace
{

SharedPtr<PrefabResource> CreateComplexTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);
    node->CreateComponent<ReplicatedTransform>();

    auto staticModel = node->CreateComponent<StaticModel>();
    staticModel->SetCastShadows(true);

    auto childNode = node->CreateChild("Child");
    childNode->SetPosition({0.0f, 1.0f, 0.0f});
    auto light = childNode->CreateComponent<Light>();
    light->SetCastShadows(true);
    light->SetColor(Color::RED);

    return Tests::ConvertNodeToPrefab(node);
}

SharedPtr<PrefabResource> CreateSimpleTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);
    node->CreateComponent<ReplicatedTransform>();

    return Tests::ConvertNodeToPrefab(node);
}


}

TEST_CASE("Scene is synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);
    const float syncDelay = 0.25f;

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/SceneSynchronization/SimpleTest.prefab", CreateSimpleTestPrefab);

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

        auto replicatedNodeA = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Replicated Node A");
        replicatedNodeA->SetScale(2.0f);
        transformReplicatedNodeA = replicatedNodeA->GetWorldTransform();

        auto replicatedNodeB = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Replicated Node B");
        replicatedNodeB->SetPosition({ -1.0f, 2.0f, 0.5f });
        transformReplicatedNodeB = replicatedNodeB->GetWorldTransform();

        auto replicatedNodeChild1 = Tests::SpawnOnServer<BehaviorNetworkObject>(replicatedNodeA, prefab, "Replicated Node Child 1");
        replicatedNodeChild1->SetPosition({ -2.0f, 3.0f, 1.5f });
        transformReplicatedNodeChild1 = replicatedNodeChild1->GetWorldTransform();

        auto replicatedNodeChild2 = Tests::SpawnOnServer<BehaviorNetworkObject>(replicatedNodeChild1, prefab, "Replicated Node Child 2");
        replicatedNodeChild2->SetRotation({ 90.0f, Vector3::UP });
        transformReplicatedNodeChild2 = replicatedNodeChild2->GetWorldTransform();

        auto serverOnlyChild3 = replicatedNodeB->CreateChild("Server Only Child 3");
        serverOnlyChild3->SetPosition({ -1.0f, 0.0f, 0.0f });

        auto replicatedNodeChild4 = Tests::SpawnOnServer<BehaviorNetworkObject>(serverOnlyChild3, prefab, "Replicated Node Child 4");
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

        REQUIRE(replicatedNodeA->GetWorldTransform().Equals(transformReplicatedNodeA, M_LARGE_EPSILON));
        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB, M_LARGE_EPSILON));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1, M_LARGE_EPSILON));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2, M_LARGE_EPSILON));
        REQUIRE(replicatedNodeChild4->GetWorldTransform().Equals(transformReplicatedNodeChild4, M_LARGE_EPSILON));
    }

    // Remove "Replicated Node A"
    // Add "Replicated Node C"
    {
        auto replicatedNodeA = serverScene->GetChild("Replicated Node A", true);
        replicatedNodeA->Remove();
        auto replicatedNodeC = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Replicated Node C");
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

        REQUIRE(replicatedNodeB->GetWorldTransform().Equals(transformReplicatedNodeB, M_LARGE_EPSILON));
        REQUIRE(replicatedNodeC->GetWorldTransform().Equals(Matrix3x4::IDENTITY, M_LARGE_EPSILON));
        REQUIRE(replicatedNodeChild1->GetWorldTransform().Equals(transformReplicatedNodeChild1, M_LARGE_EPSILON));
        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2, M_LARGE_EPSILON));
    }

    // Re-parent "Replicated Node Child 2" to Scene
    // Remove "Replicated Node Child 1", "Replicated Node B", "Replicated Node C"
    {
        auto replicatedNodeChild1 = serverScene->GetChild("Replicated Node Child 1", true);
        auto replicatedNodeChild2 = serverScene->GetChild("Replicated Node Child 2", true);
        auto replicatedNodeB = serverScene->GetChild("Replicated Node B", true);
        auto replicatedNodeC = serverScene->GetChild("Replicated Node C", true);

        replicatedNodeChild2->SetParent(serverScene);
        replicatedNodeChild1->Remove();
        replicatedNodeB->Remove();
        replicatedNodeC->Remove();
    }

    sim.SimulateTime(syncDelay);

    for (Scene* clientScene : clientScenes)
    {
        auto clientOnlyNode = clientScene->GetChild("Client Only Node", true);
        auto replicatedNodeChild2 = clientScene->GetChild("Replicated Node Child 2", true);

        REQUIRE(clientScene->GetNumChildren() == 2);
        REQUIRE(clientScene == clientOnlyNode->GetParent());
        REQUIRE(clientScene == replicatedNodeChild2->GetParent());

        REQUIRE(replicatedNodeChild2->GetWorldTransform().Equals(transformReplicatedNodeChild2, M_LARGE_EPSILON));
    }
}

TEST_CASE("Position and rotation are synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/SceneSynchronization/SimpleTest.prefab", CreateSimpleTestPrefab);

    // Setup scenes
    const auto interpolationQuality = Tests::ConnectionQuality{0.08f, 0.12f, 0.20f, 0, 0};
    const auto extrapolationQuality = Tests::ConnectionQuality{0.25f, 0.35f, 0.40f, 0, 0};
    const float positionError = ReplicatedTransform::DefaultMovementThreshold;

    const float moveSpeedNodeA = 1.0f;
    const float rotationSpeedNodeA = 10.0f;
    const float moveSpeedNodeB = 0.1f;
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> interpolatingClientScene = MakeShared<Scene>(context);
    interpolatingClientScene->SetName("Interpolating Scene");
    SharedPtr<Scene> extrapolatingClientScene = MakeShared<Scene>(context);
    extrapolatingClientScene->SetName("Extrapolation Scene");

    Node* serverNodeA = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Node");
    auto serverTransformA = serverNodeA->GetComponent<ReplicatedTransform>();
    serverTransformA->SetExtrapolateRotation(true);

    Node* serverNodeB = Tests::SpawnOnServer<BehaviorNetworkObject>(serverNodeA, prefab, "Node Child", { 0.0f, 0.0f, 1.0f });
    auto serverTransformB = serverNodeB->GetComponent<ReplicatedTransform>();
    serverTransformB->SetExtrapolateRotation(true);

    // Animate objects forever
    serverScene->SubscribeToEvent(serverScene, E_SCENEUPDATE,
        [&](StringHash, VariantMap& eventData)
    {
        const float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        serverNodeA->Translate(timeStep * moveSpeedNodeA * Vector3::LEFT, TS_PARENT);
        serverNodeA->Rotate({ timeStep * rotationSpeedNodeA, Vector3::UP }, TS_PARENT);
        serverNodeB->Translate(timeStep * moveSpeedNodeB * Vector3::FORWARD, TS_PARENT);
    });

    // Spend some time alone
    Tests::NetworkSimulator sim(serverScene);
    const auto& serverReplicator = *serverScene->GetComponent<ReplicationManager>()->GetServerReplicator();
    sim.SimulateTime(9.0f);

    // Add clients and wait for synchronization
    sim.AddClient(interpolatingClientScene, interpolationQuality);
    sim.AddClient(extrapolatingClientScene, extrapolationQuality);
    sim.SimulateTime(9.0f);

    // Expect positions and rotations to be precisely synchronized on interpolating client
    {
        const auto& clientReplica = *interpolatingClientScene->GetComponent<ReplicationManager>()->GetClientReplica();
        const NetworkTime replicaTime = clientReplica.GetReplicaTime();
        const double delay = serverReplicator.GetServerTime() - replicaTime;

        auto clientNodeA = interpolatingClientScene->GetChild("Node", true);
        auto clientNodeB = interpolatingClientScene->GetChild("Node Child", true);

        REQUIRE(delay / Tests::NetworkSimulator::FramesInSecond == Catch::Approx(0.2).margin(0.03));

        REQUIRE(serverTransformA->SampleTemporalPosition(replicaTime).value_.Equals(clientNodeA->GetWorldPosition(), positionError));
        REQUIRE(serverTransformA->SampleTemporalRotation(replicaTime).value_.Equivalent(clientNodeA->GetWorldRotation(), M_EPSILON));

        REQUIRE(serverTransformB->SampleTemporalPosition(replicaTime).value_.Equals(clientNodeB->GetWorldPosition(), positionError));
        REQUIRE(serverTransformB->SampleTemporalRotation(replicaTime).value_.Equivalent(clientNodeB->GetWorldRotation(), M_EPSILON));
    }

    // Expect positions and rotations to be roughly synchronized on extrapolating client
    {
        const auto& clientReplica = *extrapolatingClientScene->GetComponent<ReplicationManager>()->GetClientReplica();
        const NetworkTime replicaTime = clientReplica.GetReplicaTime();
        const double delay = serverReplicator.GetServerTime() - replicaTime;

        auto clientNodeA = extrapolatingClientScene->GetChild("Node", true);
        auto clientNodeB = extrapolatingClientScene->GetChild("Node Child", true);

        REQUIRE(delay / Tests::NetworkSimulator::FramesInSecond == Catch::Approx(0.25).margin(0.03));

        REQUIRE(serverTransformA->SampleTemporalPosition(replicaTime).value_.Equals(clientNodeA->GetWorldPosition(), positionError));
        REQUIRE(serverTransformA->SampleTemporalRotation(replicaTime).value_.Equivalent(clientNodeA->GetWorldRotation(), M_EPSILON));

        REQUIRE(serverTransformB->SampleTemporalPosition(replicaTime).value_.Equals(clientNodeB->GetWorldPosition(), 0.002f));
        REQUIRE(serverTransformB->SampleTemporalRotation(replicaTime).value_.Equivalent(clientNodeB->GetWorldRotation(), M_EPSILON));
    }
}

TEST_CASE("Prefabs are replicated on clients")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/SceneSynchronization/ComplexTest.prefab", CreateComplexTestPrefab);

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    // Start simulation
    Tests::NetworkSimulator sim(serverScene);
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);

    // Create nodes
    {
        Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Node 1", {1.0f, 0.0f, 0.0f});
        Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Node 2", {2.0f, 0.0f, 0.0f});
    }
    sim.SimulateTime(10.0f);

    // Expect prefabs replicated
    for (Scene* clientScene : clientScenes)
    {
        auto node1 = clientScene->GetChild("Node 1", true);
        REQUIRE(node1);

        auto node2 = clientScene->GetChild("Node 2", true);
        REQUIRE(node2);

        auto node1Child = node1->GetChild("Child");
        REQUIRE(node1Child);

        auto node2Child = node2->GetChild("Child");
        REQUIRE(node2Child);

        CHECK(node1->GetWorldPosition().Equals({1.0f, 0.0f, 0.0f}));
        CHECK(node1Child->GetWorldPosition().Equals({1.0f, 1.0f, 0.0f}));
        CHECK(node2->GetWorldPosition().Equals({2.0f, 0.0f, 0.0f}));
        CHECK(node2Child->GetWorldPosition().Equals({2.0f, 1.0f, 0.0f}));

        auto staticModel1 = node1->GetComponent<StaticModel>();
        REQUIRE(staticModel1);

        auto staticModel2 = node2->GetComponent<StaticModel>();
        REQUIRE(staticModel2);

        auto light1 = node1Child->GetComponent<Light>();
        REQUIRE(light1);

        auto light2 = node2Child->GetComponent<Light>();
        REQUIRE(light2);

        CHECK(staticModel1->GetCastShadows() == true);
        CHECK(staticModel2->GetCastShadows() == true);
        CHECK(light1->GetCastShadows() == true);
        CHECK(light2->GetCastShadows() == true);
        CHECK(light1->GetColor() == Color::RED);
        CHECK(light2->GetColor() == Color::RED);
    }
}

TEST_CASE("Ownership is consistent on server and on clients")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/SceneSynchronization/SimpleTest.prefab", CreateSimpleTestPrefab);

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    auto serverScene = MakeShared<Scene>(context);
    SharedPtr<Scene> clientScenes[] = {
        MakeShared<Scene>(context),
        MakeShared<Scene>(context),
        MakeShared<Scene>(context)
    };

    // Start simulation
    Tests::NetworkSimulator sim(serverScene);
    for (Scene* clientScene : clientScenes)
        sim.AddClient(clientScene, quality);

    // Create nodes
    {
        Node* node = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Unowned Node");

        auto object = node->GetDerivedComponent<NetworkObject>();
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Standalone);
    }
    {
        Node* node = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Owned Node 0");

        auto object = node->GetDerivedComponent<NetworkObject>();
        object->SetOwner(sim.GetServerToClientConnection(clientScenes[0]));
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Standalone);
    }
    {
        Node* node = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Owned Node 1");

        auto object = node->GetDerivedComponent<NetworkObject>();
        object->SetOwner(sim.GetServerToClientConnection(clientScenes[1]));
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Standalone);
    }
    {
        Node* node = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Owned Node 2");

        auto object = node->GetDerivedComponent<NetworkObject>();
        object->SetOwner(sim.GetServerToClientConnection(clientScenes[2]));
        REQUIRE(object->GetNetworkMode() == NetworkObjectMode::Standalone);
    }
    sim.SimulateTime(10.0f);

    // Check ownership of objects
    const auto getObject = [](Scene* scene, const ea::string& name)
    {
        return scene->GetChild(name, true)->GetDerivedComponent<NetworkObject>();
    };

    REQUIRE(getObject(serverScene, "Unowned Node")->GetNetworkMode() == NetworkObjectMode::Server);
    REQUIRE(getObject(serverScene, "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::Server);
    REQUIRE(getObject(serverScene, "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::Server);
    REQUIRE(getObject(serverScene, "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::Server);

    REQUIRE(getObject(clientScenes[0], "Unowned Node")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[0], "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::ClientOwned);
    REQUIRE(getObject(clientScenes[0], "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[0], "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);

    REQUIRE(getObject(clientScenes[1], "Unowned Node")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[1], "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[1], "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::ClientOwned);
    REQUIRE(getObject(clientScenes[1], "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);

    REQUIRE(getObject(clientScenes[2], "Unowned Node")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[2], "Owned Node 0")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[2], "Owned Node 1")->GetNetworkMode() == NetworkObjectMode::ClientReplicated);
    REQUIRE(getObject(clientScenes[2], "Owned Node 2")->GetNetworkMode() == NetworkObjectMode::ClientOwned);

    // Check client-side arrays
    const auto getClientReplica = [](Scene* scene)
    {
        return scene->GetComponent<ReplicationManager>()->GetClientReplica();
    };

    REQUIRE(getClientReplica(clientScenes[0])->GetOwnedNetworkObject() == getObject(clientScenes[0], "Owned Node 0"));
    REQUIRE(getClientReplica(clientScenes[1])->GetOwnedNetworkObject() == getObject(clientScenes[1], "Owned Node 1"));
    REQUIRE(getClientReplica(clientScenes[2])->GetOwnedNetworkObject() == getObject(clientScenes[2], "Owned Node 2"));

    // Check server-side arrays
    auto serverReplicator = serverScene->GetComponent<ReplicationManager>()->GetServerReplicator();

    REQUIRE(serverReplicator->GetNetworkObjectOwnedByConnection(sim.GetServerToClientConnection(clientScenes[0])) == getObject(serverScene, "Owned Node 0"));
    REQUIRE(serverReplicator->GetNetworkObjectOwnedByConnection(sim.GetServerToClientConnection(clientScenes[1])) == getObject(serverScene, "Owned Node 1"));
    REQUIRE(serverReplicator->GetNetworkObjectOwnedByConnection(sim.GetServerToClientConnection(clientScenes[2])) == getObject(serverScene, "Owned Node 2"));

    // Remove all owned nodes
    serverScene->GetChild("Owned Node 0", true)->Remove();
    serverScene->GetChild("Owned Node 1", true)->Remove();
    serverScene->GetChild("Owned Node 2", true)->Remove();

    sim.SimulateTime(10.0f);

    REQUIRE(getClientReplica(clientScenes[0])->GetOwnedNetworkObject() == nullptr);
    REQUIRE(getClientReplica(clientScenes[1])->GetOwnedNetworkObject() == nullptr);
    REQUIRE(getClientReplica(clientScenes[2])->GetOwnedNetworkObject() == nullptr);

    REQUIRE(serverReplicator->GetNetworkObjectOwnedByConnection(sim.GetServerToClientConnection(clientScenes[0])) == nullptr);
    REQUIRE(serverReplicator->GetNetworkObjectOwnedByConnection(sim.GetServerToClientConnection(clientScenes[1])) == nullptr);
    REQUIRE(serverReplicator->GetNetworkObjectOwnedByConnection(sim.GetServerToClientConnection(clientScenes[2])) == nullptr);

}
