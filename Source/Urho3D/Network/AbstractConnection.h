// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"
#include "Urho3D/Container/IndexAllocator.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Network/Protocol.h"
#include "Urho3D/Network/PacketTypeFlags.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

/// Interface of connection to another host.
/// Virtual for easier unit testing.
class URHO3D_API AbstractConnection : public Object, public IDFamily<AbstractConnection>
{
    URHO3D_OBJECT(AbstractConnection, Object);

public:
    AbstractConnection(Context* context) : Object(context) {}

    void SetMaxPacketSize(unsigned limit) { maxPacketSize_ = limit; }
    unsigned GetMaxPacketSize() const { return maxPacketSize_; }
    unsigned GetMaxMessageSize() const { return maxPacketSize_ - NetworkMessageHeaderSize; }

    /// Send message to the other end of the connection.
    virtual void SendMessageInternal(NetworkMessageId messageId, const unsigned char* data, unsigned numBytes, PacketTypeFlags packetType = PacketType::ReliableOrdered) = 0;
    /// Return debug connection string for logging.
    virtual ea::string ToString() const = 0;
    /// Return whether the clock is synchronized between client and server.
    virtual bool IsClockSynchronized() const = 0;
    /// Convert remote timestamp to local timestamp.
    virtual unsigned RemoteToLocalTime(unsigned time) const = 0;
    /// Convert local timestamp to remote timestamp.
    virtual unsigned LocalToRemoteTime(unsigned time) const = 0;
    /// Return current local time.
    virtual unsigned GetLocalTime() const = 0;
    /// Return local time of last successful ping-pong roundtrip.
    virtual unsigned GetLocalTimeOfLatestRoundtrip() const = 0;
    /// Return ping of the connection.
    virtual unsigned GetPing() const = 0;

    /// Syntax sugar for SendBuffer
    /// @{
    void SendMessage(NetworkMessageId messageId, ConstByteSpan payload = {},
        PacketTypeFlags packetType = PacketType::ReliableOrdered, ea::string_view debugInfo = {});

    void SendMessage(NetworkMessageId messageId, const VectorBuffer& msg,
        PacketTypeFlags packetType = PacketType::ReliableOrdered, ea::string_view debugInfo = {});

    template <class T>
    void SendSerializedMessage(NetworkMessageId messageId, const T& message, PacketTypeFlags messageType = PacketType::ReliableOrdered)
    {
    #ifdef URHO3D_LOGGING
        const ea::string debugInfo = message.ToString();
    #else
        static const ea::string debugInfo;
    #endif

        msg_.Clear();
        message.Save(msg_);
        SendMessage(messageId, msg_.GetBuffer(), messageType, debugInfo);
    }

    template <class T>
    void SendGeneratedMessage(NetworkMessageId messageId, PacketTypeFlags messageType, T generator)
    {
    #ifdef URHO3D_LOGGING
        ea::string debugInfo;
        ea::string* debugInfoPtr = &debugInfo;
    #else
        static const ea::string debugInfo;
        ea::string* debugInfoPtr = nullptr;
    #endif

        msg_.Clear();
        if (generator(msg_, debugInfoPtr))
            SendMessage(messageId, msg_.GetBuffer(), messageType, debugInfo);
    }

    void LogReceivedMessage(NetworkMessageId messageId, ea::string_view debugInfo) const;

    template <class T> void LogReceivedMessage(NetworkMessageId messageId, const T& message) const
    {
        LogReceivedMessage(messageId, ea::string_view{message.ToString()});
    }

    LogLevel GetMessageLogLevel(NetworkMessageId messageId) const;

#ifndef SWIG
    VectorBuffer& GetOutgoingMessageBuffer() { return msg_; }
    ByteVector& GetIncomingMessageBuffer() { return incomingMessageBuffer_; }
    ea::string& GetDebugInfoBuffer() { return debugInfoBuffer_; }
#endif
    /// @}

protected:
    VectorBuffer msg_;

private:
    unsigned maxPacketSize_{DefaultMaxPacketSize};

    ByteVector incomingMessageBuffer_;
    ea::string debugInfoBuffer_;
};

/// Helper class to send large message as multiple small messages.
/// All messages are sent on destruction.
/// Separate large messages will never overlap neither on send nor on receive.
class LargeMessageWriter
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
class LargeMessageReader
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

} // namespace Urho3D
