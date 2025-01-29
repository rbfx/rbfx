// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"
#include "Urho3D/Container/IndexAllocator.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/IO/Log.h"
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
    AbstractConnection(Context* context);

    /// Connection limits.
    /// @{
    unsigned GetMaxPacketSize() const;
    unsigned GetMaxMessageSize() const;
    /// @}

    /// Set maximum size of network packet. Connection transport may override this value.
    virtual void SetMaxPacketSize(unsigned limit);
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

    /// Syntax sugar for sending and receiving messages.
    /// @{
    void SendMessage(NetworkMessageId messageId, ConstByteSpan payload = {},
        PacketTypeFlags packetType = PacketType::ReliableOrdered, ea::string_view debugInfo = {});

    void SendMessage(NetworkMessageId messageId, const VectorBuffer& msg,
        PacketTypeFlags packetType = PacketType::ReliableOrdered, ea::string_view debugInfo = {});

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
    unsigned maxPacketSize_{};

    ByteVector incomingMessageBuffer_;
    ea::string debugInfoBuffer_;
};

} // namespace Urho3D
