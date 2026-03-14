// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/IndexAllocator.h"
#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Timer.h"
#include "Urho3D/Network/ClockSynchronizer.h"
#include "Urho3D/Network/Protocol.h"
#include "Urho3D/Replica/ReplicationManager.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

class NetworkConnection;

/// Replication-oriented interface attached to transport connection.
class URHO3D_API ReplicatedPeer
{
public:
    explicit ReplicatedPeer(NetworkConnection* connection, unsigned pingIntervalMs=250, unsigned maxPingMs=10000, unsigned clockBufferSize=40,
        unsigned pingBufferSize=10, ea::function<unsigned()> getTimestamp = [] { return Time::GetSystemTime(); });
    virtual ~ReplicatedPeer() = default;

    /// Return unique peer ID.
    unsigned GetObjectID() const { return idFamily_.GetObjectID(); }
    /// Return associated network connection.
    NetworkConnection* GetConnection() const { return connection_; }

    /// Set replication manager used for automatic message handling. This effectively enables replication-related message processing and sending.
    /// You may unset replication manager to suspend connection from participating in scene replication.
    void SetReplicationManager(ReplicationManager* replicationManager);
    /// Return replication manager used for automatic message handling.
    ReplicationManager* GetReplicationManager() const { return replicationManager_; }

    /// Return debug connection string for logging.
    virtual ea::string ToString() const;
    /// Return whether the clock is synchronized between client and server.
    virtual bool IsClockSynchronized() const { return clock_.IsReady(); }
    /// Convert remote timestamp to local timestamp.
    virtual unsigned RemoteToLocalTime(unsigned time) const { return clock_.RemoteToLocal(time); }
    /// Convert local timestamp to remote timestamp.
    virtual unsigned LocalToRemoteTime(unsigned time) const { return clock_.LocalToRemote(time); }
    /// Return current local time.
    virtual unsigned GetLocalTime() const { return Time::GetSystemTime(); }
    /// Return local time of last successful ping-pong roundtrip.
    virtual unsigned GetLocalTimeOfLatestRoundtrip() const { return clock_.GetLocalTimeOfLatestRoundtrip(); }
    /// Return ping of the connection.
    virtual unsigned GetPing() const { return clock_.GetPing(); }

    /// @{
    ClockSynchronizer& GetClock() { return clock_; }
    const ClockSynchronizer& GetClock() const { return clock_; }
    /// @}

private:
    class Listener : public Object
    {
        URHO3D_OBJECT(ReplicatedPeer::Listener, Object);

    public:
        explicit Listener(Context* context) : Object(context) {}
    };

    SharedPtr<ReplicatedPeer, RefCounted> AsSharedPtr();
    void OnConnected();
    void OnDisconnected();

    /// Potentially consume a network message. Return true if consumed.
    void ProcessReplicationMessage(NetworkMessageId messageId, MemoryBuffer& msg, bool& handled);
    /// Send pending replication-related messages, if any.
    void SendReplicationMessages();

    NetworkConnection* connection_ = nullptr;
    ClockSynchronizer clock_;
    Listener listener_;
    IDFamily<ReplicatedPeer> idFamily_{};
    WeakPtr<ReplicationManager> replicationManager_;
};

}; // namespace Urho3D
