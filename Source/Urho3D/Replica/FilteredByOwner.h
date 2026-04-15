// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Replica/BehaviorNetworkObject.h"

namespace Urho3D
{

/// Behavior that filters NetworkObject by the owner.
/// The object with this behavior is only relevant to its owner connection and is never replicated to other connections.
class URHO3D_API FilteredByOwner : public NetworkBehavior
{
    URHO3D_OBJECT(FilteredByOwner, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::GetRelevanceForClient;

    explicit FilteredByOwner(Context* context);
    ~FilteredByOwner() override;

    static void RegisterObject(Context* context);

    /// Implement NetworkBehavior.
    /// @{
    ea::optional<NetworkObjectRelevance> GetRelevanceForClient(AbstractConnection* connection) override;
    /// @}
};

}; // namespace Urho3D
