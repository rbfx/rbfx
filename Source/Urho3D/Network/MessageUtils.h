// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/Network/AbstractConnection.h"

#include <EASTL/optional.h>

namespace Urho3D
{

/// Helper class to send large message as multiple small messages.
/// All messages are sent on destruction.
/// Separate large messages will never overlap neither on send nor on receive.
class URHO3D_API LargeMessageWriter
{
public:
    LargeMessageWriter(
        AbstractConnection& connection, NetworkMessageId incompleteMessageId, NetworkMessageId lastMessageId);
    ~LargeMessageWriter();

    void Discard();

    VectorBuffer& GetBuffer();
    ea::string* GetDebugInfo();

private:
    AbstractConnection& connection_;
    VectorBuffer& buffer_;
    ea::string& debugInfo_;
    const NetworkMessageId incompleteMessageId_;
    const NetworkMessageId lastMessageId_;

    bool discarded_{};
};

/// Helper class to reassemble large messages.
class URHO3D_API LargeMessageReader
{
public:
    using Callback = ea::function<void(MemoryBuffer& messageData)>;

    LargeMessageReader(
        AbstractConnection& connection, NetworkMessageId incompleteMessageId, NetworkMessageId lastMessageId);

    void OnMessage(NetworkMessageId messageId, MemoryBuffer& messageData, Callback onMessageReceived);

private:
    ByteVector& buffer_;
    const NetworkMessageId incompleteMessageId_;
    const NetworkMessageId lastMessageId_;
};

/// Helper class to send multiple messages of the same type with the same common header.
/// Messages are sent as soon as maximum packet size is reached.
/// Message without payloads is not sent.
/// Size of header and single payload should not exceed maximum message size.
class URHO3D_API MultiMessageWriter
{
public:
    MultiMessageWriter(AbstractConnection& connection, NetworkMessageId messageId, PacketTypeFlags packetType);
    ~MultiMessageWriter();

    /// Complete shared header that is going to be sent for each individual message. Could be empty.
    void CompleteHeader();
    /// Complete individual payload. Single message will contain one or more payloads.
    void CompletePayload();

    VectorBuffer& GetBuffer();
    ea::string* GetDebugInfo();

private:
    void SendPreviousPayloads();

    AbstractConnection& connection_;
    VectorBuffer& buffer_;
    ea::string& debugInfo_;
    const NetworkMessageId messageId_;
    const PacketTypeFlags packetType_;

    ea::optional<unsigned> headerSize_;
    unsigned nextPayloadOffset_{};
    unsigned nextDebugInfoOffset_{};
};

/// Read simple network message as object.
template <class T> T ReadSerializedMessage(MemoryBuffer& src)
{
    T msg;
    msg.Load(src);
    return msg;
}

/// Write simple network message from object.
template <class T>
void WriteSerializedMessage(AbstractConnection& connection, NetworkMessageId messageId, const T& message,
    PacketTypeFlags messageType = PacketType::ReliableOrdered)
{
#ifdef URHO3D_LOGGING
    const ea::string debugInfo = message.ToString();
#else
    const ea::string& debugInfo = EMPTY_STRING;
#endif

    VectorBuffer& buffer = connection.GetOutgoingMessageBuffer();
    buffer.Clear();
    message.Save(buffer);
    connection.SendMessage(messageId, buffer.GetBuffer(), messageType, debugInfo);
}

} // namespace Urho3D
