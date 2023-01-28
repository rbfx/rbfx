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
    void SetTLSCertificate(ea::string_view certificatePemFile, ea::string_view keyPemFile, ea::string_view keyPassword);

protected:
    void OnDisconnected(DataChannelConnection* connection);

    ea::shared_ptr<rtc::WebSocketServer> webSocketServer_ = {};
    ea::vector<SharedPtr<DataChannelConnection>> connections_;
    ea::string certificatePemFile_;
    ea::string keyPemFile_;
    ea::string keyPassword_;
};

}   // namespace Urho3D
