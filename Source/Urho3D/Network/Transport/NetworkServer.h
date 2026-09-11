// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Network/Transport/NetworkConnection.h"
#include "Urho3D/Network/URL.h"

#include <EASTL/functional.h>
#include <EASTL/internal/function.h>
#include <EASTL/vector.h>

namespace Urho3D
{

class URHO3D_API NetworkServer : public Object
{
    URHO3D_OBJECT(NetworkServer, Object);

public:
    Signal<void(NetworkConnection*), NetworkServer> OnConnected;
    Signal<void(NetworkConnection*), NetworkServer> OnDisconnected;
    Signal<void(), NetworkServer> OnListenStart;
    Signal<void(), NetworkServer> OnListenStop;

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
    void SetConnectionFactory(ea::function<SharedPtr<NetworkConnection>()> factory)
    {
        connectionFactory_ = ea::move(factory);
    }
    /// Creates a new connection instance. May be called from non-main thread.
    SharedPtr<NetworkConnection> CreateConnection();

protected:
    /// Notify Network subsystem that teardown has started.
    void NotifyStopping();
    void AddConnection(const SharedPtr<NetworkConnection>& connection);
    void RemoveConnection(const SharedPtr<NetworkConnection>& connection);

    /// Called once, when new connection is established and ready to be used.
    /// Is called from the main thread.
    virtual void HandleConnected(const SharedPtr<NetworkConnection>& connection);
    /// Called once, when a fully established connection disconnects gracefully or is aborted abruptly.
    /// Is called from the main thread.
    virtual void HandleDisconnected(const SharedPtr<NetworkConnection>& connection);
    /// Called once, when server starts listening. Is called from the main thread.
    virtual void HandleListenStart();
    /// Called once, when server stops listening. Is called from the main thread.
    virtual void HandleListenStop();

    void DispatchConnected(const SharedPtr<NetworkConnection>& connection);
    void DispatchDisconnected(const SharedPtr<NetworkConnection>& connection);
    void DispatchListenStart();
    void DispatchListenStop();

private:
    ea::vector<SharedPtr<NetworkConnection>> connections_;
    ea::function<SharedPtr<NetworkConnection>()> connectionFactory_;
};

} // namespace Urho3D
