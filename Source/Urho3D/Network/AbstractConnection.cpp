// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Network/AbstractConnection.h"

namespace Urho3D
{

void AbstractConnection::SendMessage(
    NetworkMessageId messageId, ConstByteSpan payload, PacketTypeFlags packetType, ea::string_view debugInfo)
{
    if (payload.size() > GetMaxMessageSize())
    {
        URHO3D_LOGERROR("{}: Message #{} ({} bytes) is too big to send", ToString(), static_cast<unsigned>(messageId),
            payload.size());
        return;
    }

    SendMessageInternal(messageId, payload.data(), payload.size(), packetType);

    Log::GetLogger().Write(GetMessageLogLevel(messageId), "{}: Message #{} ({} bytes) sent{}{}{}{}", ToString(),
        static_cast<unsigned>(messageId), payload.size(), (packetType & PacketType::Reliable) ? ", reliable" : "",
        (packetType & PacketType::Ordered) ? ", ordered" : "", debugInfo.empty() ? "" : ": ", debugInfo);
}

void AbstractConnection::SendMessage(
    NetworkMessageId messageId, const VectorBuffer& msg, PacketTypeFlags packetType, ea::string_view debugInfo)
{
    SendMessage(messageId, msg.GetBuffer(), packetType, debugInfo);
}

void AbstractConnection::LogReceivedMessage(NetworkMessageId messageId, ea::string_view debugInfo) const
{
    Log::GetLogger().Write(GetMessageLogLevel(messageId), "{}: Message #{} received: {}", ToString(),
        static_cast<unsigned>(messageId), debugInfo);
}

LogLevel AbstractConnection::GetMessageLogLevel(NetworkMessageId messageId) const
{
    static const ea::unordered_set<NetworkMessageId> debugMessages = {
        MSG_IDENTITY,
        MSG_SCENELOADED,
        MSG_REQUESTPACKAGE,

        MSG_LOADSCENE,
        MSG_SCENECHECKSUMERROR,
        MSG_PACKAGEINFO,

        MSG_CONFIGURE,
        MSG_SYNCHRONIZED,
    };
    return debugMessages.contains(messageId) ? LOG_DEBUG : LOG_TRACE;
}

LargeMessageWriter::LargeMessageWriter(
    AbstractConnection& connection, NetworkMessageId incompleteMessageId, NetworkMessageId lastMessageId)
    : connection_(connection)
    , buffer_(connection_.GetOutgoingMessageBuffer())
    , debugInfo_(connection_.GetDebugInfoBuffer())
    , incompleteMessageId_(incompleteMessageId)
    , lastMessageId_(lastMessageId)
{
    buffer_.Clear();
    debugInfo_.clear();
}

VectorBuffer& LargeMessageWriter::GetBuffer()
{
    return buffer_;
}

ea::string* LargeMessageWriter::GetDebugInfo()
{
#ifdef URHO3D_LOGGING
    return &debugInfo_;
#else
    return nullptr;
#endif
}

LargeMessageWriter::~LargeMessageWriter()
{
    if (!discarded_)
    {
        const auto payload = ConstByteSpan{buffer_.GetBuffer()};
        const unsigned maxMessageSize = connection_.GetMaxMessageSize();
        const unsigned numChunks = (buffer_.GetSize() + maxMessageSize - 1) / maxMessageSize;
        for (unsigned i = 0; i < numChunks; ++i)
        {
            const bool isLastChunk = i + 1 == numChunks;
            const NetworkMessageId messageId = isLastChunk ? lastMessageId_ : incompleteMessageId_;
            const unsigned chunkOffset = i * maxMessageSize;
            const unsigned chunkSize = isLastChunk ? buffer_.GetSize() % maxMessageSize : maxMessageSize;
            const auto chunkData = payload.subspan(chunkOffset, chunkSize);
            const ea::string& chunkDebugInfo = isLastChunk ? debugInfo_ : EMPTY_STRING;
            connection_.SendMessage(messageId, chunkData, PacketType::ReliableOrdered, chunkDebugInfo);
        }
    }

    buffer_.Clear();
}

void LargeMessageWriter::Discard()
{
    discarded_ = true;
}

LargeMessageReader::LargeMessageReader(
    AbstractConnection& connection, NetworkMessageId incompleteMessageId, NetworkMessageId lastMessageId)
    : buffer_(connection.GetIncomingMessageBuffer())
    , incompleteMessageId_(incompleteMessageId)
    , lastMessageId_(lastMessageId)
{
}

void LargeMessageReader::OnMessage(NetworkMessageId messageId, MemoryBuffer& messageData, Callback onMessageReceived)
{
    URHO3D_ASSERT(messageId == incompleteMessageId_ || messageId == lastMessageId_);

    if (messageId == lastMessageId_ && buffer_.empty())
    {
        onMessageReceived(messageData);
    }
    else
    {
        buffer_.insert(buffer_.end(), messageData.GetData(), messageData.GetData() + messageData.GetSize());
        if (messageId == lastMessageId_)
        {
            MemoryBuffer memoryBuffer{buffer_};
            onMessageReceived(memoryBuffer);
            buffer_.clear();
        }
    }
}

} // namespace Urho3D
