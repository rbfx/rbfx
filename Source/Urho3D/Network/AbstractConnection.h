//
// Copyright (c) 2008-2020 the Urho3D project.
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
#include "../Core/Object.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/Protocol.h"

namespace Urho3D
{

enum class NetworkMessageFlag
{
    None = 0,
    InOrder = 1 << 0,
    Reliable = 1 << 1
};
URHO3D_FLAGSET(NetworkMessageFlag, NetworkMessageFlags);

/// Interface of connection to another host.
/// Virtual for easier unit testing.
class URHO3D_API AbstractConnection : public Object
{
    URHO3D_OBJECT(AbstractConnection, Object);

public:
    AbstractConnection(Context* context) : Object(context) {}

    /// Send message to the other end of the connection.
    virtual void SendMessageInternal(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes) = 0;
    /// Return debug connection string for logging.
    virtual ea::string ToString() const = 0;

    /// Syntax sugar for SendMessage
    /// @{
    void SendLoggedMessage(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes, ea::string_view debugInfo = {})
    {
        SendMessageInternal(messageId, reliable, inOrder, data, numBytes);

        URHO3D_LOGDEBUG("{}: Message #{} ({} bytes) sent{}{}{}{}",
            ToString(),
            static_cast<unsigned>(messageId),
            numBytes,
            reliable ? ", reliable" : "",
            inOrder ? ", ordered" : "",
            debugInfo.empty() ? "" : ": ",
            debugInfo);
    }

    void SendMessage(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes, ea::string_view debugInfo = {})
    {
        SendLoggedMessage(messageId, reliable, inOrder, data, numBytes);
    }

    void SendMessage(NetworkMessageId messageId, bool reliable, bool inOrder, const VectorBuffer& msg, ea::string_view debugInfo = {})
    {
        SendLoggedMessage(messageId, reliable, inOrder, msg.GetData(), msg.GetSize());
    }

    template <class T>
    void SendSerializedMessage(NetworkMessageId messageId, const T& message, NetworkMessageFlags flags)
    {
        const bool reliable = flags.Test(NetworkMessageFlag::Reliable);
        const bool inOrder = flags.Test(NetworkMessageFlag::InOrder);

    #ifdef URHO3D_LOGGING
        const ea::string debugInfo = message.ToString();
    #else
        static const ea::string debugInfo;
    #endif

        msg_.Clear();
        message.Save(msg_);
        SendLoggedMessage(messageId, reliable, inOrder, msg_.GetData(), msg_.GetSize(), debugInfo);
    }

    template <class T>
    void SendGeneratedMessage(NetworkMessageId messageId, NetworkMessageFlags flags, T generator)
    {
        const bool reliable = flags.Test(NetworkMessageFlag::Reliable);
        const bool inOrder = flags.Test(NetworkMessageFlag::InOrder);

    #ifdef URHO3D_LOGGING
        ea::string debugInfo;
        ea::string* debugInfoPtr = &debugInfo;
    #else
        static const ea::string debugInfo;
        ea::string* debugInfoPtr = nullptr;
    #endif

        msg_.Clear();
        generator(msg_, debugInfoPtr);
        SendLoggedMessage(messageId, reliable, inOrder, msg_.GetData(), msg_.GetSize(), debugInfo);
    }

    void OnMessageReceived(NetworkMessageId messageId, MemoryBuffer& messageData) const
    {
        URHO3D_LOGDEBUG("{}: Message #{} received: {} bytes",
            ToString(),
            static_cast<unsigned>(messageId),
            messageData.GetSize());
    }

    template <class T>
    void OnMessageReceived(NetworkMessageId messageId, const T& message) const
    {
        URHO3D_LOGDEBUG("{}: Message #{} received: {}",
            ToString(),
            static_cast<unsigned>(messageId),
            message.ToString());
    }
    /// @}

protected:
    /// Reusable message buffer.
    VectorBuffer msg_;
};

}
