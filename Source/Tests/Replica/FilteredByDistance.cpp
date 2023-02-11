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
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Replica/BehaviorNetworkObject.h>
#include <Urho3D/Replica/FilteredByDistance.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Replica/ReplicatedTransform.h>

namespace
{

SharedPtr<PrefabResource> CreateFilteredTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);
    node->CreateComponent<ReplicatedTransform>();

    auto filter = node->CreateComponent<FilteredByDistance>();
    filter->SetRelevant(false);
    filter->SetDistance(10.0f);

    return Tests::ConvertNodeToPrefab(node);
}

SharedPtr<PrefabResource> CreateUnfilteredTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);
    node->CreateComponent<ReplicatedTransform>();

    return Tests::ConvertNodeToPrefab(node);
}

}

TEST_CASE("FilteredByDistance handles object hierarchies")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto filteredPrefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/FilteredByDistance/FilteredTest.prefab", CreateFilteredTestPrefab);
    auto unfilteredPrefab = Tests::GetOrCreateResource<PrefabResource>(context, "@/FilteredByDistance/UnfilteredTest.prefab", CreateUnfilteredTestPrefab);

    // Create scenes
    auto serverScene = MakeShared<Scene>(context);
    auto clientScene = MakeShared<Scene>(context);

    const auto quality = Tests::ConnectionQuality{ 0.08f, 0.12f, 0.20f, 0.02f, 0.02f };
    Tests::NetworkSimulator sim(serverScene);
    sim.AddClient(clientScene, quality);
    sim.SimulateTime(5.0f);

    // Spawn objects
    {
        auto clientNode = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, filteredPrefab, "Client Node");
        clientNode->GetComponent<BehaviorNetworkObject>()->SetOwner(sim.GetServerToClientConnection(clientScene));
        clientNode->SetWorldPosition(Vector3(0.0f, 0.0f, 0.0f));

        auto filteredParentNode = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, filteredPrefab, "Filtered Parent Node");
        filteredParentNode->SetWorldPosition(Vector3(0.0f, 0.0f, 0.0f));

        auto unfilteredChildNode = Tests::SpawnOnServer<BehaviorNetworkObject>(filteredParentNode, unfilteredPrefab, "Unfiltered Child Node");
        unfilteredChildNode->SetWorldPosition(Vector3(0.0f, 0.0f, 8.0f));
    }

    // Except all objects on the client
    sim.SimulateTime(8.0f);

    {
        auto clientNode = clientScene->GetChild("Client Node", true);
        auto filteredParentNode = clientScene->GetChild("Filtered Parent Node", true);
        auto unfilteredChildNode = clientScene->GetChild("Unfiltered Child Node", true);

        REQUIRE(clientNode);
        REQUIRE(filteredParentNode);
        REQUIRE(unfilteredChildNode);

        REQUIRE(clientNode->GetParent() == clientScene);
        REQUIRE(filteredParentNode->GetParent() == clientScene);
        REQUIRE(unfilteredChildNode->GetParent() == filteredParentNode);

        REQUIRE(clientNode->GetWorldPosition() == Vector3{0.0f, 0.0f, 0.0f});
        REQUIRE(filteredParentNode->GetWorldPosition() == Vector3{0.0f, 0.0f, 0.0f});
        REQUIRE(unfilteredChildNode->GetWorldPosition() == Vector3{0.0f, 0.0f, 8.0f});
    }

    // Move filtered object outside of the range
    serverScene->GetChild("Filtered Parent Node", true)->SetWorldPosition(Vector3{0.0f, 0.0f, -12.0f});
    sim.SimulateTime(8.0f);

    // Except whole tree gone
    {
        auto clientNode = clientScene->GetChild("Client Node", true);
        auto filteredParentNode = clientScene->GetChild("Filtered Parent Node", true);
        auto unfilteredChildNode = clientScene->GetChild("Unfiltered Child Node", true);

        REQUIRE(clientNode);
        REQUIRE_FALSE(filteredParentNode);
        REQUIRE_FALSE(unfilteredChildNode);
    }

    // Move filtered object back
    serverScene->GetChild("Filtered Parent Node", true)->SetWorldPosition(Vector3{0.0f, 0.0f, -8.0f});
    sim.SimulateTime(8.0f);

    // Except whole tree back
    {
        auto clientNode = clientScene->GetChild("Client Node", true);
        auto filteredParentNode = clientScene->GetChild("Filtered Parent Node", true);
        auto unfilteredChildNode = clientScene->GetChild("Unfiltered Child Node", true);

        REQUIRE(clientNode);
        REQUIRE(filteredParentNode);
        REQUIRE(unfilteredChildNode);

        REQUIRE(clientNode->GetParent() == clientScene);
        REQUIRE(filteredParentNode->GetParent() == clientScene);
        REQUIRE(unfilteredChildNode->GetParent() == filteredParentNode);

        REQUIRE(clientNode->GetWorldPosition() == Vector3{0.0f, 0.0f, 0.0f});
        REQUIRE(filteredParentNode->GetWorldPosition() == Vector3{0.0f, 0.0f, -8.0f});
        REQUIRE(unfilteredChildNode->GetWorldPosition() == Vector3{0.0f, 0.0f, 0.0f});
    }

    // Test child node removal
    serverScene->GetChild("Unfiltered Child Node", true)->Remove();
    sim.SimulateTime(8.0f);

    {
        auto clientNode = clientScene->GetChild("Client Node", true);
        auto filteredParentNode = clientScene->GetChild("Filtered Parent Node", true);
        auto unfilteredChildNode = clientScene->GetChild("Unfiltered Child Node", true);

        REQUIRE(clientNode);
        REQUIRE(filteredParentNode);
        REQUIRE_FALSE(unfilteredChildNode);
    }
}
