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
#include <Urho3D/Network/AbstractConnection.h>
#include <Urho3D/Network/Transport/NetworkConnection.h>
#include <Urho3D/Network/URL.h>

namespace rtc
{

class WebSocket;
class PeerConnection;
class DataChannel;

}

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
    void SendMessage(ea::string_view data, PacketTypeFlags type = PacketType::ReliableOrdered) override;

protected:
    void InitializeFromSocket(DataChannelServer* server, std::shared_ptr<rtc::WebSocket> websocket);
    void OnDataChannelConnected(int index);
    void OnDataChannelDisconnected(int index);

    Mutex mutex_;
    WeakPtr<DataChannelServer> server_ = {};
    std::shared_ptr<rtc::WebSocket> websocket_ = {};
    std::shared_ptr<rtc::PeerConnection> peer_ = {};
    std::shared_ptr<rtc::DataChannel> dataChannels_[4] = {};
    static const unsigned short maxDataSize_ = UINT16_MAX;
    VectorBuffer buffer_;
    bool websocketWasOpened_ = false;
};

}   // namespace Urho3D
