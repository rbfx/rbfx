// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Network/AbstractConnection.h"
#include "Urho3D/Network/URL.h"

namespace Urho3D
{

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

    explicit NetworkConnection(Context* context) : Object(context) { }
    /// Returns true, if connection initialization has started. Connection may still be unusable at the time this method returns.
    virtual bool Connect(const URL& url) = 0;
    /// Initializes a disconnection. Connection is no longer usable when this method returns, even though it may still remain connected for a short while.
    virtual void Disconnect() = 0;
    /// Copies data and queues it for sending.
    virtual void SendMessage(ea::string_view data, PacketTypeFlags type = PacketType::ReliableOrdered) = 0;
    /// Return maximum size of network message.
    virtual unsigned GetMaxMessageSize() const = 0;
    /// Result may be empty, when connection is not %State::Connected.
    ea::string GetAddress() const { return address_; }
    /// Result may be 0, when connection is not %State::Connected or when result is not applicable to the underlying transport.
    unsigned short GetPort() const { return port_; }
    /// Return current state of the connection.
    State GetState() const { return state_; }

    /// Called once, when connection is fully set up and data is ready to be sent and received. May be called from non-main thread.
    ea::function<void()> onConnected_;
    /// Called once, when connection disconnect was initiated (only if onConnected_ was also called). May be called from non-main thread.
    ea::function<void()> onDisconnected_;
    /// Called once, if connection fails to connect (only if onConnected_ was never called). May be called from non-main thread.
    ea::function<void()> onError_;
    /// Called when a new network message is received. May be called from non-main thread.
    ea::function<void(ea::string_view)> onMessage_;

protected:
    State state_ = State::Disconnected;
    ea::string address_ = "";
    unsigned short port_ = 0;
};

}   // namespace Urho3D
