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
#include <Urho3D/Network/Transport/NetworkServer.h>

#include <rtc/configuration.hpp>

struct juice_server;

namespace rtc
{

class WebSocket;
class WebSocketServer;

}

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
    void SetIceServers(ea::span<const ea::string_view> servers);
    /// Restrict WebRTC to a specific UDP port range for all new connections (default: 1024-65535).
    /// Useful for port-forwarded direct connections where predictable ports are needed.
    void SetPortRange(uint16_t begin, uint16_t end) { portRangeBegin_ = begin; portRangeEnd_ = end; }
    /// Multiplex all peer connections onto a single UDP port (libjuice only).
    /// The clean solution for dedicated servers — only one port to forward.
    void SetIceUdpMux(bool enable) { enableIceUdpMux_ = enable; }
    /// Force TURN-relay-only mode for all new connections where direct connections are impossible.
    /// When enabled, all media traffic goes through the TURN server.
    void SetIceTransportPolicy(rtc::TransportPolicy policy) { iceTransportPolicy_ = policy; }
    /// Bind to a specific local address for all new connections (multi-homed servers).
    void SetBindAddress(ea::string_view address) { bindAddress_ = address; }
    /// Override network MTU for all new connections (0 = use default).
    void SetMtu(size_t mtu) { mtu_ = mtu; }

protected:
    ea::shared_ptr<rtc::WebSocketServer> webSocketServer_ = {};
    ea::string certificatePemFile_;
    ea::string keyPemFile_;
    ea::string keyPassword_;
    ea::vector<ea::string> iceServers_;
    uint16_t portRangeBegin_ = 1024;
    uint16_t portRangeEnd_ = 65535;
    bool enableIceUdpMux_ = false;
    rtc::TransportPolicy iceTransportPolicy_ = rtc::TransportPolicy::All;
    ea::string bindAddress_;
    size_t mtu_ = 0;
};

}   // namespace Urho3D
