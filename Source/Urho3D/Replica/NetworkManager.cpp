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

    recentlyAddedComponents_.insert(networkId);

    URHO3D_LOGINFO("NetworkObject {} is added", ToString(networkId));
}

void NetworkManagerBase::OnComponentRemoved(BaseTrackedComponent* baseComponent)
{
    auto networkObject = static_cast<NetworkObject*>(baseComponent);

    const NetworkId networkId = networkObject->GetNetworkId();

    if (recentlyAddedComponents_.erase(networkId) == 0)
        recentlyRemovedComponents_.insert(networkId);

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
    for (Component* component : GetTrackedComponents())
    {
        if (auto networkObject = static_cast<NetworkObject*>(component))
            nodesToRemove.emplace_back(networkObject->GetNode());
    }
    unsigned numRemovedNodes = 0;
    for (Node* node : nodesToRemove)
    {
        if (node)
        {
            node->Remove();
            ++numRemovedNodes;
        }
    }

    networkObjectsDirty_.clear();
    ClearRecentActions();

    URHO3D_LOGINFO("{} nodes removed on NetworkObject cleanup", numRemovedNodes);
}

void NetworkManagerBase::ClearRecentActions()
{
    recentlyAddedComponents_.clear();
    recentlyRemovedComponents_.clear();
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
    if (clientProcessor_ && clientProcessor_->replica_)
        return clientProcessor_->replica_->GetDebugInfo();
    else if (clientProcessor_)
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
{}

NetworkManager::~NetworkManager() = default;

void NetworkManager::MarkAsServer()
{
    if (clientProcessor_)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from client to server mode");
        clientProcessor_ = ea::nullopt;
        assert(0);
    }

    if (!server_)
    {
        server_ = MakeShared<ServerNetworkManager>(this, GetScene());
    }
}

void NetworkManager::MarkAsClient(AbstractConnection* connectionToServer)
{
    if (server_)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from client to server mode");
        server_ = nullptr;
        assert(0);
    }

    if (clientProcessor_ && clientProcessor_->replica_ && clientProcessor_->replica_->GetConnection() != connectionToServer)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from one server to another without scene recreation");
        clientProcessor_ = ea::nullopt;
        assert(0);
    }

    if (!clientProcessor_)
    {
        clientProcessor_ = ClientProcessor{};
        RemoveAllComponents();
    }
}

ServerNetworkManager& NetworkManager::AsServer()
{
    assert(server_);
    return *server_;
}

ClientNetworkManager& NetworkManager::AsClient()
{
    assert(clientProcessor_ && clientProcessor_->replica_);
    return *clientProcessor_->replica_;
}

void NetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (clientProcessor_)
    {
        // If replica is not initialized, collect initialization data
        if (!clientProcessor_->replica_)
            ProcessMessageOnUninitializedClient(connection, messageId, messageData);

        if (clientProcessor_->replica_)
            clientProcessor_->replica_->ProcessMessage(messageId, messageData);
    }

    if (server_)
    {
        server_->ProcessMessage(connection, messageId, messageData);
    }
}

void NetworkManager::ProcessMessageOnUninitializedClient(
    AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    URHO3D_ASSERT(clientProcessor_ && !clientProcessor_->replica_);

    if (messageId == MSG_CONFIGURE)
    {
        const auto msg = ReadNetworkMessage<MsgConfigure>(messageData);
        connection->OnMessageReceived(messageId, msg);

        clientProcessor_->ackMagic_ = msg.magic_;
        clientProcessor_->serverSettings_ = msg.settings_;
    }
    else if (messageId == MSG_SCENE_CLOCK)
    {
        const auto msg = ReadNetworkMessage<MsgSceneClock>(messageData);
        connection->OnMessageReceived(messageId, msg);

        clientProcessor_->initialClock_ = msg;
    }

    // If ready, initialize
    if (connection->IsClockSynchronized() && clientProcessor_->IsReadyToInitialize())
    {
        clientProcessor_->replica_ = MakeShared<ClientNetworkManager>(
            GetScene(), connection, *clientProcessor_->initialClock_, *clientProcessor_->serverSettings_);

        connection->SendSerializedMessage(
            MSG_SYNCHRONIZED, MsgSynchronized{*clientProcessor_->ackMagic_}, NetworkMessageFlag::Reliable);
    }
}

}
