// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
        // Apply ICE configuration to new connection
        dcConnection->SetIceServers(iceServers_);
        dcConnection->SetPortRange(portRangeBegin_, portRangeEnd_);
        dcConnection->SetIceUdpMux(enableIceUdpMux_);
        dcConnection->SetIceTransportPolicy(iceTransportPolicy_);
        if (!bindAddress_.empty())
            dcConnection->SetBindAddress(bindAddress_);
        if (mtu_ > 0)
            dcConnection->SetMtu(mtu_);
        dcConnection->InitializeFromSocket(this, ws);
    });

    DispatchListenStart();
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

    NotifyStopping();

    auto connections = GetConnections();
    for (auto& connection : connections)
        connection->Disconnect();

    auto webSocketServer = ea::move(webSocketServer_);
    webSocketServer->stop();
    DispatchListenStop();
#endif
}

void DataChannelServer::SetTLSCertificate(
    ea::string_view certificatePemFile, ea::string_view keyPemFile, ea::string_view keyPassword)
{
    certificatePemFile_ = certificatePemFile;
    keyPemFile_ = keyPemFile;
    keyPassword_ = keyPassword;
}

void DataChannelServer::SetPortRange(unsigned begin, unsigned end)
{
    portRangeBegin_ = begin;
    portRangeEnd_ = end;
}

} // namespace Urho3D
