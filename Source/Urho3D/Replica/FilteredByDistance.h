// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Replica/BehaviorNetworkObject.h"

namespace Urho3D
{

/// Behavior that filters NetworkObject by the minimum distance to the client.
/// If the distance is less than the threshold, no relevance is reported.
/// If the distance is greater than the threshold, specified relevance or irrelevance is reported.
class URHO3D_API FilteredByDistance : public NetworkBehavior
{
    URHO3D_OBJECT(FilteredByDistance, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::GetRelevanceForClient;
    static constexpr float DefaultDistance = 100.0f;

    explicit FilteredByDistance(Context* context);
    ~FilteredByDistance() override;

    static void RegisterObject(Context* context);

    /// Manage attributes.
    /// @{
    void SetRelevant(bool value) { isRelevant_ = value; }
    bool IsRelevant() const { return isRelevant_; }
    void SetUpdatePeriod(unsigned value) { updatePeriod_ = value; }
    unsigned GetUpdatePeriod() const { return updatePeriod_; }
    void SetDistance(float value) { distance_ = value; }
    float GetDistance() const { return distance_; }
    /// @}

    /// Implement NetworkBehavior.
    /// @{
    ea::optional<NetworkObjectRelevance> GetRelevanceForClient(ReplicatedPeer* connection) override;
    /// @}

private:
    bool isRelevant_{true};
    unsigned updatePeriod_{};
    float distance_{DefaultDistance};
};

};
