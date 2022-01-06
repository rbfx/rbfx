//
// Copyright (c) 2008-2020 the Urho3D project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Network/LocalClockSynchronizer.h"

#include "../IO/Log.h"
#ifdef URHO3D_PHYSICS
#include "../Physics/PhysicsEvents.h"
#include "../Physics/PhysicsWorld.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#endif

namespace Urho3D
{

namespace
{

class PlaceholderObject : public Object
{
public:
    using Object::Object;

    Urho3D::StringHash GetType() const override { return {}; }
    const ea::string& GetTypeName() const override { return EMPTY_STRING; }
    const Urho3D::TypeInfo* GetTypeInfo() const override { return nullptr; }
};

}

LocalClockSynchronizer::LocalClockSynchronizer(unsigned leaderFrequency)
    : leaderFrequency_(leaderFrequency)
    , followerFrequency_(leaderFrequency)
{
}

void LocalClockSynchronizer::SetFollowerFrequency(unsigned followerFrequency)
{
    const unsigned maxSubsteps = ea::max(1u, followerFrequency / leaderFrequency_);
    followerFrequency_ = leaderFrequency_ * maxSubsteps;

    if (followerFrequency_ != followerFrequency)
    {
        URHO3D_LOGWARNING(
            "Cannot synchronize follower clock at {} FPS with leading clock at {} FPS. {} FPS is used.",
            followerFrequency, leaderFrequency_, followerFrequency_);
    }
}

void LocalClockSynchronizer::Update(float timeStep, ea::optional<float> leaderResetValue)
{
    unsigned numPendingSubsteps = 0;

    if (leaderResetValue)
    {
        numSubsteps_ = 0;
        timeAccumulator_ = *leaderResetValue;
        numPendingSubsteps = 1;
    }
    else
    {
        timeAccumulator_ += timeStep;
    }

    const float fixedTimeStep = 1.0f / followerFrequency_;
    while (timeAccumulator_ >= fixedTimeStep)
    {
        timeAccumulator_ -= fixedTimeStep;
        ++numPendingSubsteps;
    }

    const unsigned maxNumSubsteps = followerFrequency_ / leaderFrequency_;
    const unsigned newNumSubsteps = ea::min(numSubsteps_ + numPendingSubsteps, maxNumSubsteps);

    numPendingSubsteps_ = newNumSubsteps - numSubsteps_;
    numSubsteps_ = newNumSubsteps;
}

PhysicsClockSynchronizer::PhysicsClockSynchronizer(Scene* scene, unsigned networkFrequency, bool interpolated)
#ifdef URHO3D_PHYSICS
    : physicsWorld_(scene->GetComponent<PhysicsWorld>())
    , sync_(networkFrequency)
    , interpolated_(interpolated)
    , eventListener_(MakeShared<PlaceholderObject>(scene->GetContext()))
#endif
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        wasUpdateEnabled_ = physicsWorld_->IsUpdateEnabled();
        physicsWorld_->SetUpdateEnabled(false);
    }

    eventListener_->SubscribeToEvent(scene, E_SCENESUBSYSTEMUPDATE, [this](StringHash, VariantMap&)
    {
        UpdatePhysics();
    });
#endif
}

PhysicsClockSynchronizer::~PhysicsClockSynchronizer()
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
        physicsWorld_->SetUpdateEnabled(wasUpdateEnabled_);
#endif
}

void PhysicsClockSynchronizer::UpdateClock(float timeStep, ea::optional<float> leaderResetValue)
{
#ifdef URHO3D_PHYSICS
    if (!physicsWorld_)
        return;

    sync_.SetFollowerFrequency(physicsWorld_->GetFps());
    sync_.Update(timeStep, leaderResetValue);
#endif
}

void PhysicsClockSynchronizer::UpdatePhysics()
{
#ifdef URHO3D_PHYSICS
    if (!physicsWorld_)
        return;

    const float fixedTimeStep = 1.0f / sync_.GetFollowerFrequency();
    const float overtime = interpolated_ ? sync_.GetFollowerAccumulatedTime() : 0.0f;
    physicsWorld_->CustomUpdate(sync_.GetFollowerSteps(), fixedTimeStep, overtime);
#endif
}

}
