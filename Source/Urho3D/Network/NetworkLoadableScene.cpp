// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Network/NetworkLoadableScene.h"

#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Network/Protocol.h"
#include "Urho3D/Network/ReplicatedPeer.h"
#include "Urho3D/Network/Transport/NetworkConnection.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/BinaryFile.h"
#include "Urho3D/Resource/JSONFile.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Replica/ReplicationManager.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

ReplicationManager* GetSceneReplicationManager(Scene* scene)
{
    return scene ? scene->GetComponent<ReplicationManager>() : nullptr;
}

void AssignConnectionReplicationManager(AbstractConnection* replicatedPeer, Scene* scene)
{
    if (replicatedPeer)
        replicatedPeer->SetReplicationManager(GetSceneReplicationManager(scene));
}

}

NetworkLoadableScene::NetworkLoadableScene(Context* context)
    : Object(context)
{
}

NetworkLoadableScene::NetworkLoadableScene(
    NetworkConnection* connection, SharedPtr<AbstractConnection, RefCounted> replicatedPeer)
    : Object(connection->GetContext())
{
    URHO3D_ASSERT(connection);
    Attach(connection, ea::move(replicatedPeer));
}

void NetworkLoadableScene::OnPrepareLoadSceneRequest(const ea::string& sceneFileName, VectorBuffer& request)
{
    onPrepareLoadSceneRequest_(this, sceneFileName, request);
}

void NetworkLoadableScene::OnLoadSceneRequestReceived(const ea::string& sceneFileName, MemoryBuffer& request, bool& accept)
{
    onLoadSceneRequestReceived_(this, sceneFileName, request, accept);
}

void NetworkLoadableScene::OnLoadSceneStarted(const ea::string& sceneFileName)
{
    onLoadSceneStarted_(this, sceneFileName);
}

void NetworkLoadableScene::OnLoadSceneFinished(const ea::string& sceneFileName, bool& success, VectorBuffer& response)
{
    onLoadSceneFinished_(this, sceneFileName, success, response);
}

void NetworkLoadableScene::OnRemoteLoadSceneFinished(const ea::string& sceneFileName, bool success, MemoryBuffer& response)
{
    onRemoteLoadSceneFinished_(this, sceneFileName, success, response);
}

void NetworkLoadableScene::Attach(
    NetworkConnection* connection, SharedPtr<AbstractConnection, RefCounted> replicatedPeer)
{
    if (connection_.Get() == connection)
    {
        replicationConnection_ = ea::move(replicatedPeer);
        return;
    }

    if (connection_)
    {
        connection_->onMessage_.Unsubscribe(this);
        connection_->onConnected_.Unsubscribe(this);
        connection_->onDisconnected_.Unsubscribe(this);
    }

    connection_ = connection;
    replicationConnection_ = ea::move(replicatedPeer);

    if (!connection)
        return;

    connection->onMessage_.SubscribeWithSender(this, &NetworkLoadableScene::ProcessNetworkMessage);
    connection->onConnected_.SubscribeWithSender(this, &NetworkLoadableScene::HandleConnectionConnected);
    connection->onDisconnected_.SubscribeWithSender(this, &NetworkLoadableScene::HandleConnectionDisconnected);
}

void NetworkLoadableScene::SetScene(Scene* scene)
{
    SetScene(SharedPtr<Scene>(scene));
}

void NetworkLoadableScene::SetScene(const SharedPtr<Scene>& scene)
{
    if (loadState_ != LoadState::Idle)
    {
        HandleProtocolError("SetScene was called while previous scene-loading action is still in progress");
        return;
    }

    scene_ = scene;
    loadingSceneName_.clear();
    expectedSceneName_.clear();
    UnsubscribeFromEvent(E_ASYNCLOADFINISHED);

    if (!IsServerSideConnection())
        AssignConnectionReplicationManager(replicationConnection_, scene_);

    if (IsServerSideConnection())
        RequestRemoteLoadScene();
}

bool NetworkLoadableScene::IsServerSideConnection() const
{
    return connection_ && connection_->GetServer() != nullptr;
}

