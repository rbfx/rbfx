// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Network/SceneLoader.h"

#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Math/RandomEngine.h"
#include "Urho3D/Network/Protocol.h"
#include "Urho3D/Network/ReplicatedPeer.h"
#include "Urho3D/Network/NetworkConnection.h"
#include "Urho3D/Network/NetworkDefs.h"
#include "Urho3D/Replica/ReplicationManager.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

#include <fmt/ranges.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

ReplicationManager* GetSceneReplicationManager(Scene* scene)
{
    return scene ? scene->GetComponent<ReplicationManager>() : nullptr;
}

} // namespace

ServerSceneLoader::ServerSceneLoader(
    NetworkConnection* connection, const ReplicatedPeerPtr& replicatedPeer)
    : Object(connection->GetContext())
    , connection_(connection)
    , replicatedPeer_(replicatedPeer)
{
    URHO3D_ASSERT(connection_);
    URHO3D_ASSERT(replicatedPeer_);

    connection_->OnConnected.SubscribeWithSender(this, &ServerSceneLoader::OnClientConnected);
    connection_->OnDisconnected.SubscribeWithSender(this, &ServerSceneLoader::OnClientDisconnected);
    connection_->OnMessageReceived.SubscribeWithSender(this, &ServerSceneLoader::OnMessageReceived);
}

void ServerSceneLoader::SetScene(Scene* scene, const StringVariantMap& params)
{
    scene_ = scene;
    params_ = params;
    pendingRequestMagic_ = ea::nullopt;

    if (connection_ && connection_->IsConnected())
        BeginSceneLoad();
}

void ServerSceneLoader::BeginSceneLoad()
{
    if (!scene_)
        return;

    pendingRequestMagic_ = RandomEngine::GetDefaultEngine().GetUInt();
    replicatedPeer_->SetReplicationManager(nullptr);

    VectorBuffer message;
    message.WriteUInt(*pendingRequestMagic_);
    message.WriteStringVariantMap(params_);
    connection_->SendMessage(MSG_LOAD_SCENE, message.GetBuffer(), PacketType::ReliableOrdered);

    URHO3D_LOGINFO("Server requested scene load for replicated peer #{} with parameters {}",
        replicatedPeer_->GetObjectID(), params_);
}

void ServerSceneLoader::OnClientConnected(NetworkConnection* connection)
{
    BeginSceneLoad();
}

void ServerSceneLoader::OnClientDisconnected(NetworkConnection* connection)
{
    pendingRequestMagic_ = ea::nullopt;
}

void ServerSceneLoader::OnMessageReceived(
    NetworkConnection* connection, NetworkMessageId messageId, ConstByteSpan message, bool& handled)
{
    if (!scene_)
        return;

    if (!handled && messageId == MSG_SCENE_LOAD_RESULT)
    {
        MemoryBuffer messageReader{message};
        const unsigned magic = messageReader.ReadUInt();
        if (magic == pendingRequestMagic_)
        {
            URHO3D_LOGINFO("Replicated peer #{} completed scene load on server side", replicatedPeer_->GetObjectID());
            pendingRequestMagic_ = ea::nullopt;
            replicatedPeer_->SetReplicationManager(GetSceneReplicationManager(scene_));
        }
        else
        {
            URHO3D_LOGWARNING("Replicated peer #{} reported unexpected scene load", replicatedPeer_->GetObjectID());
        }
        handled = true;
    }
}

ClientSceneLoader::ClientSceneLoader(
    NetworkConnection* connection, const ReplicatedPeerPtr& replicatedPeer)
    : Object(connection->GetContext())
    , connection_(connection)
    , replicatedPeer_(replicatedPeer)
{
    URHO3D_ASSERT(connection_);
    URHO3D_ASSERT(replicatedPeer_);

    connection_->OnMessageReceived.SubscribeWithSender(this, &ClientSceneLoader::OnMessageReceived);
}

void ClientSceneLoader::SetScene(Scene* scene)
{
    scene_ = scene;
}

void ClientSceneLoader::OnLoadRequested(const StringVariantMap& params, ea::function<void()> onCompleted)
{
    if (onLoadRequested_)
        onLoadRequested_(params, onCompleted);
    else
        onCompleted();
}

void ClientSceneLoader::OnMessageReceived(
    NetworkConnection* connection, NetworkMessageId messageId, ConstByteSpan message, bool& handled)
{
    if (!connection_ || !replicatedPeer_)
        return;

    if (!handled && messageId == MSG_LOAD_SCENE)
    {
        MemoryBuffer messageReader{message};
        const unsigned magic = messageReader.ReadUInt();
        const StringVariantMap params = messageReader.ReadStringVariantMap();

        URHO3D_LOGINFO("Replicated peer #{} begins scene load on client side with parameters {}",
            replicatedPeer_->GetObjectID(), params);
        replicatedPeer_->SetReplicationManager(nullptr);

        WeakPtr<ClientSceneLoader> weakSelf{this};
        OnLoadRequested(params, [weakSelf, magic]
        {
            if (const auto self = weakSelf.Lock())
                self->OnSceneLoaded(magic);
        });
    }
}

void ClientSceneLoader::OnSceneLoaded(unsigned magic)
{
    if (!connection_ || !replicatedPeer_)
        return;

    URHO3D_LOGINFO("Replicated peer #{} completed scene load on client side", replicatedPeer_->GetObjectID());
    replicatedPeer_->SetReplicationManager(GetSceneReplicationManager(scene_));

    VectorBuffer message;
    message.WriteUInt(magic);
    connection_->SendMessage(MSG_SCENE_LOAD_RESULT, message.GetBuffer(), PacketType::ReliableOrdered);
}

} // namespace Urho3D
