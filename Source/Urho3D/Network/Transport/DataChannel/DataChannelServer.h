// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/Network/NetworkServer.h"

#include <rtc/configuration.hpp>

struct juice_server;

namespace rtc
{

class WebSocket;
class WebSocketServer;
enum class TransportPolicy;

} // namespace rtc

namespace Urho3D
{

class DataChannelConnection;

class URHO3D_API DataChannelServer : public NetworkServer
{
    friend class DataChannelConnection;
    URHO3D_OBJECT(DataChannelServer, NetworkServer);

public:
    explicit DataChannelServer(Context* context);

    static void RegisterObject(Context* context);

    /// Supports "ws" and "wss" schemes. "wss" scheme requires calling %SetTLSCertificate before calling %Listen.
    bool Listen(const URL& url) override;
    void Stop() override;
    bool IsListening() const override;

    void SetTLSCertificate(ea::string_view certificatePemFile, ea::string_view keyPemFile, ea::string_view keyPassword);
    /// Configure ICE servers (STUN/TURN) to use for NAT traversal on all new connections.
    /// Format: "stun:server:port" or "turn:user:pass@server:port"
    void SetIceServers(const StringVector& servers) { iceServers_ = servers; }
    /// Restrict WebRTC to a specific UDP port range for all new connections (default: 1024-65535).
    /// Useful for port-forwarded direct connections where predictable ports are needed.
    void SetPortRange(unsigned begin, unsigned end);
    /// Multiplex all peer connections onto a single UDP port (libjuice only).
    /// The clean solution for dedicated servers — only one port to forward.
    void SetIceUdpMux(bool enable) { enableIceUdpMux_ = enable; }
    /// Force TURN-relay-only mode for all new connections where direct connections are impossible.
    /// When enabled, all media traffic goes through the TURN server.
    void SetIceTransportPolicy(rtc::TransportPolicy policy) { iceTransportPolicy_ = policy; }
    /// Bind to a specific local address for all new connections (multi-homed servers).
    void SetBindAddress(ea::string_view address) { bindAddress_ = address; }
    /// Override network MTU for all new connections (0 = use default).
    void SetMtu(unsigned mtu) { mtu_ = mtu; }

protected:
    ea::shared_ptr<rtc::WebSocketServer> webSocketServer_;

    ea::string certificatePemFile_;
    ea::string keyPemFile_;
    ea::string keyPassword_;
    StringVector iceServers_;
    unsigned portRangeBegin_ = 1024;
    unsigned portRangeEnd_ = 65535;
    bool enableIceUdpMux_{};
    rtc::TransportPolicy iceTransportPolicy_{};
    ea::string bindAddress_;
    unsigned mtu_{};
};

} // namespace Urho3D
