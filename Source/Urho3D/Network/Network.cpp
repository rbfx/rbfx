// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Network/Network.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/Core/Profiler.h"
#include "Urho3D/Engine/EngineEvents.h"
#include "Urho3D/Network/NetworkConnection.h"
#include "Urho3D/Network/NetworkEvents.h"
#include "Urho3D/Network/NetworkServer.h"
#include "Urho3D/Network/DataChannel/DataChannelConnection.h"
#include "Urho3D/Network/DataChannel/DataChannelServer.h"
#include "Urho3D/Replica/BehaviorNetworkObject.h"
#include "Urho3D/Replica/FilteredByDistance.h"
#include "Urho3D/Replica/FilteredByOwner.h"
#include "Urho3D/Replica/NetworkObject.h"
#include "Urho3D/Replica/PredictedKinematicController.h"
#include "Urho3D/Replica/ReplicatedAnimation.h"
#include "Urho3D/Replica/ReplicatedParent.h"
#include "Urho3D/Replica/ReplicatedTransform.h"
#include "Urho3D/Replica/ReplicationManager.h"
#include "Urho3D/Replica/StaticNetworkObject.h"
#include "Urho3D/Replica/TrackedAnimatedModel.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

Network::Network(Context* context)
    : Object(context)
{
    // Register Network library object factories
    RegisterNetworkLibrary(context_);

    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(Network, HandleBeginFrame));
    SubscribeToEvent(E_RENDERUPDATE, URHO3D_HANDLER(Network, HandleRenderUpdate));

    SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(Network, HandleClientDisconnected));
    SubscribeToEvent(E_SERVERCLIENTDISCONNECTED, URHO3D_HANDLER(Network, HandleServerClientDisconnected));
}

Network::~Network()
{
}

void Network::SetUpdateFps(unsigned fps)
{
    updateFps_ = Max(fps, 1);
    updateInterval_ = 1.0f / (float)updateFps_;
    updateAcc_ = 0.0f;
}

SharedPtr<NetworkConnection> Network::CreateConnection(StringHash type)
{
    if (type.IsEmpty())
        type = DataChannelConnection::TypeId;
    return DynamicCast<NetworkConnection>(context_->CreateObject(type));
}

SharedPtr<NetworkServer> Network::CreateServer(StringHash type)
{
    if (type.IsEmpty())
        type = DataChannelServer::TypeId;
    return DynamicCast<NetworkServer>(context_->CreateObject(type));
}

void Network::Update(float timeStep)
{
    URHO3D_PROFILE("UpdateNetwork");

    // Check if periodic update should happen now
    updateAcc_ += timeStep;
    updateNow_ = updateAcc_ >= updateInterval_;
    if (updateNow_)
        updateAcc_ = fmodf(updateAcc_, updateInterval_);

    {
        using namespace NetworkInputProcessed;
        VariantMap& eventData = GetEventDataMap();
        eventData[P_TIMESTEP] = timeStep;
        SendEvent(E_NETWORKINPUTPROCESSED, eventData);
    }
}

void Network::PostUpdate(float timeStep)
{
    URHO3D_PROFILE("PostUpdateNetwork");

    // Update periodically on the server
    if (updateNow_)
        SendNetworkUpdateEvent(E_NETWORKUPDATE, true);

    // Always update on the client
    SendNetworkUpdateEvent(E_NETWORKUPDATE, false);
}

void Network::HandleBeginFrame(VariantMap& eventData)
{
    using namespace BeginFrame;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void Network::HandleRenderUpdate(VariantMap& eventData)
{
    using namespace RenderUpdate;

    PostUpdate(eventData[P_TIMESTEP].GetFloat());
}

void Network::OnConnectionDisconnecting(NetworkConnection* connection)
{
    disconnectingConnections_.insert(SharedPtr<NetworkConnection>(connection));
}

void Network::OnServerStopping(NetworkServer* server)
{
    stoppingServers_.insert(SharedPtr<NetworkServer>(server));
}

void Network::HandleClientDisconnected(VariantMap& eventData)
{
    using namespace ClientDisconnected;
    auto* connection = static_cast<NetworkConnection*>(eventData[P_CONNECTION].GetVoidPtr());
    disconnectingConnections_.erase(SharedPtr<NetworkConnection>(connection));
}

void Network::HandleServerClientDisconnected(VariantMap& eventData)
{
    using namespace ServerClientDisconnected;
    auto* connection = static_cast<NetworkConnection*>(eventData[P_CONNECTION].GetVoidPtr());
    auto* server = static_cast<NetworkServer*>(eventData[P_SERVER].GetVoidPtr());
    disconnectingConnections_.erase(SharedPtr<NetworkConnection>(connection));

    if (server->GetConnections().size() <= 1)
        stoppingServers_.erase(SharedPtr<NetworkServer>(server));
}

void Network::SendNetworkUpdateEvent(StringHash eventType, bool isServer)
{
    using namespace NetworkUpdate;
    auto& eventData = GetEventDataMap();
    eventData[P_ISSERVER] = isServer;
    SendEvent(eventType, eventData);
}

void RegisterNetworkLibrary(Context* context)
{
    NetworkObjectRegistry::RegisterObject(context);
    ReplicationManager::RegisterObject(context);

    NetworkObject::RegisterObject(context);
    StaticNetworkObject::RegisterObject(context);
    BehaviorNetworkObject::RegisterObject(context);

    NetworkBehavior::RegisterObject(context);
    ReplicatedAnimation::RegisterObject(context);
    ReplicatedParent::RegisterObject(context);
    ReplicatedTransform::RegisterObject(context);
    TrackedAnimatedModel::RegisterObject(context);
    FilteredByDistance::RegisterObject(context);
    FilteredByOwner::RegisterObject(context);
#ifdef URHO3D_PHYSICS
    PredictedKinematicController::RegisterObject(context);
#endif

    DataChannelConnection::RegisterObject(context);
    DataChannelServer::RegisterObject(context);
}

} // namespace Urho3D
