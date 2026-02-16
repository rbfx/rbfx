// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "AdvancedNetworkingServer.h"

#include "AdvancedNetworkingConnection.h"
#include "AdvancedNetworkingPlayer.h"
#include "AdvancedNetworkingRaycast.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Math/RandomEngine.h>
#include <Urho3D/Network/URL.h>
#include <Urho3D/Replica/ReplicatedTransform.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Replica/TrackedAnimatedModel.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>

namespace
{
static constexpr float SERVER_CAMERA_OFFSET = 2.0f;
}

AdvancedNetworkingServer::AdvancedNetworkingServer(Scene* scene)
    : DataChannelServer(scene->GetContext())
    , scene_(scene)
{
    SetConnectionFactory([this]() { return MakeShared<AdvancedNetworkingConnection>(context_); });
    onConnected_.Subscribe(this, &AdvancedNetworkingServer::HandleServerConnected);
    onDisconnected_.Subscribe(this, &AdvancedNetworkingServer::HandleServerDisconnected);
    SubscribeToEvent(E_POSTUPDATE, &AdvancedNetworkingServer::ProcessRaycasts);
}

bool AdvancedNetworkingServer::Start(unsigned short port)
{
    if (!Listen(URL(Format("ws://0.0.0.0:{}", port))))
    {
        URHO3D_LOGERROR("AdvancedNetworking: failed to start server on port {}", port);
        return false;
    }

    URHO3D_LOGINFO("AdvancedNetworking: server started on port {}", port);
    return true;
}

void AdvancedNetworkingServer::Stop()
{
    if (!IsListening())
        return;

    for (const auto& [connection, node] : serverObjects_)
    {
        if (connection)
            connection->Disconnect();
        if (node)
            node->Remove();
    }
    serverObjects_.clear();
    serverRaycasts_.clear();

    DataChannelServer::Stop();
}

void AdvancedNetworkingServer::ProcessRaycasts()
{
    auto* replicationManager = scene_ ? scene_->GetComponent<ReplicationManager>() : nullptr;
    ServerReplicator* serverReplicator = replicationManager ? replicationManager->GetServerReplicator() : nullptr;
    if (!serverReplicator)
        return;

    const NetworkTime serverTime = serverReplicator->GetServerTime();

    ea::erase_if(serverRaycasts_,
        [&](const ServerRaycastInfo& raycastInfo)
    {
        if (!raycastInfo.clientConnection_)
            return true;

        if (serverTime - raycastInfo.inputTime_ < 0.0f)
            return false;

        ProcessSingleRaycast(raycastInfo);
        return true;
    });
}

void AdvancedNetworkingServer::ProcessSingleRaycast(const ServerRaycastInfo& raycastInfo)
{
    if (!raycastInfo.clientPeer_ || !scene_)
        return;

    auto* replicationManager = scene_->GetComponent<ReplicationManager>();
    auto* octree = scene_->GetComponent<Octree>();
    ServerReplicator* serverReplicator = replicationManager ? replicationManager->GetServerReplicator() : nullptr;
    if (!replicationManager || !octree || !serverReplicator)
        return;

    NetworkObject* clientObject = serverReplicator->GetNetworkObjectOwnedByConnection(raycastInfo.clientPeer_);
    if (!clientObject)
        return;

    auto replicatedTransform = clientObject->GetNode()->FindComponent<ReplicatedTransform>();
    if (!replicatedTransform)
        return;

    const Vector3 origin = scene_->ToRelativeWorldPosition(replicatedTransform->SampleTemporalPosition(raycastInfo.inputTime_).value_)
        + Vector3::UP * SERVER_CAMERA_OFFSET;
    const Vector3 target = scene_->ToRelativeWorldPosition(raycastInfo.target_);
    const Ray ray{origin, target - origin};

    RayOctreeQuery query(ray, RAY_TRIANGLE, M_INFINITY, DRAWABLE_GEOMETRY, IMPORTANT_VIEW_MASK);
    octree->RaycastSingle(query);

    for (NetworkObject* networkObject : replicationManager->GetNetworkObjects())
    {
        if (networkObject == clientObject)
            continue;

        auto behaviorNetworkObject = dynamic_cast<BehaviorNetworkObject*>(networkObject);
        if (!behaviorNetworkObject)
            continue;

        auto trackedModelAnimation = behaviorNetworkObject->GetNetworkBehavior<TrackedAnimatedModel>();
        if (!trackedModelAnimation)
            continue;

        trackedModelAnimation->ProcessTemporalRayQuery(raycastInfo.replicaTime_, query, query.result_);
    }

    ea::quick_sort(query.result_.begin(), query.result_.end(),
        [](const RayQueryResult& lhs, const RayQueryResult& rhs) { return lhs.distance_ < rhs.distance_; });

    VectorBuffer msg;
    const ea::optional<DoubleVector3> hitPosition = !query.result_.empty()
        ? ea::make_optional(scene_->ToAbsoluteWorldPosition(query.result_[0].position_))
        : ea::nullopt;
    WriteRaycastResult(msg, scene_->ToAbsoluteWorldPosition(origin), hitPosition);
    if (raycastInfo.clientConnection_)
        raycastInfo.clientConnection_->SendMessage(MSG_ADVANCEDNETWORKING_RAYHIT, msg, PacketType::UnreliableUnordered);
}

