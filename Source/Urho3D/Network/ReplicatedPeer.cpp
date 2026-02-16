// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Core/Assert.h"
#include "Urho3D/Network/ClockSynchronizer.h"
#include "Urho3D/Network/MessageUtils.h"
#include "Urho3D/Network/ReplicatedPeer.h"
#include "Urho3D/Network/Transport/NetworkConnection.h"
#include "Urho3D/Replica/ReplicationManager.h"
#include "Urho3D/Core/CoreEvents.h"

namespace Urho3D
{

AbstractConnection::AbstractConnection(NetworkConnection* connection, unsigned pingIntervalMs, unsigned maxPingMs, unsigned clockBufferSize,
        unsigned pingBufferSize, ea::function<unsigned()> getTimestamp)
    : connection_(connection)
    , clock_(pingIntervalMs, maxPingMs, clockBufferSize, pingBufferSize, getTimestamp)
    , listener_(connection->GetContext())
{
    URHO3D_ASSERT(connection);

    listener_.SubscribeToEvent(E_POSTUPDATE, [this]() { SendReplicationMessages(); });
    auto&& handleSendReplicationMessages = [this](NetworkMessageId messageId, MemoryBuffer& msg, bool& handled) { ProcessReplicationMessage(messageId, msg, handled); };
    connection_->onMessage_.Subscribe(&listener_, handleSendReplicationMessages);
    connection_->onConnected_.Subscribe(&listener_, [this]() { OnConnected(); });
    connection_->onDisconnected_.Subscribe(&listener_, [this](bool) { OnDisconnected(); });
}

void AbstractConnection::SetReplicationManager(ReplicationManager* replicationManager)
{
    URHO3D_ASSERT(connection_);

    if (replicationManager_ == replicationManager)
        return;

    // Remove active connection from the old replication manager
    if (replicationManager_ && connection_->IsConnected())
        OnDisconnected();

    replicationManager_ = replicationManager;

    // Add active connection to the new replication manager
    if (replicationManager_ && connection_->IsConnected())
        OnConnected();
}

SharedPtr<AbstractConnection, RefCounted> AbstractConnection::AsSharedPtr()
{
    return SharedPtr<AbstractConnection, RefCounted>(this, connection_);
}

void AbstractConnection::ProcessReplicationMessage(NetworkMessageId messageId, MemoryBuffer& msg, bool& handled)
{
    if (handled || !replicationManager_)
        return;

    if (messageId == MSG_CLOCK_SYNC)
    {
        ClockSynchronizerMessage clockMessage;
        clockMessage.Load(msg);
        GetClock().ProcessMessage(clockMessage);
        handled = true;
    }

    if (replicationManager_)
        handled |= replicationManager_->ProcessMessage(static_cast<AbstractConnection*>(this), messageId, msg);
}

void AbstractConnection::SendReplicationMessages()
{
    if (!connection_ || !connection_->IsConnected() || !replicationManager_)
        return;

    while (const auto clockMessage = GetClock().PollMessage())
        WriteSerializedMessage(*connection_, MSG_CLOCK_SYNC, *clockMessage, PacketType::UnreliableUnordered);
}

ea::string AbstractConnection::ToString() const
{
    if (!connection_ || connection_->IsDisconnected())
        return Format("#{} <disconnected>", GetObjectID());
    return Format("#{} {}:{}", GetObjectID(), connection_->GetAddress(), connection_->GetPort());
}

void AbstractConnection::OnConnected()
{
    URHO3D_ASSERT(connection_);

    if (!replicationManager_)
        return;

    if (replicationManager_->IsServer())
        replicationManager_->GetServerReplicator()->AddConnection(AsSharedPtr());
    else
        replicationManager_->StartClient(AsSharedPtr());
}

void AbstractConnection::OnDisconnected()
{
    URHO3D_ASSERT(connection_);

    if (!replicationManager_)
        return;

    replicationManager_->DropConnection(AsSharedPtr());
}

} // namespace Urho3D
