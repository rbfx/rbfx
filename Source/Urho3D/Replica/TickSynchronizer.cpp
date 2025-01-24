//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Precompiled.h"

#include "../Replica/TickSynchronizer.h"

#include "../IO/Log.h"
#ifdef URHO3D_PHYSICS
#include "../Physics/PhysicsEvents.h"
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
    bool IsInstanceOf(StringHash type) const override { return false; }
};

}

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
        timeAccumulator_ = overtime;
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
    timeAccumulator_ += timeStep;
    if (isServer_)
        return;

    NormalizeOnClient();
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

PhysicsTickSynchronizer::PhysicsTickSynchronizer(Scene* scene, unsigned networkFrequency, bool isServer)
#ifdef URHO3D_PHYSICS
    : physicsWorld_(scene->GetComponent<PhysicsWorld>())
    , sync_(networkFrequency, isServer)
    , eventListener_(MakeShared<PlaceholderObject>(scene->GetContext()))
#endif
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        wasUpdateEnabled_ = physicsWorld_->IsUpdateEnabled();
        wasInterpolated_ = physicsWorld_->GetInterpolation();
        interpolated_ = !isServer && wasInterpolated_;

        physicsWorld_->SetUpdateEnabled(false);
        physicsWorld_->SetInterpolation(interpolated_);
    }

    eventListener_->SubscribeToEvent(scene, E_SCENESUBSYSTEMUPDATE, [this] { UpdatePhysics(); });
#endif
}

PhysicsTickSynchronizer::~PhysicsTickSynchronizer()
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        physicsWorld_->SetUpdateEnabled(wasUpdateEnabled_);
        physicsWorld_->SetInterpolation(wasInterpolated_);
    }
#endif
}

void PhysicsTickSynchronizer::Synchronize(NetworkFrame networkFrame, float overtime)
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        sync_.SetFollowerFrequency(physicsWorld_->GetFps());
        const unsigned synchronizedTick = sync_.Synchronize(overtime);
        synchronizedStep_ = SynchronizedPhysicsStep{synchronizedTick, networkFrame};
    }
#endif
}

void PhysicsTickSynchronizer::Update(float timeStep)
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        sync_.SetFollowerFrequency(physicsWorld_->GetFps());
        sync_.Update(timeStep);
    }
#endif
}

void PhysicsTickSynchronizer::UpdatePhysics()
{
#ifdef URHO3D_PHYSICS
    if (physicsWorld_)
    {
        const float fixedTimeStep = 1.0f / sync_.GetFollowerFrequency();
        const float overtime = interpolated_ ? sync_.GetFollowerAccumulatedTime() : 0.0f;
        physicsWorld_->CustomUpdate(sync_.GetPendingFollowerTicks(), fixedTimeStep, overtime, synchronizedStep_);
        synchronizedStep_ = ea::nullopt;
    }
#endif
}

}
