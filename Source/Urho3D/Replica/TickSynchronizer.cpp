// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Replica/TickSynchronizer.h"

#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/IO/Log.h"
#ifdef URHO3D_PHYSICS
#include "Urho3D/Physics/PhysicsEvents.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"
#endif

namespace Urho3D
{

TickSynchronizer::TickSynchronizer(unsigned leaderFrequency, bool isServer)
    : leaderFrequency_(leaderFrequency)
    , isServer_(isServer)
    , followerFrequency_(leaderFrequency)
{
}

void TickSynchronizer::SetFollowerFrequency(unsigned followerFrequency)
{
    const unsigned maxFollowerTicks = ea::max(1u, followerFrequency / leaderFrequency_);
    followerFrequency_ = leaderFrequency_ * maxFollowerTicks;

    if (followerFrequency_ != followerFrequency)
    {
        URHO3D_LOGWARNING(
            "Cannot synchronize follower clock at {} FPS with leading clock at {} FPS. {} FPS is used.",
            followerFrequency, leaderFrequency_, followerFrequency_);
    }
}

unsigned TickSynchronizer::Synchronize(float overtime)
{
    const unsigned maxFollowerTicks = followerFrequency_ / leaderFrequency_;
    if (isServer_)
    {
        numPendingFollowerTicks_ = maxFollowerTicks;
        numFollowerTicks_ = maxFollowerTicks;
        timeAccumulator_ = 0.0f;
        return 0;
    }
    else
    {
        const unsigned followerTicksDebt = numFollowerTicks_ != 0 ? maxFollowerTicks - numFollowerTicks_ : 0;

        numPendingFollowerTicks_ = followerTicksDebt + 1;
        numFollowerTicks_ = 1;
        timeAccumulator_ = overtime;
        NormalizeOnClient();

        return followerTicksDebt;
    }
}

void TickSynchronizer::Update(float timeStep)
{
    numPendingFollowerTicks_ = 0;
    if (!isServer_)
    {
        timeAccumulator_ += timeStep;
        NormalizeOnClient();
    }
}

void TickSynchronizer::NormalizeOnClient()
{
    const float fixedTimeStep = 1.0f / followerFrequency_;
    while (timeAccumulator_ >= fixedTimeStep)
    {
        timeAccumulator_ -= fixedTimeStep;
        ++numPendingFollowerTicks_;
        ++numFollowerTicks_;
    }

    const unsigned maxFollowerTicks = followerFrequency_ / leaderFrequency_;
    if (numFollowerTicks_ > maxFollowerTicks)
    {
        const unsigned extraFollowerTicks = numFollowerTicks_ - maxFollowerTicks;
        numPendingFollowerTicks_ -= extraFollowerTicks;
        numFollowerTicks_ -= extraFollowerTicks;
    }
}

SceneUpdateSynchronizer::SceneUpdateSynchronizer(Scene* scene, const Params& params)
    : Object(scene->GetContext())
    , params_(params)
    , sync_(params_.networkFrequency_, params_.isServer_)
    , scene_(scene)
#ifdef URHO3D_PHYSICS
    , physicsWorld_(scene->GetComponent<PhysicsWorld>())
#endif
{
    sync_.SetFollowerFrequency(params_.networkFrequency_);

    if (params_.isServer_)
    {
        scene_->SetManualUpdate(true);
        SubscribeToEvent(E_UPDATE, &SceneUpdateSynchronizer::UpdateSceneOnServer);
    }

#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        physicsWorld_->SetManualUpdate(true);

        wasInterpolated_ = physicsWorld_->GetInterpolation();
        if (params_.isServer_)
            physicsWorld_->SetInterpolation(false);

        SubscribeToEvent(scene, E_SCENESUBSYSTEMUPDATE, &SceneUpdateSynchronizer::UpdatePhysics);
    }
#endif
}

SceneUpdateSynchronizer::~SceneUpdateSynchronizer()
{
    if (scene_)
        scene_->SetManualUpdate(false);

#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        physicsWorld_->SetManualUpdate(false);
        physicsWorld_->SetInterpolation(wasInterpolated_);
    }
#endif
}

void SceneUpdateSynchronizer::Synchronize(NetworkFrame networkFrame, float overtime)
{
    UpdateFollowerFrequency();

    const unsigned synchronizedTick = sync_.Synchronize(overtime);
    synchronizedNetworkFrame_ = NetworkFrameSync{networkFrame, synchronizedTick};
}

void SceneUpdateSynchronizer::Update(float timeStep)
{
    UpdateFollowerFrequency();

    sync_.Update(timeStep);
}

void SceneUpdateSynchronizer::UpdateFollowerFrequency()
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
        sync_.SetFollowerFrequency(physicsWorld_->GetFps());
#endif
}

void SceneUpdateSynchronizer::UpdateSceneOnServer()
{
    if (!scene_)
        return;

    const float fixedTimeStep = 1.0f / sync_.GetFollowerFrequency();
    const unsigned numSteps = sync_.GetPendingFollowerTicks();
    if (numSteps != 0 || params_.allowZeroUpdatesOnServer_)
        scene_->Update(numSteps * fixedTimeStep);
}

void SceneUpdateSynchronizer::UpdatePhysics()
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        const float fixedTimeStep = 1.0f / sync_.GetFollowerFrequency();
        const float overtime = physicsWorld_->GetInterpolation() ? sync_.GetFollowerAccumulatedTime() : 0.0f;
        physicsWorld_->CustomUpdate(
            sync_.GetPendingFollowerTicks(), fixedTimeStep, overtime, synchronizedNetworkFrame_);
        synchronizedNetworkFrame_ = ea::nullopt;
    }
#endif
}

}
