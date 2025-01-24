// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Network/MessageUtils.h"

namespace Urho3D
{

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

MultiMessageWriter::MultiMessageWriter(
    AbstractConnection& connection, NetworkMessageId messageId, PacketTypeFlags packetType)
    : connection_(connection)
    , buffer_(connection_.GetOutgoingMessageBuffer())
    , debugInfo_(connection_.GetDebugInfoBuffer())
    , messageId_(messageId)
    , packetType_(packetType)
{
    buffer_.Clear();
    debugInfo_.clear();
}

MultiMessageWriter::~MultiMessageWriter()
{
    if (nextPayloadOffset_ != headerSize_)
        SendPreviousPayloads();
}

void MultiMessageWriter::CompleteHeader()
{
    if (headerSize_)
    {
        URHO3D_ASSERTLOG(false, "Common message header ({} bytes) is already completed", *headerSize_);
        return;
    }

    headerSize_ = buffer_.GetSize();
    nextPayloadOffset_ = *headerSize_;

    URHO3D_ASSERTLOG(
        *headerSize_ <= connection_.GetMaxMessageSize(), "Common message header ({} bytes) is too big", *headerSize_);
}

void MultiMessageWriter::CompletePayload()
{
    if (!headerSize_)
        headerSize_ = 0;

    // If latest payload doesn't fit, send the message and move the buffer.
    if (buffer_.GetSize() > connection_.GetMaxMessageSize() && nextPayloadOffset_ != headerSize_)
    {
        SendPreviousPayloads();

        auto& data = buffer_.GetBuffer();
        const unsigned nextPayloadSize = data.size() - nextPayloadOffset_;
        ea::copy(data.begin() + nextPayloadOffset_, data.end(), data.begin() + *headerSize_);
        buffer_.Resize(*headerSize_ + nextPayloadSize);
        debugInfo_.erase(0, nextDebugInfoOffset_);
    }

    // Current payload is ok to send
    nextPayloadOffset_ = buffer_.GetSize();
    nextDebugInfoOffset_ = debugInfo_.size();
}

VectorBuffer& MultiMessageWriter::GetBuffer()
{
    return buffer_;
}

ea::string* MultiMessageWriter::GetDebugInfo()
{
#ifdef URHO3D_LOGGING
    return &debugInfo_;
#else
    return nullptr;
#endif
}

void MultiMessageWriter::SendPreviousPayloads()
{
    const auto payload = ConstByteSpan{buffer_.GetBuffer()}.subspan(0, nextPayloadOffset_);
    const auto debugInfo = ea::string_view{debugInfo_}.substr(0, nextDebugInfoOffset_);
    connection_.SendMessage(messageId_, payload, packetType_, debugInfo);
}

} // namespace Urho3D
