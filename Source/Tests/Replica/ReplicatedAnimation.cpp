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
#include "../ModelUtils.h"
#include "../NetworkUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Replica/ReplicatedAnimation.h>

namespace
{

SharedPtr<Animation> CreateTestAnimation1(Context* context)
{
    return Tests::CreateLoopedTranslationAnimation(context, "", "", {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 2.0f);
}

SharedPtr<Animation> CreateTestAnimation2(Context* context)
{
    return Tests::CreateLoopedTranslationAnimation(context, "", "", {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 2.0f}, 2.0f);
}

SharedPtr<PrefabResource> CreateTestPrefab(Context* context)
{
    auto node = MakeShared<Node>(context);
    node->CreateComponent<AnimationController>();
    node->CreateComponent<ReplicatedAnimation>();

    return Tests::ConvertNodeToPrefab(node);
}

}

TEST_CASE("Animation is synchronized between client and server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto prefab = Tests::GetOrCreateResource<PrefabResource>(context, "@Tests/ReplicatedAnimation/Test.prefab", CreateTestPrefab);
    auto animation1 = Tests::GetOrCreateResource<Animation>(context, "@Tests/ReplicatedAnimation/Animation1.ani", CreateTestAnimation1);
    auto animation2 = Tests::GetOrCreateResource<Animation>(context, "@Tests/ReplicatedAnimation/Animation2.ani", CreateTestAnimation2);

    // Setup scenes
    const auto quality = Tests::ConnectionQuality{0.01f, 0.01f, 0.01f, 0, 0};
    //const auto quality = Tests::ConnectionQuality{0.30f, 0.32f, 0.35f, 0, 0};

    auto serverScene = MakeShared<Scene>(context);
    auto clientScene = MakeShared<Scene>(context);

    Node* serverNode = Tests::SpawnOnServer<BehaviorNetworkObject>(serverScene, prefab, "Node");
    auto serverAnimationController = serverNode->GetComponent<AnimationController>();
    serverAnimationController->PlayNewExclusive(AnimationParameters{animation1}.Looped());

    // Spend some time alone
    Tests::NetworkSimulator sim(serverScene);
    sim.SimulateTime(5.0f);

    // Add clients and wait for synchronization
    sim.AddClient(clientScene, quality);
    sim.SimulateTime(10.0f);

    const ServerReplicator& serverReplicator = *serverScene->GetComponent<ReplicationManager>()->GetServerReplicator();
    const ClientReplica& clientReplica = *clientScene->GetComponent<ReplicationManager>()->GetClientReplica();
    const double clientDelay = (serverReplicator.GetServerTime() - clientReplica.GetReplicaTime()) / Tests::NetworkSimulator::FramesInSecond;

    Node* clientNode = clientScene->GetChild("Node");
    REQUIRE(clientNode);
    auto clientAnimationController = clientNode->GetComponent<AnimationController>();
    auto clientReplicatedAnimation = clientNode->GetComponent<ReplicatedAnimation>();
    REQUIRE(clientAnimationController);

    // Expect initial animation played
    {
        const auto& lookups = clientReplicatedAnimation->GetAnimationLookup();
        REQUIRE(lookups.size() == 1);
        REQUIRE(lookups.find(StringHash{animation1->GetName()})->second == animation1->GetName());

        REQUIRE(clientAnimationController->GetNumAnimations() == 1);
        {
            const AnimationParameters& serverParams = serverAnimationController->GetAnimationParameters(0);
            const AnimationParameters& clientParams = clientAnimationController->GetAnimationParameters(0);
            REQUIRE(clientParams.animation_ == animation1);
            REQUIRE(clientParams.time_.Value() + clientDelay == Catch::Approx(serverParams.time_.Value()));
        }
    }

    // Play other animation, expect synchronized
    serverAnimationController->PlayNewExclusive(AnimationParameters{animation2}.Looped());
    sim.SimulateTime(1.0f);

    {
        const auto& lookups = clientReplicatedAnimation->GetAnimationLookup();
        REQUIRE(lookups.size() == 2);
        REQUIRE(lookups.find(StringHash{animation1->GetName()})->second == animation1->GetName());
        REQUIRE(lookups.find(StringHash{animation2->GetName()})->second == animation2->GetName());

        REQUIRE(clientAnimationController->GetNumAnimations() == 1);
        {
            const AnimationParameters& serverParams = serverAnimationController->GetAnimationParameters(0);
            const AnimationParameters& clientParams = clientAnimationController->GetAnimationParameters(0);
            REQUIRE(clientParams.animation_ == animation2);
            REQUIRE(clientParams.time_.Value() + clientDelay == Catch::Approx(serverParams.time_.Value()));
        }
    }

    // Play one more animation, expect synchronized
    serverAnimationController->PlayNew(AnimationParameters{animation1}.Looped().Additive());
    sim.SimulateTime(1.0f);

    {
        const auto& lookups = clientReplicatedAnimation->GetAnimationLookup();
        REQUIRE(lookups.size() == 2);
        REQUIRE(lookups.find(StringHash{animation1->GetName()})->second == animation1->GetName());
        REQUIRE(lookups.find(StringHash{animation2->GetName()})->second == animation2->GetName());

        REQUIRE(clientAnimationController->GetNumAnimations() == 2);
        {
            const AnimationParameters& serverParams = serverAnimationController->GetAnimationParameters(0);
            const AnimationParameters& clientParams = clientAnimationController->GetAnimationParameters(0);
            REQUIRE(clientParams.animation_ == animation2);
            REQUIRE(clientParams.time_.Value() + clientDelay == Catch::Approx(serverParams.time_.Value()));
            REQUIRE(clientParams.blendMode_ == ABM_LERP);
        }
        {
            const AnimationParameters& serverParams = serverAnimationController->GetAnimationParameters(1);
            const AnimationParameters& clientParams = clientAnimationController->GetAnimationParameters(1);
            REQUIRE(clientParams.animation_ == animation1);
            REQUIRE(clientParams.time_.Value() + clientDelay == Catch::Approx(serverParams.time_.Value()));
            REQUIRE(clientParams.blendMode_ == ABM_ADDITIVE);
        }
    }
}
