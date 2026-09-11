// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Network/NetworkServer.h"

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/Assert.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Network/Network.h"
#include "Urho3D/Network/NetworkEvents.h"

#include <EASTL/algorithm.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

NetworkServer::NetworkServer(Context* context)
    : Object(context)
{
}

void NetworkServer::NotifyStopping()
{
    if (auto* network = GetSubsystem<Network>())
        network->OnServerStopping(this);
}

void NetworkServer::HandleConnected(const SharedPtr<NetworkConnection>& connection)
{
    OnConnected(this, connection);

    using namespace ServerClientConnected;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    eventData[P_CONNECTION] = connection;
    eventData[P_ADDRESS] = connection->GetAddress();
    eventData[P_PORT] = connection->GetPort();
    SendEvent(E_SERVERCLIENTCONNECTED, eventData);
}

void NetworkServer::HandleDisconnected(const SharedPtr<NetworkConnection>& connection)
{
    OnDisconnected(this, connection);

    using namespace ServerClientDisconnected;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    eventData[P_CONNECTION] = connection;
    eventData[P_ADDRESS] = connection->GetAddress();
    eventData[P_PORT] = connection->GetPort();
    SendEvent(E_SERVERCLIENTDISCONNECTED, eventData);

    RemoveConnection(connection);
}

void NetworkServer::HandleListenStart()
{
    OnListenStart(this);

    using namespace ServerListenStart;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    SendEvent(E_SERVERLISTENSTART, eventData);
}

void NetworkServer::HandleListenStop()
{
    OnListenStop(this);

    using namespace ServerListenStop;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    SendEvent(E_SERVERLISTENSTOP, eventData);
}

void NetworkServer::DispatchConnected(const SharedPtr<NetworkConnection>& connection)
{
    SharedPtr<NetworkServer> self(this);
    auto workQueue = context_->GetSubsystem<WorkQueue>();
    workQueue->RunTaskOnMainThread([self = std::move(self), connection]() { self->HandleConnected(connection); });
}

void NetworkServer::DispatchDisconnected(const SharedPtr<NetworkConnection>& connection)
{
    SharedPtr<NetworkServer> self(this);
    auto workQueue = context_->GetSubsystem<WorkQueue>();
    workQueue->RunTaskOnMainThread([self = std::move(self), connection]() { self->HandleDisconnected(connection); });
}

void NetworkServer::DispatchListenStart()
{
    SharedPtr<NetworkServer> self(this);
    auto workQueue = context_->GetSubsystem<WorkQueue>();
    workQueue->RunTaskOnMainThread([self = std::move(self)]() { self->HandleListenStart(); });
}

void NetworkServer::DispatchListenStop()
{
    SharedPtr<NetworkServer> self(this);
    auto workQueue = context_->GetSubsystem<WorkQueue>();
    workQueue->RunTaskOnMainThread([self = std::move(self)]() { self->HandleListenStop(); });
}

const ea::vector<SharedPtr<NetworkConnection>>& NetworkServer::GetConnections() const
{
    return connections_;
}

SharedPtr<NetworkConnection> NetworkServer::CreateConnection()
{
    URHO3D_ASSERT(connectionFactory_);
    SharedPtr<NetworkConnection> connection = connectionFactory_();

    auto workQueue = context_->GetSubsystem<WorkQueue>();
    SharedPtr<NetworkServer> self(this);
    workQueue->RunTaskOnMainThread([self = std::move(self), connection]() { self->AddConnection(connection); });

    return connection;
}

void NetworkServer::AddConnection(const SharedPtr<NetworkConnection>& connection)
{
    connections_.push_back(connection);
}

void NetworkServer::RemoveConnection(const SharedPtr<NetworkConnection>& connection)
{
    auto pred = [connection](const SharedPtr<NetworkConnection>& elem) { return elem == connection; };
    ea::erase_if(connections_, pred);
}

} // namespace Urho3D
