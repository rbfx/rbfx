// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Network/NetworkConnection.h"

#include "Urho3D/Core/Assert.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/Network/NetworkEvents.h"
#include "Urho3D/Network/NetworkServer.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

NetworkConnection::NetworkConnection(Context* context)
    : Object(context)
{
}

void NetworkConnection::HandleConnected()
{
    OnConnected(this);

    using namespace ClientConnected;
    auto& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = this;
    eventData[P_ADDRESS] = address_;
    eventData[P_PORT] = port_;
    SendEvent(E_CLIENTCONNECTED, eventData);
}

void NetworkConnection::HandleDisconnected()
{
    OnDisconnected(this);

    using namespace ClientDisconnected;
    auto& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = this;
    eventData[P_ADDRESS] = address_;
    eventData[P_PORT] = port_;
    SendEvent(E_CLIENTDISCONNECTED, eventData);
}

void NetworkConnection::NotifyDisconnecting()
{
    if (auto* network = GetSubsystem<Network>())
        network->OnConnectionDisconnecting(this);
}

void NetworkConnection::Disconnect()
{
    NotifyDisconnecting();
}

unsigned NetworkConnection::GetMaxMessageSize() const
{
    return MaxNetworkPacketSize;
}

bool NetworkConnection::SendData(ConstByteSpan data, PacketTypeFlags type)
{
    using namespace ClientSendData;
    auto& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = this;
    eventData[P_TYPE] = type.AsInteger();
    eventData[P_SIZE] = static_cast<int>(data.size());
    eventData[P_DATA] = const_cast<unsigned char*>(data.data());
    eventData[P_HANDLED] = false;
    SendEvent(E_CLIENTSENDDATA, eventData);

    bool outHandled = eventData[P_HANDLED].GetBool();
    if (!outHandled)
        OnSendData(this, data, type, outHandled);

    return !outHandled;
}

bool NetworkConnection::SendMessage(
    NetworkMessageId messageId, ConstByteSpan data, PacketTypeFlags type, ea::string_view debugInfo)
{
    using namespace ClientSendMessage;
    auto& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = this;
    eventData[P_MESSAGEID] = static_cast<int>(messageId);
    eventData[P_TYPE] = type.AsInteger();
    eventData[P_SIZE] = static_cast<int>(data.size());
    eventData[P_DATA] = const_cast<unsigned char*>(data.data());
    eventData[P_HANDLED] = false;
    SendEvent(E_CLIENTSENDMESSAGE, eventData);

    bool outHandled = eventData[P_HANDLED].GetBool();

    if (!outHandled)
        OnSendMessage(this, messageId, data, type, debugInfo, outHandled);

    if (outHandled)
        return false;

    outgoing_.WriteVLE(messageId);
    if (!data.empty())
        outgoing_.Write(data.data(), data.size());

    const bool sent = SendData(outgoing_.GetBuffer(), type);
    outgoing_.Clear();
    return sent;
}

unsigned NetworkConnection::GetMaxPayloadSize() const
{
    return ea::max(NetworkMessageHeaderSize, GetMaxMessageSize()) - NetworkMessageHeaderSize;
}

void NetworkConnection::SetServer(NetworkServer* server)
{
    server_ = server;
}

void NetworkConnection::DispatchConnected()
{
    SharedPtr<NetworkConnection> self(this);
    auto workQueue = context_->GetSubsystem<WorkQueue>();
    workQueue->RunTaskOnMainThread([self = std::move(self)]()
    {
        self->state_ = NetworkConnectionState::Connected;
        self->HandleConnected();
    });
}

void NetworkConnection::DispatchDisconnected()
{
    SharedPtr<NetworkConnection> self(this);
    auto workQueue = context_->GetSubsystem<WorkQueue>();
    workQueue->RunTaskOnMainThread([self = std::move(self)]()
    {
        self->state_ = NetworkConnectionState::Disconnected;
        self->HandleDisconnected();
    });
}

void NetworkConnection::DispatchDataReceived(ConstByteSpan message)
{
    if (!processDataOnMainThread_ || Thread::IsMainThread())
    {
        using namespace ClientData;
        auto& eventData = GetEventDataMap();
        eventData[P_CONNECTION] = this;
        eventData[P_SIZE] = static_cast<int>(message.size());
        eventData[P_DATA] = const_cast<unsigned char*>(message.data());
        eventData[P_HANDLED] = false;
        SendEvent(E_CLIENTDATA, eventData);

        bool outHandled = eventData[P_HANDLED].GetBool();

        if (!outHandled)
            OnDataReceived(this, message, outHandled);

        if (!outHandled)
            HandleDataReceived(message);
    }
    else
        QueueIncomingPacket(message);
}

bool NetworkConnection::HandleDataReceived(ConstByteSpan message)
{
    MemoryBuffer messageBuffer{message};
    const auto msgId = static_cast<NetworkMessageId>(messageBuffer.ReadVLE());
    const ConstByteSpan payload{
        messageBuffer.GetData() + messageBuffer.GetPosition(), messageBuffer.GetSize() - messageBuffer.GetPosition()};

    using namespace ClientMessage;
    auto& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = this;
    eventData[P_MESSAGEID] = static_cast<int>(msgId);
    eventData[P_SIZE] = static_cast<int>(payload.size());
    eventData[P_DATA] = const_cast<unsigned char*>(payload.data());
    eventData[P_HANDLED] = false;
    SendEvent(E_CLIENTMESSAGE, eventData);

    bool outHandled = eventData[P_HANDLED].GetBool();

    if (!outHandled)
        OnMessageReceived(this, msgId, payload, outHandled);

    if (outHandled)
        return true;

    return HandleMessageReceived(msgId, payload);
}

void NetworkConnection::QueueIncomingPacket(ConstByteSpan message)
{
    {
        VectorBuffer buffer;

        MutexLock lock(mutex_);
        if (!buffers_.empty())
        {
            buffer = ea::move(buffers_.back());
            buffers_.pop_back();
        }

        buffer.Clear();
        if (!message.empty())
            buffer.Write(message.data(), message.size());
        incoming_.push_back(ea::move(buffer));

        // First packet schedules a callback on main thread. If more packets arrive after that time,
        // they will be processed in the same callback to avoid flooding the main thread with tasks.
        if (incoming_.size() > 1)
            return;
    }

    SharedPtr<NetworkConnection> self(this);
    auto workQueue = context_->GetSubsystem<WorkQueue>();
    workQueue->PostTaskForMainThread([self = std::move(self)]() { self->ProcessQueuedPackets(); });
}

void NetworkConnection::ProcessQueuedPackets()
{
    // Move front buffer to back buffer
    {
        MutexLock lock(mutex_);
        incomingBack_.swap(incoming_);
    }

    // Process back buffer packets while network thread can continue queuing new packets to front buffer
    for (const auto& packet : incomingBack_)
        DispatchDataReceived(packet.GetBuffer());

    // Recycle back buffer vectors
    {
        MutexLock lock(mutex_);
        for (auto& packet : incomingBack_)
            buffers_.push_back(ea::move(packet));
        incomingBack_.clear();
    }
}

} // namespace Urho3D
