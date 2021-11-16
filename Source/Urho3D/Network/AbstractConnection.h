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

#include "../Core/Object.h"
#include "../IO/VectorBuffer.h"
#include "../Network/Protocol.h"

namespace Urho3D
{

/// %Connection to a remote network host.
class URHO3D_API AbstractConnection : public Object
{
    URHO3D_OBJECT(AbstractConnection, Object);

public:
    AbstractConnection(Context* context) : Object(context) {}

    /// Send message to the other end of the connection.
    virtual void SendMessage(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes) = 0;
    /// Return debug connection string for logging.
    virtual ea::string ToString() const = 0;

    /// Syntax sugar for SendMessage
    /// @{
    void SendMessage(NetworkMessageId messageId, bool reliable, bool inOrder, const VectorBuffer& msg)
    {
        SendMessage(messageId, reliable, inOrder, msg.GetData(), msg.GetSize());
    }

    template <class T>
    void SendMessage(const T& message, bool reliable, bool inOrder)
    {
        msg_.Clear();
        message.Save(msg_);
        SendMessage(T::Id, reliable, inOrder, msg_.GetData(), msg_.GetSize());
    }
    /// @}

protected:
    /// Reusable message buffer.
    VectorBuffer msg_;
};

}
