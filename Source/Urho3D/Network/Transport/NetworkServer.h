//
// Copyright (c) 2017-2022 the rbfx project.
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

#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/Network/URL.h>
#include <Urho3D/Network/Transport/NetworkConnection.h>


namespace Urho3D
{

class URHO3D_API NetworkServer : public Object
{
    URHO3D_OBJECT(NetworkServer, Object);
public:
    explicit NetworkServer(Context* context) : Object(context) { }
    virtual bool Listen(const URL& url) = 0;
    virtual void Stop() = 0;

    /// Called once, when new connection is established and ready to be used. May be called from non-main thread.
    ea::function<void(NetworkConnection*)> onConnected_;
    /// Called once, when a fully established connection disconnects gracefully or is aborted abruptly. May be called from non-main thread.
    ea::function<void(NetworkConnection*)> onDisconnected_;
};

}   // namespace Urho3D
