// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Replica/NetworkId.h"
#include "Urho3D/Replica/NetworkTime.h"
#ifdef URHO3D_PHYSICS
    #include "Urho3D/Physics/PhysicsWorld.h"
#endif

#include <EASTL/optional.h>

namespace Urho3D
{

class Scene;

/// Helper class that synchronizes two fixed-timestep clocks.
/// Leader and follower ticks are considered synchronized
/// if their beginnings logically correspond to the same moment in time.
/// \note Leader clock should not tick faster than follower clock.
/// \note Leader clock should be explicitly reset on each tick.
/// \note Follower clock will never tick ahead of the leader clock.
class URHO3D_API TickSynchronizer
{
public:
    TickSynchronizer(unsigned leaderFrequency, bool isServer);

    void SetFollowerFrequency(unsigned followerFrequency);
    unsigned GetFollowerFrequency() const { return followerFrequency_; }

    /// Synchronize with tick of the leader clock.
    /// Overtime specifies how much time passed since leader clock tick.
    /// Returns number of follower clock ticks before leader and follower clocks are synchronized.
    unsigned Synchronize(float overtime);
    /// Update follower clock within one tick of leader clock.
    void Update(float timeStep);

    /// Return number of the follower clock ticks that are expected to happen during current engine update.
    unsigned GetPendingFollowerTicks() const { return numPendingFollowerTicks_; }
    /// Return amount of time elapsed after latest follower tick.
    float GetFollowerAccumulatedTime() const { return timeAccumulator_; }

private:
    void NormalizeOnClient();

    const unsigned leaderFrequency_{};
    const bool isServer_{};
    unsigned followerFrequency_{};

    float timeAccumulator_{};
    unsigned numFollowerTicks_{};
    unsigned numPendingFollowerTicks_{};
};

/// Helper class that synchronizes Scene updates with network clock.
class URHO3D_API SceneUpdateSynchronizer : public Object
{
    URHO3D_OBJECT(SceneUpdateSynchronizer, Object);

public:
    struct Params
    {
        bool isServer_{};
        unsigned networkFrequency_{};
        bool allowZeroUpdatesOnServer_{};
    };

    SceneUpdateSynchronizer(Scene* scene, const Params& params);
    ~SceneUpdateSynchronizer();

    void Synchronize(NetworkFrame networkFrame, float overtime);
    void Update(float timeStep);

protected:
    void UpdateFollowerFrequency();
    void UpdateSceneOnServer();
    void UpdatePhysics();

    const Params params_;

    WeakPtr<Scene> scene_;
#ifdef URHO3D_PHYSICS
    WeakPtr<PhysicsWorld> physicsWorld_;
#endif

    bool wasInterpolated_{};

    TickSynchronizer sync_;
    ea::optional<NetworkFrameSync> synchronizedNetworkFrame_;
};

} // namespace Urho3D