void NetworkLoadableScene::HandleProtocolError(ea::string_view reason)
{
    if (!connection_)
        return;

    URHO3D_LOGERROR("NetworkLoadableScene protocol error: {}", reason);
    connection_->Disconnect();
}

void NetworkLoadableScene::RequestRemoteLoadScene()
{
    URHO3D_LOGINFO("NLS::RequestRemoteLoadScene: connection_={} scene_={} IsConnected={} IsServerSide={} loadState_={}",
        (void*)connection_.Get(), (void*)scene_.Get(),
        connection_ ? connection_->IsConnected() : false,
        IsServerSideConnection(),
        (int)loadState_);
    if (!connection_ || !scene_ || !connection_->IsConnected() || !IsServerSideConnection())
        return;

    if (loadState_ != LoadState::Idle)
    {
        HandleProtocolError("cannot send MSG_LOAD_SCENE while state is not idle");
        return;
    }

    const ea::string& sceneFileName = scene_->GetFileName();
    if (sceneFileName.empty())
    {
        URHO3D_LOGWARNING("Failed to request remote scene loading: scene file name is empty");
        return;
    }

    VectorBuffer request;
    request.WriteString(sceneFileName);
    OnPrepareLoadSceneRequest(sceneFileName, request);

    expectedSceneName_ = sceneFileName;
    loadState_ = LoadState::AwaitingRemoteLoadResult;
    connection_->SendMessage(MSG_LOAD_SCENE, request, PacketType::ReliableOrdered);
}

void NetworkLoadableScene::ProcessNetworkMessage(
    NetworkConnection* connection, NetworkMessageId messageId, MemoryBuffer& message, bool& handled)
{
    if (handled || connection != connection_)
        return;

    switch (messageId)
    {
    case MSG_LOAD_SCENE:
        ProcessLoadSceneRequest(message);
        handled = true;
        break;

    case MSG_SCENE_LOAD_RESULT:
        ProcessLoadSceneResult(message);
        handled = true;
        break;

    default:
        break;
    }
}

void NetworkLoadableScene::ProcessLoadSceneRequest(MemoryBuffer& message)
{
    if (!connection_)
        return;

    if (IsServerSideConnection())
    {
        HandleProtocolError("server received unexpected MSG_LOAD_SCENE");
        return;
    }

    if (loadState_ != LoadState::Idle)
    {
        HandleProtocolError("client received MSG_LOAD_SCENE while another scene load is active");
        return;
    }

    const ea::string sceneFileName = message.ReadString();
    if (sceneFileName.empty())
    {
        HandleProtocolError("MSG_LOAD_SCENE payload is invalid: empty scene file name");
        return;
    }

    bool accept = true;
    OnLoadSceneRequestReceived(sceneFileName, message, accept);

    if (!accept)
    {
        SendLocalLoadResult(sceneFileName, false);
        return;
    }

    if (!StartLocalLoad(sceneFileName))
        FinishLocalLoad(false);
}

void NetworkLoadableScene::ProcessLoadSceneResult(MemoryBuffer& message)
{
    if (!connection_)
        return;

    if (!IsServerSideConnection())
    {
        HandleProtocolError("client received unexpected MSG_SCENE_LOAD_RESULT");
        return;
    }

    if (loadState_ != LoadState::AwaitingRemoteLoadResult)
    {
        HandleProtocolError("server received MSG_SCENE_LOAD_RESULT without pending MSG_LOAD_SCENE request");
        return;
    }

    const ea::string sceneFileName = message.ReadString();
    if (message.GetSize() - message.GetPosition() < sizeof(unsigned char))
    {
        HandleProtocolError("MSG_SCENE_LOAD_RESULT payload is too short: missing success flag");
        return;
    }
    const bool success = message.ReadBool();

    if (sceneFileName != expectedSceneName_)
    {
        HandleProtocolError("MSG_SCENE_LOAD_RESULT scene name does not match active request");
        return;
    }

    loadState_ = LoadState::Idle;
    expectedSceneName_.clear();

    if (success)
        AssignConnectionReplicationManager(replicationConnection_, scene_);

    OnRemoteLoadSceneFinished(sceneFileName, success, message);
}

