// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/Mutex.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Network/Network.h"
#include "Urho3D/Network/PacketTypeFlags.h"
#include "Urho3D/Network/Protocol.h"
#include "Urho3D/Network/URL.h"

#include <EASTL/vector.h>

#include <atomic>

namespace Urho3D
{

class ReplicatedPeer;
class NetworkServer;

enum class NetworkConnectionState
{
    /// Connection is fully disconnected and idle.
    Disconnected,
    /// Connection is initiated, but has not completed yet.
    Connecting,
    /// Connection is ready for sending and receiving data.
    Connected,
    /// Disconnection was initiated and no data can be sent through the connection any more.
    Disconnecting,
};

class URHO3D_API NetworkConnection : public Object
{
    URHO3D_OBJECT(NetworkConnection, Object);

public:
    // clang-format off
    Signal<void(), NetworkConnection> OnConnected;
    Signal<void(), NetworkConnection> OnDisconnected;
    Signal<void(ConstByteSpan data, bool& isHandled), NetworkConnection> OnDataReceived;
    Signal<void(NetworkMessageId messageId, ConstByteSpan messagePayload, bool& isHandled), NetworkConnection> OnMessageReceived;
    Signal<void(ConstByteSpan data, PacketTypeFlags, bool& isHandled), NetworkConnection> OnSendData;
    Signal<void(NetworkMessageId messageId, ConstByteSpan messagePayload, PacketTypeFlags, ea::string_view debugInfo, bool& isHandled), NetworkConnection> OnSendMessage;
    // clang-format on

public:
    explicit NetworkConnection(Context* context);

    /// Returns true, if connection initialization has started.
    /// Connection may still be unusable at the time this method returns.
    virtual bool Connect(const URL& url) = 0;
    /// Initializes a disconnection. Connection is no longer usable when this method returns,
    /// even though it may still remain connected for a short while.
    virtual void Disconnect();

    /// Return maximum size of raw network message.
    virtual unsigned GetMaxMessageSize() const;
    /// Sends data as-is via underlying transport. Data may be copied and sent asynchronously.
    virtual bool SendData(ConstByteSpan data, PacketTypeFlags type = PacketType::ReliableOrdered);
    /// Sends network message. Data may be copied and sent asynchronously.
    virtual bool SendMessage(NetworkMessageId messageId, ConstByteSpan data,
        PacketTypeFlags type = PacketType::ReliableOrdered, ea::string_view debugInfo = {});

    /// Return maximum size of network message payload.
    unsigned GetMaxPayloadSize() const;

    /// Result may be empty, when connection is not %NetworkConnectionState::Connected.
    const ea::string& GetAddress() const { return address_; }
    /// Result may be 0, when connection is not %NetworkConnectionState::Connected or when result is not applicable
    /// to the underlying transport.
    unsigned short GetPort() const { return port_; }
    /// Return current state of the connection.
    NetworkConnectionState GetState() const { return state_; }
    /// Return whether the connection is currently connected.
    bool IsConnected() const { return state_ == NetworkConnectionState::Connected; }
    /// Return whether the connection is currently disconnected.
    bool IsDisconnected() const { return state_ == NetworkConnectionState::Disconnected; }

    /// Return whether callbacks are invoked on the main thread.
    bool GetProcessDataOnMainThread() const { return processDataOnMainThread_; }
    /// Set whether callbacks are invoked on the main thread.
    void SetProcessDataOnMainThread(bool enable) { processDataOnMainThread_ = enable; }
    /// Return server that owns this connection, if any.
    NetworkServer* GetServer() const { return server_; }

protected:
    /// Notify Network subsystem that teardown has started.
    void NotifyDisconnecting();
    /// Set server that owns this connection.
    void SetServer(NetworkServer* server);

    /// Called once, when connection is fully set up and data is ready to be sent and received.
    /// Is called from the main thread.
    virtual void HandleConnected();
    /// Called once, when connection disconnect was initiated (only if OnConnected was also called).
    /// Is called from the main thread.
    virtual void HandleDisconnected();
    /// Called when a new data is received. May be called from non-main thread depending on configuration.
    /// Returns true when the message was processed and should not be passed to other handlers.
    virtual bool HandleDataReceived(ConstByteSpan message);
    /// Called when a new network message is received. May be called from non-main thread.
    /// Returns true when the message was processed and should not be passed to other handlers.
    virtual bool HandleMessageReceived(NetworkMessageId messageId, ConstByteSpan message) { return false; }

    /// Dispatch connection callback from transport implementation.
    void DispatchConnected();
    /// Dispatch disconnection callback from transport implementation.
    void DispatchDisconnected();
    /// Dispatch data callback from transport implementation.
    void DispatchDataReceived(ConstByteSpan message);
    /// Dispatch message callback from transport implementation.
    void DispatchMessageReceived(NetworkMessageId messageId, ConstByteSpan message);

protected:
    NetworkConnectionState state_ = NetworkConnectionState::Disconnected;
    ea::string address_ = "";
    unsigned short port_ = 0;

private:
    void QueueIncomingPacket(ConstByteSpan message);
    void ProcessQueuedPackets();

private:
    VectorBuffer outgoing_;
    ea::string debugInfo_;
    std::atomic<bool> processDataOnMainThread_ = true;
    WeakPtr<NetworkServer> server_;

    Mutex mutex_{};
    ea::vector<VectorBuffer> incoming_;
    ea::vector<VectorBuffer> incomingBack_;
    ea::vector<VectorBuffer> buffers_;
};

} // namespace Urho3D
