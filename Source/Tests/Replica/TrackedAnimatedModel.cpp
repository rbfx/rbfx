// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../ModelUtils.h"
#include "../NetworkUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Replica/BehaviorNetworkObject.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Replica/TrackedAnimatedModel.h>

namespace
{

SharedPtr<Model> CreateTestAnimatedModel(Context* context)
{
    return Tests::CreateSkinnedQuad_Model(context)->ExportModel();
}

}

TEST_CASE("TrackedAnimatedModel tracks bones on server")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->GetSubsystem<Network>()->SetUpdateFps(Tests::NetworkSimulator::FramesInSecond);

    auto model = Tests::GetOrCreateResource<Model>(context, "@/TrackedAnimatedModel/TestModel.mdl", CreateTestAnimatedModel);

    // Setup scene
    auto serverScene = MakeShared<Scene>(context);

    Node* node = serverScene->CreateChild("Node");
    node->CreateComponent<BehaviorNetworkObject>();
    auto animatedModel = node->CreateComponent<AnimatedModel>();
    animatedModel->SetModel(model);
    auto trackedAnimatedModel = node->CreateComponent<TrackedAnimatedModel>();

    Node* quad1 = node->GetChild("Quad 1", true);
    Node* quad2 = node->GetChild("Quad 2", true);

    // Animate objects forever
    serverScene->SubscribeToEvent(serverScene, E_SCENEUPDATE,
        [&](VariantMap& eventData)
    {
        const float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();
        quad1->Translate(timeStep * Vector3::LEFT, TS_PARENT);
        quad1->Rotate({timeStep, Vector3::UP}, TS_PARENT);
        quad2->Translate(timeStep * Vector3::FORWARD, TS_PARENT);
    });

    // Simulate some time and remember current state
    Tests::NetworkSimulator sim(serverScene);
    ServerReplicator* serverReplicator = serverScene->GetComponent<ReplicationManager>()->GetServerReplicator();

    sim.SimulateTime(10.0f);
    const NetworkTime serverTime = serverReplicator->GetServerTime();
    const Vector3 quad1Position = quad1->GetWorldPosition();
    const Quaternion quad1Rotation = quad1->GetWorldRotation();
    const Vector3 quad2Position = quad2->GetWorldPosition();
    const Quaternion quad2Rotation = quad2->GetWorldRotation();

    // Spend some more time and check that trace matches
    sim.SimulateTime(2.0f);
    REQUIRE(trackedAnimatedModel->SampleTemporalBonePosition(serverTime, 1).Cast<Vector3>() == quad1Position);
    REQUIRE(trackedAnimatedModel->SampleTemporalBonePosition(serverTime, 2).Cast<Vector3>() == quad2Position);
    REQUIRE(trackedAnimatedModel->SampleTemporalBoneRotation(serverTime, 1) == quad1Rotation);
    REQUIRE(trackedAnimatedModel->SampleTemporalBoneRotation(serverTime, 2) == quad2Rotation);
}
