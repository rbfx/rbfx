#include "Urho3D/Network/Transport/NetworkConnection.h"

#include "Urho3D/Core/Assert.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/Network/Transport/NetworkServer.h"

namespace Urho3D
{

NetworkMessageBuilder::NetworkMessageBuilder(
    NetworkConnection* connection, PacketTypeFlags type, VectorBuffer& buffer, unsigned offset)
    : buffer_(buffer)
    , offset_(offset)
    , finished_(false)
    , connection_(connection)
    , type_(type)
{
}

NetworkMessageBuilder::~NetworkMessageBuilder()
{
    EndMessage();
}

void NetworkMessageBuilder::EndMessage()
{
    if (finished_ || !IsValid())
        return;

    connection_->EndMessage(*this);
    finished_ = true;
}

NetworkConnection::NetworkConnection(Context* context)
    : Object(context)
    , workQueue_(context->GetSubsystem<WorkQueue>())
{
    URHO3D_ASSERT(workQueue_);
}

unsigned NetworkConnection::GetMaxMessageSize() const
{
    return ea::min(MaxNetworkMessageSize, maxPacketSize_ - NetworkMessageHeaderSize);
}

bool NetworkConnection::SendData(const MemoryBuffer& data, PacketTypeFlags type)
{
    bool outHandled = false;
    onSendData_(this, data, type, outHandled);
    return !outHandled;
}

bool NetworkConnection::SendMessage(
    NetworkMessageId messageId, const MemoryBuffer& data, PacketTypeFlags type, ea::string_view debugInfo)
{
    debugInfo_ = debugInfo;
    bool outHandled = false;
    onSendMessage_(this, messageId, data, type, outHandled);
    if (outHandled)
        return false;

    auto& buffer = outgoing_;
    buffer.WriteVLE(messageId);

    if (data.GetSize() > 0)
        buffer.Write(data.GetData(), data.GetSize());

    const bool sent = SendData(buffer, type);
    buffer.Clear();
    return sent;
}

NetworkMessageBuilder NetworkConnection::BeginMessage(
    NetworkMessageId messageId, PacketTypeFlags type, ea::string_view debugInfo)
{
    debugInfo_ = debugInfo;
    auto& buffer = outgoing_;
    buffer.WriteVLE(messageId);

    NetworkMessageBuilder builder{this, type, buffer, buffer.Tell()};
    return builder;
}

void NetworkConnection::EndMessage(NetworkMessageBuilder& builder)
{
    auto& buffer = builder.buffer_;
    SendData(buffer, builder.type_);
    buffer.Clear();
}

void NetworkConnection::SetServer(NetworkServer* server)
{
    server_ = server;
}

void NetworkConnection::DoOnConnected()
{
    SharedPtr<NetworkConnection> ref(this);
    TaskFunction&& cb = [self = std::move(ref)](unsigned, WorkQueue*)
    {
        self->OnConnected();
        self->state_ = State::Connected;
    };
    workQueue_->RunTaskOnMainThread(cb);
}

void NetworkConnection::DoOnDisconnected()
{
    SharedPtr<NetworkConnection> ref(this);
    TaskFunction&& cb = [self = std::move(ref)](unsigned, WorkQueue*)
    {
        self->OnDisconnected();
        self->state_ = State::Disconnected;
        self->selfRef_ = nullptr;
    };
    workQueue_->RunTaskOnMainThread(cb);
}

void NetworkConnection::DoOnData(MemoryBuffer& message)
{
    if (!processDataOnMainThread_ || Thread::IsMainThread())
    {
        bool outHandled = false;
        onData_(this, message, outHandled);
        if (!outHandled)
            OnData(message);
    }
    else
        QueueIncomingPacket(message);
}

bool NetworkConnection::OnData(MemoryBuffer& message)
{
    const auto msgId = static_cast<NetworkMessageId>(message.ReadVLE());
    bool outHandled = false;
    onMessage_(this, msgId, message, outHandled);
    if (outHandled)
        return true;
    return OnMessage(msgId, message);
}

void NetworkConnection::QueueIncomingPacket(const MemoryBuffer& message)
{
    VectorBuffer buffer;
    {
        MutexLock lock(mutex_);
        if (!buffers_.empty())
        {
            buffer = ea::move(buffers_.back());
            buffers_.pop_back();
        }

        buffer.Clear();
        if (message.GetSize() > 0)
            buffer.Write(message.GetData(), message.GetSize());
        incoming_.push_back(ea::move(buffer));

        // First packet schedules a callback on main thread. If more packets arrive after that time,
        // they will be processed in the same callback to avoid flooding the main thread with tasks.
        if (incoming_.size() > 1)
            return;
    }

    SharedPtr<NetworkConnection> ref(this);
    TaskFunction&& cb = [self = std::move(ref)](unsigned, WorkQueue*) { self->ProcessQueuedPackets(); };
    workQueue_->PostTaskForMainThread(cb);
}

void NetworkConnection::ProcessQueuedPackets()
{
    // Move front buffer to back buffer
    {
        MutexLock lock(mutex_);
        incomingBack_.swap(incoming_);
    }

    // Process back buffer packets while network thread can continue queuing new packets to front buffer
    for (auto& packet : incomingBack_)
    {
        MemoryBuffer message(packet);
        bool outHandled = false;
        onData_(this, message, outHandled);
        if (!outHandled)
            OnData(message);
    }

    // Recycle back buffer vectors
    {
        MutexLock lock(mutex_);
        for (auto& packet : incomingBack_)
            buffers_.push_back(ea::move(packet));
        incomingBack_.clear();
    }
}

} // namespace Urho3D