unsigned AdvancedNetworkingServer::GetClientCount() const
{
    return serverObjects_.size();
}

Node* AdvancedNetworkingServer::CreateControllableObject(SharedPtr<AbstractConnection, RefCounted> owner)
{
    if (!scene_)
        return nullptr;

    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto prefab = cache ? cache->GetResource<PrefabResource>("Prefabs/AdvancedNetworkingPlayer.prefab") : nullptr;
    if (!prefab)
    {
        URHO3D_LOGERROR("AdvancedNetworking: failed to load player prefab");
        return nullptr;
    }

    const Vector3 position{Random(20.0f) - 10.0f, 5.0f, Random(20.0f) - 10.0f};
    Node* playerNode = scene_->InstantiatePrefab(prefab->GetNodePrefab(), position, Quaternion::IDENTITY);
    if (!playerNode)
    {
        URHO3D_LOGERROR("AdvancedNetworking: failed to instantiate player prefab");
        return nullptr;
    }

    auto networkObject = playerNode->CreateComponent<BehaviorNetworkObject>();
    networkObject->SetClientPrefab(prefab);
    networkObject->SetOwner(owner);

    if (Light* playerLight = playerNode->FindComponent<Light>())
        playerLight->SetColor(Color::GREEN);

    return playerNode;
}

void AdvancedNetworkingServer::HandleServerConnected(NetworkConnection* connection)
{
    URHO3D_LOGINFO("AdvancedNetworking: server accepted connection from {}:{}", connection->GetAddress(), connection->GetPort());

    auto* advancedConnection = dynamic_cast<AdvancedNetworkingConnection*>(connection);
    auto* replicationManager = scene_ ? scene_->GetComponent<ReplicationManager>() : nullptr;
    if (!advancedConnection || !replicationManager)
    {
        URHO3D_LOGERROR("AdvancedNetworking: invalid server connection state");
        return;
    }

    advancedConnection->GetReplicatedPeer()->SetReplicationManager(replicationManager);
    advancedConnection->onMessage_.SubscribeWithSender(this, &AdvancedNetworkingServer::HandleNetworkMessage);

    Node* object = CreateControllableObject(advancedConnection->GetReplicatedPeer());
    serverObjects_[connection] = object;
    if (object)
        URHO3D_LOGINFO("AdvancedNetworking: spawned server player node '{}'", object->GetName());
    else
        URHO3D_LOGERROR("AdvancedNetworking: failed to spawn server player object");
}

void AdvancedNetworkingServer::HandleServerDisconnected(NetworkConnection* connection)
{
    URHO3D_LOGINFO("AdvancedNetworking: server disconnected client {}:{}", connection->GetAddress(), connection->GetPort());

    Node* object = serverObjects_[connection];
    if (object)
        object->Remove();

    serverObjects_.erase(connection);
}

void AdvancedNetworkingServer::HandleNetworkMessage(Urho3D::NetworkConnection* connection,
    NetworkMessageId messageId, MemoryBuffer& message, bool& handled)
{
    if (messageId != MSG_ADVANCEDNETWORKING_RAYCAST)
        return;

    auto* advancedConnection = dynamic_cast<AdvancedNetworkingConnection*>(connection);
    if (!advancedConnection)
        return;

    const ServerRaycastInfo raycastInfo = ReadRaycastRequest(connection, advancedConnection->GetReplicatedPeer(), message);
    serverRaycasts_.push_back(raycastInfo);
    handled = true;
}
