// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Network/ReplicatedPeer.h"

#include "Urho3D/Core/Assert.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Network/ClockSynchronizer.h"
#include "Urho3D/Network/MessageUtils.h"
#include "Urho3D/Network/NetworkConnection.h"
#include "Urho3D/Replica/ReplicationManager.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

ReplicatedPeer::ReplicatedPeer(NetworkConnection* connection, const ReplicatedPeerSettings& settings)
    : connection_(connection)
    , clock_(settings.pingIntervalMs_, settings.maxPingMs_, settings.clockBufferSize_, settings.pingBufferSize_,
          settings.getTimestamp_ ? settings.getTimestamp_ : &Time::GetSystemTime)
    , listener_(connection->GetContext())
{
    URHO3D_ASSERT(connection);

    listener_.SubscribeToEvent(E_POSTUPDATE, [this]() { SendReplicationMessages(); });
    connection_->OnMessageReceived.Subscribe(&listener_,
        [this](NetworkMessageId id, ConstByteSpan data, bool& handled) { OnMessageReceived(id, data, handled); });
    connection_->OnConnected.Subscribe(&listener_, [this]() { OnConnected(); });
    connection_->OnDisconnected.Subscribe(&listener_, [this](bool) { OnDisconnected(); });

    URHO3D_LOGINFO("Replicated peer #{} is created for {} connection \"{}\"", GetObjectID(),
        connection->GetServer() != nullptr ? "server" : "client", connection->GetAddress());
}

ReplicatedPeer::~ReplicatedPeer()
{
    URHO3D_LOGINFO("Replicated peer #{} is destroyed", GetObjectID());
}

void ReplicatedPeer::SetReplicationManager(ReplicationManager* replicationManager)
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

SharedPtr<ReplicatedPeer, RefCounted> ReplicatedPeer::AsSharedPtr()
{
    return SharedPtr<ReplicatedPeer, RefCounted>(this, connection_);
}

void ReplicatedPeer::OnMessageReceived(NetworkMessageId messageId, ConstByteSpan msg, bool& handled)
{
    if (handled)
        return;

    MemoryBuffer messageReader{msg};

    if (messageId == MSG_CLOCK_SYNC)
    {
        const auto clockMessage = ReadSerializedMessage<ClockSynchronizerMessage>(messageReader);
        GetClock().ProcessMessage(clockMessage);
        handled = true;
    }

    if (replicationManager_)
        handled |= replicationManager_->ProcessMessage(static_cast<ReplicatedPeer*>(this), messageId, messageReader);
}

void ReplicatedPeer::SendReplicationMessages()
{
    if (!connection_ || !connection_->IsConnected())
        return;

    while (const auto clockMessage = GetClock().PollMessage())
        WriteSerializedMessage(*connection_, MSG_CLOCK_SYNC, *clockMessage, PacketType::UnreliableUnordered);
}

ea::string ReplicatedPeer::ToString() const
{
    if (!connection_ || connection_->IsDisconnected())
        return Format("#{} <disconnected>", GetObjectID());
    return Format("#{} {}:{}", GetObjectID(), connection_->GetAddress(), connection_->GetPort());
}

void ReplicatedPeer::OnConnected()
{
    URHO3D_ASSERT(connection_);

    if (!replicationManager_)
        return;

    if (replicationManager_->IsServer())
    {
        URHO3D_LOGINFO("Replicated peer #{} is attached to server-side scene", GetObjectID());
        replicationManager_->GetServerReplicator()->AddConnection(AsSharedPtr());
    }
    else
    {
        URHO3D_LOGINFO("Replicated peer #{} is attached to client-side scene", GetObjectID());
        replicationManager_->StartClient(AsSharedPtr());
    }
}

void ReplicatedPeer::OnDisconnected()
{
    URHO3D_ASSERT(connection_);

    if (!replicationManager_)
        return;

    replicationManager_->DropConnection(AsSharedPtr());
    URHO3D_LOGINFO("Replicated peer #{} is detached from the scene", GetObjectID());
}

} // namespace Urho3D
