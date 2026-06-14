// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Network/Network.h"
#include "Urho3D/Network/Transport/NetworkServer.h"

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/Core/Assert.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Thread.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Network/NetworkEvents.h"

#include <EASTL/algorithm.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

NetworkServer::NetworkServer(Context* context)
    : Object(context)
    , workQueue_(context->GetSubsystem<WorkQueue>())
{
    URHO3D_ASSERT(workQueue_);
}

void NetworkServer::NotifyStopping()
{
    if (auto* network = GetSubsystem<Network>())
        network->OnServerStopping(this);
}

void NetworkServer::OnConnected(NetworkConnection* connection)
{
    onConnected_(this, connection);

    using namespace ServerClientConnected;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    eventData[P_CONNECTION] = connection;
    eventData[P_ADDRESS] = connection->GetAddress();
    eventData[P_PORT] = connection->GetPort();
    SendEvent(E_SERVERCLIENTCONNECTED, eventData);
}

void NetworkServer::OnDisconnected(NetworkConnection* connection)
{
    onDisconnected_(this, connection);

    using namespace ServerClientDisconnected;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    eventData[P_CONNECTION] = connection;
    eventData[P_ADDRESS] = connection->GetAddress();
    eventData[P_PORT] = connection->GetPort();
    SendEvent(E_SERVERCLIENTDISCONNECTED, eventData);
}

void NetworkServer::OnListenStart()
{
    onListenStart_(this);

    using namespace ServerListenStart;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    SendEvent(E_SERVERLISTENSTART, eventData);
}

void NetworkServer::OnListenStop()
{
    onListenStop_(this);

    using namespace ServerListenStop;
    auto& eventData = GetEventDataMap();
    eventData[P_SERVER] = this;
    SendEvent(E_SERVERLISTENSTOP, eventData);
}

void NetworkServer::DoOnConnected(NetworkConnection* connection)
{
    SharedPtr<NetworkServer> self(this);
    SharedPtr<NetworkConnection> ref(connection);
    TaskFunction&& cb = [self = std::move(self), connection = std::move(ref)](unsigned, WorkQueue*) { self->OnConnected(connection); };
    workQueue_->RunTaskOnMainThread(ea::move(cb));
}

void NetworkServer::DoOnDisconnected(NetworkConnection* connection)
{
    SharedPtr<NetworkServer> self(this);
    SharedPtr<NetworkConnection> ref(connection);
    TaskFunction&& cb = [self = std::move(self), connection = std::move(ref)](unsigned, WorkQueue*) {
        self->OnDisconnected(connection);
        self->RemoveConnection(connection);
    };
    workQueue_->RunTaskOnMainThread(ea::move(cb));
}

void NetworkServer::DoOnListenStart()
{
    SharedPtr<NetworkServer> self(this);
    TaskFunction&& cb = [self = std::move(self)](unsigned, WorkQueue*) { self->OnListenStart(); };
    workQueue_->RunTaskOnMainThread(ea::move(cb));
}

void NetworkServer::DoOnListenStop()
{
    SharedPtr<NetworkServer> self(this);
    TaskFunction&& cb = [self = std::move(self)](unsigned, WorkQueue*) { self->OnListenStop(); };
    workQueue_->RunTaskOnMainThread(ea::move(cb));
}

const ea::vector<SharedPtr<NetworkConnection>>& NetworkServer::GetConnections() const
{
    return connections_;
}

SharedPtr<NetworkConnection> NetworkServer::CreateConnection()
{
    URHO3D_ASSERT(connectionFactory_);
    SharedPtr<NetworkConnection> connection = connectionFactory_();
    AddConnection(connection);
    return connection;
}

void NetworkServer::AddConnection(NetworkConnection* connection)
{
    SharedPtr<NetworkServer> self(this);
    SharedPtr<NetworkConnection> ref(connection);
    TaskFunction&& cb = [self = std::move(self), connection = std::move(ref)](unsigned, WorkQueue*) { self->connections_.push_back(connection); };
    workQueue_->RunTaskOnMainThread(ea::move(cb));
}

void NetworkServer::RemoveConnection(NetworkConnection* connection)
{
    SharedPtr<NetworkServer> self(this);
    SharedPtr<NetworkConnection> ref(connection);
    TaskFunction&& cb = [self = std::move(self), connection = std::move(ref)](unsigned, WorkQueue*) {
        auto&& pred = [connection](const SharedPtr<NetworkConnection>& conn) { return conn.Get() == connection; };
        ea::erase_if(self->connections_, pred);
    };
    workQueue_->RunTaskOnMainThread(ea::move(cb));
}

} // namespace Urho3D
