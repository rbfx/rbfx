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

/// \file

#pragma once

#include "../Core/Object.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class Scene;
class PhysicsWorld;

/// Helper class that synchronizes two fixed-timestep clocks.
/// Leader clock should not tick faster than follower clock.
/// Leader clock should be explicitly reset on each tick.
class URHO3D_API LocalClockSynchronizer
{
public:
    explicit LocalClockSynchronizer(unsigned leaderFrequency);
    void SetFollowerFrequency(unsigned followerFrequency);
    void Update(float timeStep, ea::optional<float> leaderResetValue);

    unsigned GetFollowerSteps() const { return numPendingSubsteps_; }
    unsigned GetFollowerFrequency() const { return followerFrequency_; }
    float GetFollowerAccumulatedTime() const { return timeAccumulator_; }

private:
    const unsigned leaderFrequency_{};
    unsigned followerFrequency_{};

    float timeAccumulator_{};
    unsigned numSubsteps_{};
    unsigned numPendingSubsteps_{};
};

/// Helper class that synchronizes PhysicsWorld clock with network clock.
class URHO3D_API PhysicsClockSynchronizer
{
public:
    PhysicsClockSynchronizer(Scene* scene, unsigned networkFrequency, bool interpolated);
    ~PhysicsClockSynchronizer();

    void UpdateClock(float timeStep, ea::optional<float> leaderResetValue);

protected:
    void UpdatePhysics();

#ifdef URHO3D_PHYSICS
    WeakPtr<PhysicsWorld> physicsWorld_;
    LocalClockSynchronizer sync_;
    bool interpolated_{};
    SharedPtr<Object> eventListener_;

    bool wasUpdateEnabled_{};
#endif
};

}
