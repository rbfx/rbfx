// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Network/ReplicatedPeer.h>
#include <Urho3D/Network/Transport/DataChannel/DataChannelConnection.h>

using namespace Urho3D;

// Replicated connection. It carries additional replication state and processes
// replication packets automatically.
class AdvancedNetworkingConnection : public DataChannelConnection
{
public:
    explicit AdvancedNetworkingConnection(Context* context)
        : DataChannelConnection(context)
        , replicatedPeer_(this)
    {
    }

    SharedPtr<ReplicatedPeer, RefCounted> GetReplicatedPeer()
    {
        return SharedPtr<ReplicatedPeer, RefCounted>(&replicatedPeer_, this);
    }

private:
    ReplicatedPeer replicatedPeer_;
};
