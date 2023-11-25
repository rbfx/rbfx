//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/// \file

#pragma once

#include "../Container/FlagSet.h"
#include "../Container/IndexAllocator.h"
#include "../Core/Object.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/Protocol.h"
#include "../Network/PacketTypeFlags.h"

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
    void SendLoggedMessage(NetworkMessageId messageId, const unsigned char* data, unsigned numBytes, PacketTypeFlags packetType = PacketType::ReliableOrdered, ea::string_view debugInfo = {})
    {
        SendMessageInternal(messageId, data, numBytes, packetType);

        Log::GetLogger().Write(GetMessageLogLevel(messageId), "{}: Message #{} ({} bytes) sent{}{}{}{}",
            ToString(),
            static_cast<unsigned>(messageId),
            numBytes,
            (packetType & PacketType::Reliable) ? ", reliable" : "",
            (packetType & PacketType::Ordered) ? ", ordered" : "",
            debugInfo.empty() ? "" : ": ",
            debugInfo);
    }

    void SendMessage(NetworkMessageId messageId, const unsigned char* data = nullptr, unsigned numBytes = 0, PacketTypeFlags packetType = PacketType::ReliableOrdered, ea::string_view debugInfo = {})
    {
        SendLoggedMessage(messageId, data, numBytes, packetType);
    }

    void SendMessage(NetworkMessageId messageId, const VectorBuffer& msg, PacketTypeFlags packetType = PacketType::ReliableOrdered, ea::string_view debugInfo = {})
    {
        SendLoggedMessage(messageId, msg.GetData(), msg.GetSize(), packetType);
    }

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
        SendLoggedMessage(messageId, msg_.GetData(), msg_.GetSize(), messageType, debugInfo);
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
            SendLoggedMessage(messageId, msg_.GetData(), msg_.GetSize(), messageType, debugInfo);
    }

    void OnMessageReceived(NetworkMessageId messageId, MemoryBuffer& messageData) const
    {
        Log::GetLogger().Write(GetMessageLogLevel(messageId), "{}: Message #{} received: {} bytes",
            ToString(),
            static_cast<unsigned>(messageId),
            messageData.GetSize());
    }

    template <class T>
    void OnMessageReceived(NetworkMessageId messageId, const T& message) const
    {
        Log::GetLogger().Write(GetMessageLogLevel(messageId), "{}: Message #{} received: {}",
            ToString(),
            static_cast<unsigned>(messageId),
            message.ToString());
    }

    LogLevel GetMessageLogLevel(NetworkMessageId messageId) const
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
    /// @}

protected:
    /// Reusable message buffer.
    VectorBuffer msg_;
};

}
