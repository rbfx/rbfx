// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Network/NetworkConnection.h"
#include "Urho3D/Network/URL.h"

#include <rtc/configuration.hpp>

namespace rtc
{

class WebSocket;
class PeerConnection;
class DataChannel;
enum class TransportPolicy;

} // namespace rtc

namespace Urho3D
{

class DataChannelServer;

class URHO3D_API DataChannelConnection : public NetworkConnection
{
    friend class DataChannelServer;

    URHO3D_OBJECT(DataChannelConnection, NetworkConnection);

public:
    explicit DataChannelConnection(Context* context);
    ~DataChannelConnection();

    static void RegisterObject(Context* context);

    /// Address may be a full URL and port may be set to 0. Otherwise, port is appended to address.
    bool Connect(const URL& url) override;
    void Disconnect() override;
    bool SendData(ConstByteSpan data, PacketTypeFlags type = PacketType::ReliableOrdered) override;
    unsigned GetMaxMessageSize() const override;

    /// Configure ICE servers (STUN/TURN) for NAT traversal.
    /// Format: "stun:server:port" or "turn:user:pass@server:port"
    void SetIceServers(const StringVector& servers) { iceServers_.assign(servers.begin(), servers.end()); }
    /// Restrict WebRTC to a specific UDP port range (default: 1024-65535).
    /// Useful for port-forwarded direct connections where predictable ports are needed.
    void SetPortRange(unsigned begin, unsigned end);
    /// Multiplex all peer connections onto a single UDP port (libjuice only).
    /// The clean solution for dedicated servers — only one port to forward.
    void SetIceUdpMux(bool enable) { enableIceUdpMux_ = enable; }
    /// Force TURN-relay-only mode for strict firewalls where direct connections are impossible.
    /// When enabled, all media traffic goes through the TURN server.
    void SetIceTransportPolicy(rtc::TransportPolicy policy) { iceTransportPolicy_ = policy; }
    /// Bind to a specific local address (multi-homed servers with multiple NICs).
    void SetBindAddress(ea::string_view address) { bindAddress_ = address; }
    /// Override network MTU for WebRTC data channels (0 = use default).
    /// Useful for VPNs or tunnels with reduced path MTU to avoid UDP fragmentation.
    void SetMtu(unsigned mtu) { mtu_ = mtu; }
    /// Access the underlying PeerConnection for advanced usage (ICE state, candidates, etc.).
    /// Requires knowledge of the rtc:: library. See WebRTC documentation for PeerConnection API.
    std::shared_ptr<rtc::PeerConnection> GetPeer() const { return peer_; }
    /// Initialize with a pre-connected WebSocket (for relay/custom signaling).
    /// Allows using external signaling servers instead of direct WebSocket connections.
    void InitializeWithWebSocket(std::shared_ptr<rtc::WebSocket> ws, DataChannelServer* server = nullptr);

protected:
    void InitializeFromSocket(DataChannelServer* server, std::shared_ptr<rtc::WebSocket> websocket);
    void OnDataChannelConnected(int index);
    void OnDataChannelDisconnected(int index, bool notifyCallbacks = true);

    std::shared_ptr<rtc::WebSocket> websocket_;
    std::shared_ptr<rtc::PeerConnection> peer_;
    std::shared_ptr<rtc::DataChannel> dataChannels_[4];
    VectorBuffer buffer_;
    bool websocketWasOpened_{};

    StringVector iceServers_;
    unsigned portRangeBegin_ = 1024;
    unsigned portRangeEnd_ = 65535;
    bool enableIceUdpMux_{};
    rtc::TransportPolicy iceTransportPolicy_{};
    ea::string bindAddress_;
    unsigned mtu_{};
};

} // namespace Urho3D
