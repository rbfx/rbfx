// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Mutex.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Container/Ptr.h"
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

class NetworkConnection;
class WorkQueue;
class ReplicatedPeer;
class NetworkServer;

#ifndef SWIG
/// RAII wrapper for building network messages. Automatically calls NetworkConnection::EndMessage() if not already called.
struct URHO3D_API NetworkMessageBuilder
{
    ~NetworkMessageBuilder();

    /// Check if the builder is valid.
    bool IsValid() const { return connection_ != nullptr; }
    /// Complete the message and send it immediately.
    void EndMessage();

    VectorBuffer& buffer_;
    unsigned offset_;
    bool finished_;

protected:
    NetworkMessageBuilder(NetworkConnection* connection, PacketTypeFlags type, VectorBuffer& buffer, unsigned offset);

private:
    NetworkConnection* connection_ = nullptr;
    PacketTypeFlags type_ = PacketType::ReliableOrdered;

    friend class NetworkConnection;
};
#endif

class URHO3D_API NetworkConnection : public Object
{
    URHO3D_OBJECT(NetworkConnection, Object);

public:
    enum class State
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

    explicit NetworkConnection(Context* context);
    /// Returns true, if connection initialization has started. Connection may still be unusable at the time this method returns.
    virtual bool Connect(const URL& url) = 0;
    /// Initializes a disconnection. Connection is no longer usable when this method returns, even though it may still remain connected for a short while.
    virtual void Disconnect() { selfRef_ = this; }
    /// Copies data and queues it for sending.
    virtual bool SendData(const MemoryBuffer& data, PacketTypeFlags type = PacketType::ReliableOrdered);
    /// Copies data and queues it for sending.
    virtual bool SendMessage(NetworkMessageId messageId, const MemoryBuffer& data, PacketTypeFlags type = PacketType::ReliableOrdered, ea::string_view debugInfo = {});
    /// Return maximum size of network message.
    virtual unsigned GetMaxMessageSize() const;
#ifndef SWIG
    /// Begins a message by returning a builder that writes into the outgoing buffer.
    /// The builder automatically calls EndMessage() if not explicitly called, providing RAII semantics.
    virtual NetworkMessageBuilder BeginMessage(NetworkMessageId messageId, PacketTypeFlags type = PacketType::ReliableOrdered, ea::string_view debugInfo = {});
    /// Complete a message begun with BeginMessage(). Called by NetworkMessageBuilder destructor or explicitly.
    /// Customizable in subclasses to modify message structure.
    virtual void EndMessage(NetworkMessageBuilder& builder);
#endif
    /// Result may be empty, when connection is not %State::Connected.
    const ea::string& GetAddress() const { return address_; }
    /// Result may be 0, when connection is not %State::Connected or when result is not applicable to the underlying transport.
    unsigned short GetPort() const { return port_; }
    /// Return current state of the connection.
    State GetState() const { return state_; }
    /// Return whether the connection is currently connected.
    bool IsConnected() const { return state_ == State::Connected; }
    /// Return whether the connection is currently disconnected.
    bool IsDisconnected() const { return state_ == State::Disconnected; }
    /// Return maximum size of network packet.
    unsigned GetMaxPacketSize() const { return maxPacketSize_; }
    /// Set maximum size of network packet.
    void SetMaxPacketSize(unsigned limit) { maxPacketSize_ = limit; }
#ifndef SWIG
    /// Return debug info buffer for constructing messages directly.
    ea::string& GetDebugInfoBuffer() { return debugInfo_; }
    /// Return debug info buffer for constructing messages directly.
    const ea::string& GetDebugInfoBuffer() const { return debugInfo_; }
#endif
    /// Return whether callbacks are invoked on the main thread.
    bool GetProcessDataOnMainThread() const { return processDataOnMainThread_; }
    /// Set whether callbacks are invoked on the main thread.
    void SetProcessDataOnMainThread(bool enable) { processDataOnMainThread_ = enable; }
    /// Called once, when connection is fully set up and data is ready to be sent and received. Is called from the main thread.
    virtual void OnConnected() { onConnected_(this); }
    /// Called once, when connection disconnect was initiated (only if onConnected_ was also called). Is called from the main thread.
    virtual void OnDisconnected() { onDisconnected_(this); selfRef_ = nullptr; }
    /// Called when a new data is received. May be called from non-main thread depending on configuration.
    /// Returns true when the message was processed and should not be passed to other handlers.
    virtual bool OnData(MemoryBuffer& message);
    /// Called when a new network message is received. May be called from non-main thread.
    /// Returns true when the message was processed and should not be passed to other handlers.
    virtual bool OnMessage(NetworkMessageId messageId, MemoryBuffer& message) { return false; }
    /// Return server that owns this connection, if any.
    NetworkServer* GetServer() const { return server_; }

    Signal<void(), NetworkConnection> onConnected_;
    Signal<void(), NetworkConnection> onDisconnected_;
    Signal<void(MemoryBuffer&, bool&), NetworkConnection> onData_;
    Signal<void(NetworkMessageId, MemoryBuffer&, bool&), NetworkConnection> onMessage_;
    Signal<void(const MemoryBuffer&, PacketTypeFlags, bool&), NetworkConnection> onSendData_;
    Signal<void(NetworkMessageId, const MemoryBuffer&, PacketTypeFlags, bool&), NetworkConnection> onSendMessage_;

protected:
    /// Set server that owns this connection.
    void SetServer(NetworkServer* server);
    /// Dispatch connection callback from transport implementation.
    void DoOnConnected();
    /// Dispatch disconnection callback from transport implementation.
    void DoOnDisconnected();
    /// Dispatch data callback from transport implementation.
    void DoOnData(MemoryBuffer& message);
    /// Dispatch message callback from transport implementation.
    void DoOnMessage(NetworkMessageId messageId, MemoryBuffer& message);

    SharedPtr<NetworkConnection> selfRef_;
    State state_ = State::Disconnected;
    ea::string address_ = "";
    unsigned maxPacketSize_ = MaxNetworkPacketSize;
    unsigned short port_ = 0;
    VectorBuffer outgoing_{};
    ea::string debugInfo_ = "";
    std::atomic<bool> processDataOnMainThread_ = true;
    WeakPtr<NetworkServer> server_;

private:
    void QueueIncomingPacket(const MemoryBuffer& message);
    void ProcessQueuedPackets();

    Mutex mutex_{};
    WorkQueue* workQueue_ = nullptr;
    ea::vector<VectorBuffer> incoming_;
    ea::vector<VectorBuffer> incomingBack_;
    ea::vector<VectorBuffer> buffers_;
};

}   // namespace Urho3D
