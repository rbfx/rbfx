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
#include <Urho3D/Core/Signal.h>
#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Network/URL.h>
#include <Urho3D/Network/Transport/NetworkConnection.h>

#include <EASTL/internal/function.h>
#include <EASTL/functional.h>
#include <EASTL/vector.h>


namespace Urho3D
{

class WorkQueue;

class URHO3D_API NetworkServer : public Object
{
    URHO3D_OBJECT(NetworkServer, Object);
public:
    explicit NetworkServer(Context* context);
    /// Start listening for incoming connections. Returns true on success.
    virtual bool Listen(const URL& url) = 0;
    /// Stop listening and disconnect all clients.
    virtual void Stop() = 0;
    /// Return true if server is currently listening for incoming connections.
    virtual bool IsListening() const = 0;

    /// Return snapshot of active connections.
    const ea::vector<SharedPtr<NetworkConnection>>& GetConnections() const;
    /// Set factory for creating connection instances. Must be set before calling Listen.
    void SetConnectionFactory(ea::function<SharedPtr<NetworkConnection>()> factory) { connectionFactory_ = ea::move(factory); }
    /// Creates a new connection instance. May be called from non-main thread.
    SharedPtr<NetworkConnection> CreateConnection();

    Signal<void(NetworkConnection*), NetworkServer> onConnected_;
    Signal<void(NetworkConnection*), NetworkServer> onDisconnected_;
    Signal<void(), NetworkServer> onListenStart_;
    Signal<void(), NetworkServer> onListenStop_;

protected:
    /// Notify Network subsystem that teardown has started.
    void NotifyStopping();

    /// Called once, when new connection is established and ready to be used. Is called from the main thread.
    virtual void OnConnected(NetworkConnection* connection);
    /// Called once, when a fully established connection disconnects gracefully or is aborted abruptly. Is called from the main thread.
    virtual void OnDisconnected(NetworkConnection* connection);
    /// Called once, when server starts listening. Is called from the main thread.
    virtual void OnListenStart();
    /// Called once, when server stops listening. Is called from the main thread.
    virtual void OnListenStop();

    template<typename Callback, bool Add>
    void ExecuteCallbackHelper(NetworkConnection* connection);
    void DoOnConnected(NetworkConnection* connection);
    void DoOnDisconnected(NetworkConnection* connection);
    void DoOnListenStart();
    void DoOnListenStop();
    void AddConnection(NetworkConnection* connection);
    void RemoveConnection(NetworkConnection* connection);

    WorkQueue* workQueue_ = nullptr;
    ea::vector<SharedPtr<NetworkConnection>> connections_;
    ea::function<SharedPtr<NetworkConnection>()> connectionFactory_;
};

}   // namespace Urho3D
