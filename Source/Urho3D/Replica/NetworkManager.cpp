//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Exception.h"
#include "../IO/Log.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Replica/NetworkObject.h"
#include "../Replica/NetworkManager.h"
#include "../Replica/NetworkSettingsConsts.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

NetworkManagerBase::NetworkManagerBase(Context* context)
    : BaseStableComponentRegistry(context, NetworkObject::GetTypeStatic())
{
}

void NetworkManagerBase::OnSceneSet(Scene* scene)
{
    scene_ = scene;
    BaseClassName::OnSceneSet(scene);
}

void NetworkManagerBase::OnComponentAdded(BaseTrackedComponent* baseComponent)
{
    BaseClassName::OnComponentAdded(baseComponent);

    auto networkObject = static_cast<NetworkObject*>(baseComponent);

    const NetworkId networkId = networkObject->GetNetworkId();
    const auto [index, version] = DeconstructStableComponentId(networkId);

    if (networkObjectsDirty_.size() <= index)
        networkObjectsDirty_.resize(index + 1);
    networkObjectsDirty_[index] = true;

    OnNetworkObjectAdded(this, networkObject);

    URHO3D_LOGINFO("NetworkObject {} is added", ToString(networkId));
}

void NetworkManagerBase::OnComponentRemoved(BaseTrackedComponent* baseComponent)
{
    auto networkObject = static_cast<NetworkObject*>(baseComponent);

    const NetworkId networkId = networkObject->GetNetworkId();

    OnNetworkObjectRemoved(this, networkObject);

    URHO3D_LOGINFO("NetworkObject {} is removed", ToString(networkId));

    BaseClassName::OnComponentRemoved(baseComponent);
}

void NetworkManagerBase::QueueComponentUpdate(NetworkObject* networkObject)
{
    const NetworkId networkId = networkObject->GetNetworkId();
    if (GetNetworkObject(networkId) != networkObject)
    {
        URHO3D_LOGWARNING("Cannot queue update for unknown NetworkObject {}", ToString(networkId));
        return;
    }

    const auto index = DeconstructStableComponentId(networkId).first;
    networkObjectsDirty_[index] = true;
}

void NetworkManagerBase::RemoveAllComponents()
{
    ea::vector<WeakPtr<Node>> nodesToRemove;
    for (NetworkObject* networkObject : GetNetworkObjects())
        nodesToRemove.emplace_back(networkObject->GetNode());

    for (Node* node : nodesToRemove)
    {
        if (node)
            node->Remove();
    }

    networkObjectsDirty_.clear();

    URHO3D_LOGINFO("{} instances of NetworkObject removed", nodesToRemove.size());
}

void NetworkManagerBase::UpdateAndSortNetworkObjects(ea::vector<NetworkObject*>& networkObjects) const
{
    networkObjects.clear();

    // Update hierarchy
    for (unsigned index = 0; index < networkObjectsDirty_.size(); ++index)
    {
        if (!networkObjectsDirty_[index])
            continue;

        if (NetworkObject* networkObject = GetNetworkObjectByIndex(index))
        {
            networkObject->UpdateObjectHierarchy();
            networkObject->GetNode()->GetWorldTransform();
        }
    }

    // Enumerate roots
    for (Component* component : GetTrackedComponents())
    {
        if (auto networkObject = static_cast<NetworkObject*>(component))
        {
            if (!networkObject->GetParentNetworkObject())
                networkObjects.push_back(networkObject);
        }
    }

    // Enumerate children: array may grow on iteration!
    for (unsigned i = 0; i < networkObjects.size(); ++i)
    {
        for (NetworkObject* childNetworkObject : networkObjects[i]->GetChildrenNetworkObjects())
            networkObjects.push_back(childNetworkObject);
    }
}

ea::string NetworkManager::GetDebugInfo() const
{
    if (client_ && client_->replica_)
        return client_->replica_->GetDebugInfo();
    else if (client_)
        return "Pending synchronization..."; // TODO(network): Revisit formatting
    else if (server_)
        return server_->GetDebugInfo();
    else
        return "";
}

NetworkObject* NetworkManagerBase::GetNetworkObject(NetworkId networkId, bool checkVersion) const
{
    return static_cast<NetworkObject*>(GetTrackedComponentByStableId(networkId, checkVersion));
}

NetworkObject* NetworkManagerBase::GetNetworkObjectByIndex(unsigned networkIndex) const
{
    return static_cast<NetworkObject*>(GetTrackedComponentByStableIndex(networkIndex));
}

NetworkManager::NetworkManager(Context* context)
    : NetworkManagerBase(context)
{
}

NetworkManager::~NetworkManager() = default;

void NetworkManager::RegisterObject(Context* context)
{
    context->RegisterFactory<NetworkManager>("");
}

