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

#include "Urho3D/Network/Transport/DataChannel/DataChannelServer.h"

#include "Urho3D/Core/Assert.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Network/Transport/DataChannel/DataChannelConnection.h"
#include "Urho3D/Network/Transport/NetworkConnection.h"

#ifndef URHO3D_PLATFORM_WEB
    #include <rtc/websocketserver.hpp>
#endif

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

DataChannelServer::DataChannelServer(Context* context)
    : NetworkServer(context)
{
    SetConnectionFactory([this] { return MakeShared<DataChannelConnection>(context_); });
}

void DataChannelServer::RegisterObject(Context* context)
{
    context->AddAbstractReflection<DataChannelServer>(Category_Network);
}

bool DataChannelServer::Listen(const URL& url)
{
    if (IsListening())
    {
        URHO3D_LOGERROR("DataChannelServer::Listen called while already listening.");
        return false;
    }

#ifndef URHO3D_PLATFORM_WEB
    // Signaling server
    rtc::WebSocketServer::Configuration config = {};
    config.enableTls = url.scheme_ == "wss";
    if (config.enableTls)
    {
        if (certificatePemFile_.empty() || keyPemFile_.empty())
        {
            URHO3D_LOGERROR("Server requires TLS support, but certificate and/or key were not provided.");
            return false;
        }
        config.certificatePemFile = certificatePemFile_.data();
        config.keyPemFile = keyPemFile_.data();
        config.keyPemPass = keyPassword_.data();
    }
    config.port = url.port_;
    webSocketServer_ = ea::make_shared<rtc::WebSocketServer>(config);
    webSocketServer_->onClient([this](std::shared_ptr<rtc::WebSocket> ws)
    {
        SharedPtr<NetworkConnection> connection = CreateConnection();
        SharedPtr<DataChannelConnection> dcConnection;
        dcConnection.DynamicCast(connection);
        URHO3D_ASSERT(dcConnection);
        dcConnection->InitializeFromSocket(this, ws);
    });

    DoOnListenStart();
    return true;
#else
    return false;
#endif
}

bool DataChannelServer::IsListening() const
{
#ifndef URHO3D_PLATFORM_WEB
    return webSocketServer_ != nullptr;
#else
    return false;
#endif
}

void DataChannelServer::Stop()
{
#ifndef URHO3D_PLATFORM_WEB
    if (!webSocketServer_)
        return;

    auto webSocketServer = ea::move(webSocketServer_);
    webSocketServer->stop();
    DoOnListenStop();
#endif
}

void DataChannelServer::SetTLSCertificate(
    ea::string_view certificatePemFile, ea::string_view keyPemFile, ea::string_view keyPassword)
{
    certificatePemFile_ = certificatePemFile;
    keyPemFile_ = keyPemFile;
    keyPassword_ = keyPassword;
}

} // namespace Urho3D
