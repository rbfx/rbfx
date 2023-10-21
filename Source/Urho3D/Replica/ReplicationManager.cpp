//
// Copyright (c) 2017-2020 the rbfx project.
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

#include <Urho3D/Precompiled.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Exception.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Replica/NetworkObject.h>
#include <Urho3D/Replica/NetworkSettingsConsts.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

namespace Urho3D
{

NetworkObjectRegistry::NetworkObjectRegistry(Context* context)
    : ReferencedComponentRegistryBase(context, NetworkObject::GetTypeStatic())
{
}

void NetworkObjectRegistry::RegisterObject(Context* context)
{
    context->AddAbstractReflection<NetworkObjectRegistry>(Category_Network);
}

void NetworkObjectRegistry::OnComponentAdded(TrackedComponentBase* baseComponent)
{
    BaseClassName::OnComponentAdded(baseComponent);

    auto networkObject = static_cast<NetworkObject*>(baseComponent);

    const NetworkId networkId = networkObject->GetNetworkId();
    const auto [index, version] = DeconstructComponentReference(networkId);

    if (networkObjectsDirty_.size() <= index)
        networkObjectsDirty_.resize(index + 1);
    networkObjectsDirty_[index] = true;

    OnNetworkObjectAdded(this, networkObject);

    URHO3D_LOGINFO("NetworkObject {} is added", ToString(networkId));
}

void NetworkObjectRegistry::OnComponentRemoved(TrackedComponentBase* baseComponent)
{
    auto networkObject = static_cast<NetworkObject*>(baseComponent);

    const NetworkId networkId = networkObject->GetNetworkId();

    if (NetworkObject* parentObject = networkObject->GetParentNetworkObject())
    {
        if (parentObject->GetNetworkId() != NetworkId::None)
            QueueNetworkObjectUpdate(parentObject);
    }

    OnNetworkObjectRemoved(this, networkObject);

    URHO3D_LOGINFO("NetworkObject {} is removed", ToString(networkId));

    BaseClassName::OnComponentRemoved(baseComponent);
}

void NetworkObjectRegistry::QueueNetworkObjectUpdate(NetworkObject* networkObject)
{
    const NetworkId networkId = networkObject->GetNetworkId();
    if (GetNetworkObject(networkId) != networkObject)
    {
        URHO3D_LOGWARNING("Cannot queue update for unknown NetworkObject {}", ToString(networkId));
        return;
    }

    const auto index = DeconstructComponentReference(networkId).first;
    networkObjectsDirty_[index] = true;
}

void NetworkObjectRegistry::RemoveAllNetworkObjects()
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

void NetworkObjectRegistry::UpdateNetworkObjects()
{
    for (unsigned index = 0; index < networkObjectsDirty_.size(); ++index)
    {
        if (!networkObjectsDirty_[index])
            continue;

        if (NetworkObject* networkObject = GetNetworkObjectByIndex(index))
        {
            networkObject->UpdateObjectHierarchy();
            networkObject->GetNode()->GetWorldTransform();
        }
        networkObjectsDirty_[index] = false;
    }
}

void NetworkObjectRegistry::GetSortedNetworkObjects(ea::vector<NetworkObject*>& networkObjects) const
{
    networkObjects.clear();

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

ea::string ReplicationManager::GetDebugInfo() const
{
    if (client_ && client_->replica_)
        return client_->replica_->GetDebugInfo();
    else if (client_)
        return GetUninitializedClientDebugInfo();
    else if (server_)
        return server_->GetDebugInfo();
    else
        return "";
}

NetworkObject* NetworkObjectRegistry::GetNetworkObject(NetworkId networkId, bool checkVersion) const
{
    return static_cast<NetworkObject*>(GetTrackedComponentByReference(networkId, checkVersion));
}

NetworkObject* NetworkObjectRegistry::GetNetworkObjectByIndex(unsigned networkIndex) const
{
    return static_cast<NetworkObject*>(GetTrackedComponentByReferenceIndex(networkIndex));
}

ReplicationManager::ReplicationManager(Context* context)
    : NetworkObjectRegistry(context)
{
}

ReplicationManager::~ReplicationManager() = default;

void ReplicationManager::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ReplicationManager>(Category_Subsystem);
}

void ReplicationManager::OnSceneSet(Scene* scene)
{
    BaseClassName::OnSceneSet(scene);

    if (scene)
    {
        SubscribeToEvent(scene, E_SCENEUPDATE,
            [this](VariantMap& eventData)
        {
            using namespace SceneUpdate;
            const float timeStep = eventData[P_TIMESTEP].GetFloat();
            OnSceneUpdate(timeStep);
        });

        SubscribeToEvent(scene, E_SCENEPOSTUPDATE, &ReplicationManager::UpdateNetworkObjects);
    }
    else
    {
        UnsubscribeFromEvent(E_SCENEUPDATE);
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
    }
}

void ReplicationManager::OnComponentAdded(TrackedComponentBase* baseComponent)
{
    BaseClassName::OnComponentAdded(baseComponent);

    if (IsStandalone())
    {
        auto networkObject = static_cast<NetworkObject*>(baseComponent);
        networkObject->SetNetworkMode(NetworkObjectMode::Standalone);
        networkObject->InitializeStandalone();
    }
}

void ReplicationManager::OnSceneUpdate(float timeStep)
{
    switch (mode_)
    {
    case ReplicationManagerMode::Standalone:
    {
        URHO3D_ASSERT(!server_ && !client_);

        Scene* scene = GetScene();
        VariantMap& eventData = scene->GetEventDataMap();

        using namespace SceneNetworkUpdate;
        eventData[P_SCENE] = scene;
        eventData[P_TIMESTEP_REPLICA] = timeStep;
        eventData[P_TIMESTEP_INPUT] = timeStep;
        scene->SendEvent(E_SCENENETWORKUPDATE, eventData);

        break;
    }
    case ReplicationManagerMode::Server:
    {
        URHO3D_ASSERT(server_);

        server_->ProcessSceneUpdate();

        break;
    }

    case ReplicationManagerMode::Client:
    {
        URHO3D_ASSERT(client_);

        if (client_->replica_)
            client_->replica_->ProcessSceneUpdate();

        break;
    }

    default: break;
    }
}

void ReplicationManager::Stop()
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

void ReplicationManager::StartStandalone()
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

void ReplicationManager::StartServer()
{
    Stop();

    mode_ = ReplicationManagerMode::Server;

    server_ = MakeShared<ServerReplicator>(GetScene());

    URHO3D_LOGINFO("Started server for scene replication");
}

void ReplicationManager::StartClient(AbstractConnection* connectionToServer)
{
    Stop();

    mode_ = ReplicationManagerMode::Client;

    client_ = ClientData{WeakPtr<AbstractConnection>(connectionToServer)};
    RemoveAllNetworkObjects();

    URHO3D_LOGINFO("Started client for scene replication");
}

unsigned ReplicationManager::GetUpdateFrequency() const
{
    if (server_)
        return server_->GetUpdateFrequency();
    else if (client_ && client_->replica_)
        return client_->replica_->GetUpdateFrequency();
    else
        return NetworkSettings::UpdateFrequency.defaultValue_.GetUInt();
}

float ReplicationManager::GetTraceDurationInSeconds() const
{
    if (server_)
        return server_->GetSetting(NetworkSettings::ServerTracingDuration).GetFloat();
    else if (client_ && client_->replica_)
        return client_->replica_->GetSetting(NetworkSettings::ClientTracingDuration).GetFloat();
    else
        return 0.0f;
}

unsigned ReplicationManager::GetTraceDurationInFrames() const
{
    const unsigned updateFrequency = GetSetting(NetworkSettings::UpdateFrequency).GetUInt();
    const float duration = GetTraceDurationInSeconds();
    return ea::max(1, CeilToInt(duration * updateFrequency));
}

const Variant& ReplicationManager::GetSetting(const NetworkSetting& setting) const
{
    if (server_)
        return server_->GetSetting(setting);
    else if (client_ && client_->replica_)
        return client_->replica_->GetSetting(setting);
    else
        return Variant::EMPTY;
}

bool ReplicationManager::ProcessMessage(
    AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (client_)
    {
        // If replica is not initialized, collect initialization data
        if (!client_->replica_)
            return ProcessMessageOnUninitializedClient(connection, messageId, messageData);
        else if (client_->replica_)
            return client_->replica_->ProcessMessage(messageId, messageData);
    }

    if (server_)
        return server_->ProcessMessage(connection, messageId, messageData);

    return false;
}

void ReplicationManager::DropConnection(AbstractConnection* connection)
{
    if (server_)
        server_->RemoveConnection(connection);
    else if (client_ && client_->connection_ == connection)
        StartStandalone();
}

bool ReplicationManager::ProcessMessageOnUninitializedClient(
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
    else
    {
        return false;
    }

    // If ready, initialize
    if (connection->IsClockSynchronized() && client_->IsReadyToInitialize())
    {
        client_->replica_ =
            MakeShared<ClientReplica>(GetScene(), connection, *client_->initialClock_, *client_->serverSettings_);

        connection->SendSerializedMessage(
            MSG_SYNCHRONIZED, MsgSynchronized{*client_->ackMagic_}, PacketType::ReliableUnordered);
    }

    return true;
}

ea::string ReplicationManager::GetUninitializedClientDebugInfo() const
{
    ea::vector<ea::string> waitList;
    if (client_->connection_ && !client_->connection_->IsClockSynchronized())
        waitList.push_back("system clock");
    if (!client_->serverSettings_)
        waitList.push_back("settings");
    if (!client_->initialClock_ || waitList.empty())
        waitList.push_back("server scene time");

    return Format("Connecting... Waiting for {}...", ea::string::joined(waitList, ", "));
}

} // namespace Urho3D