void NetworkManager::OnComponentAdded(BaseTrackedComponent* baseComponent)
{
    BaseClassName::OnComponentAdded(baseComponent);

    if (IsStandalone())
    {
        auto networkObject = static_cast<NetworkObject*>(baseComponent);
        networkObject->SetNetworkMode(NetworkObjectMode::Standalone);
        networkObject->InitializeStandalone();
    }
}

void NetworkManager::Stop()
{
    if (client_)
    {
        URHO3D_LOGINFO("Stopped client for scene replication");
        client_ = ea::nullopt;
    }

    if (server_)
    {
        URHO3D_LOGINFO("Stopped server for scene replication");
        server_ = nullptr;
    }

    mode_ = ReplicationManagerMode::Standalone;
}

void NetworkManager::StartStandalone()
{
    Stop();

    mode_ = ReplicationManagerMode::Standalone;

    for (NetworkObject* networkObject : GetNetworkObjects())
    {
        networkObject->SetNetworkMode(NetworkObjectMode::Standalone);
        networkObject->InitializeStandalone();
    }

    URHO3D_LOGINFO("Started standalone scene replication");
}

void NetworkManager::StartServer()
{
    Stop();

    mode_ = ReplicationManagerMode::Server;

    server_ = MakeShared<ServerReplicator>(GetScene());

    URHO3D_LOGINFO("Started server for scene replication");
}

void NetworkManager::StartClient(AbstractConnection* connectionToServer)
{
    Stop();

    mode_ = ReplicationManagerMode::Client;

    client_ = ClientData{WeakPtr<AbstractConnection>(connectionToServer)};
    RemoveAllComponents();

    URHO3D_LOGINFO("Started client for scene replication");
}

unsigned NetworkManager::GetUpdateFrequency() const
{
    if (server_)
        return server_->GetUpdateFrequency();
    else if (client_ && client_->replica_)
        return client_->replica_->GetUpdateFrequency();
    else
        return NetworkSettings::UpdateFrequency.defaultValue_.GetUInt();
}

float NetworkManager::GetTraceDurationInSeconds() const
{
    if (server_)
        return server_->GetSetting(NetworkSettings::ServerTracingDuration).GetFloat();
    else if (client_ && client_->replica_)
        return client_->replica_->GetSetting(NetworkSettings::ClientTracingDuration).GetFloat();
    else
        return 0.0f;
}

unsigned NetworkManager::GetTraceDurationInFrames() const
{
    const unsigned updateFrequency = GetSetting(NetworkSettings::UpdateFrequency).GetUInt();
    const float duration = GetTraceDurationInSeconds();
    return ea::max(1, CeilToInt(duration * updateFrequency));
}

const Variant& NetworkManager::GetSetting(const NetworkSetting& setting) const
{
    if (server_)
        return server_->GetSetting(setting);
    else if (client_ && client_->replica_)
        return client_->replica_->GetSetting(setting);
    else
        return Variant::EMPTY;
}

void NetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (client_)
    {
        // If replica is not initialized, collect initialization data
        if (!client_->replica_)
            ProcessMessageOnUninitializedClient(connection, messageId, messageData);
        else if (client_->replica_)
            client_->replica_->ProcessMessage(messageId, messageData);
    }

    if (server_)
    {
        server_->ProcessMessage(connection, messageId, messageData);
    }
}

void NetworkManager::DropConnection(AbstractConnection* connection)
{
    if (server_)
        server_->RemoveConnection(connection);
    else if (client_ && client_->connection_ == connection)
        StartStandalone();
}

void NetworkManager::ProcessMessageOnUninitializedClient(
    AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    URHO3D_ASSERT(client_ && !client_->replica_);

    if (messageId == MSG_CONFIGURE)
    {
        const auto msg = ReadNetworkMessage<MsgConfigure>(messageData);
        connection->OnMessageReceived(messageId, msg);

        client_->ackMagic_ = msg.magic_;
        client_->serverSettings_ = msg.settings_;
    }
    else if (messageId == MSG_SCENE_CLOCK)
    {
        const auto msg = ReadNetworkMessage<MsgSceneClock>(messageData);
        connection->OnMessageReceived(messageId, msg);

        client_->initialClock_ = msg;
    }

    // If ready, initialize
    if (connection->IsClockSynchronized() && client_->IsReadyToInitialize())
    {
        client_->replica_ = MakeShared<ClientReplica>(
            GetScene(), connection, *client_->initialClock_, *client_->serverSettings_);

        connection->SendSerializedMessage(
            MSG_SYNCHRONIZED, MsgSynchronized{*client_->ackMagic_}, PT_RELIABLE_UNORDERED);
    }
}

}
