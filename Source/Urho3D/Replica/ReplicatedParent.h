// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Replica/BehaviorNetworkObject.h"

namespace Urho3D
{

/// Behavior that replicates current parent network object.
class URHO3D_API ReplicatedParent : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedParent, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::ReliableDelta;

    explicit ReplicatedParent(Context* context);
    ~ReplicatedParent() override;

    static void RegisterObject(Context* context);

    /// Implement NetworkBehavior.
    /// @{
    void InitializeOnServer() override;

    bool PrepareReliableDelta(NetworkFrame frame) override;
    void WriteReliableDelta(NetworkFrame frame, Serializer& dest) override;
    void ReadReliableDelta(NetworkFrame frame, Deserializer& src) override;
    /// @}

private:
    NetworkId latestSentParentObject_{NetworkId::None};
};

}; // namespace Urho3D