bool NetworkLoadableScene::StartLocalLoad(const ea::string& sceneFileName)
{
    if (!scene_)
        scene_ = MakeShared<Scene>(context_);

    UnsubscribeFromEvent(E_ASYNCLOADFINISHED);
    scene_->StopAsyncLoading();
    loadState_ = LoadState::LoadingLocalScene;
    loadingSceneName_ = sceneFileName;
    OnLoadSceneStarted(sceneFileName);

    auto* cache = GetSubsystem<ResourceCache>();
    if (!cache)
    {
        URHO3D_LOGERROR("Failed to load scene '{}': ResourceCache subsystem is missing", sceneFileName);
        return false;
    }


    const ea::string extension = GetExtension(sceneFileName);
    bool success = false;

    if (extension == ".xml")
    {
        auto* resource = cache->GetResource<XMLFile>(sceneFileName, false);
        if (resource)
            success = scene_->LoadXML(resource->GetRoot());
    }
    else if (extension == ".json")
    {
        auto* resource = cache->GetResource<JSONFile>(sceneFileName, false);
        if (resource)
            success = scene_->LoadJSON(resource->GetRoot());
    }
    else
    {
        auto* resource = cache->GetResource<BinaryFile>(sceneFileName, false);
        if (resource)
        {
            Deserializer& source = resource->AsDeserializer();
            source.Seek(0);
            success = scene_->Load(source);
        }
    }

    if (!success)
    {
        URHO3D_LOGERROR("Failed to load scene '{}' from ResourceCache", sceneFileName);
        return false;
    }

    FinishLocalLoad(true);
    return true;
}

void NetworkLoadableScene::FinishLocalLoad(bool success)
{
    if (loadState_ != LoadState::LoadingLocalScene || loadingSceneName_.empty())
        return;

    UnsubscribeFromEvent(E_ASYNCLOADFINISHED);
    SendLocalLoadResult(loadingSceneName_, success);
    loadingSceneName_.clear();
    loadState_ = LoadState::Idle;
}

void NetworkLoadableScene::SendLocalLoadResult(const ea::string& sceneFileName, bool success)
{
    if (!connection_)
        return;

    if (IsServerSideConnection())
    {
        HandleProtocolError("server attempted to send MSG_SCENE_LOAD_RESULT");
        return;
    }

    VectorBuffer response;
    bool effectiveSuccess = success;
    OnLoadSceneFinished(sceneFileName, effectiveSuccess, response);

    if (effectiveSuccess)
        AssignConnectionReplicationManager(replicationConnection_, scene_);

    VectorBuffer message;
    message.WriteString(sceneFileName);
    message.WriteBool(effectiveSuccess);
    if (response.GetSize() != 0)
        message.Write(response.GetData(), response.GetSize());

    connection_->SendMessage(MSG_SCENE_LOAD_RESULT, message, PacketType::ReliableOrdered);
}

void NetworkLoadableScene::HandleConnectionConnected(NetworkConnection* connection)
{
    URHO3D_LOGINFO("NLS::HandleConnectionConnected: connection={} connection_={} match={} IsServerSide={}",
        (void*)connection, (void*)connection_.Get(),
        connection == connection_.Get(),
        IsServerSideConnection());
    if (connection != connection_)
        return;

    if (IsServerSideConnection())
        RequestRemoteLoadScene();
}

void NetworkLoadableScene::HandleConnectionDisconnected(NetworkConnection* connection)
{
    if (connection != connection_)
        return;

    UnsubscribeFromEvent(E_ASYNCLOADFINISHED);
    loadingSceneName_.clear();
    loadState_ = LoadState::Idle;
    expectedSceneName_.clear();
}

void NetworkLoadableScene::HandleAsyncLoadFinished(StringHash eventType, VariantMap& eventData)
{
    (void)eventType;
    (void)eventData;
    FinishLocalLoad(true);
}

} // namespace Urho3D
