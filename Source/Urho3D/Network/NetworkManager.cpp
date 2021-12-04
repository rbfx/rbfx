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
#include "../Network/NetworkObject.h"
#include "../Network/NetworkManager.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

NetworkManagerBase::NetworkManagerBase(Scene* scene)
    : Object(scene->GetContext())
    , scene_(scene)
{
}

unsigned NetworkManagerBase::AllocateNewIndex()
{
    // May need more than one attept if some indices are taken bypassing IndexAllocator
    for (unsigned i = 0; i <= IndexMask; ++i)
    {
        const unsigned index = indexAllocator_.Allocate();
        EnsureIndex(index);
        if (!networkObjects_[index])
            return index;
    }

    URHO3D_LOGERROR("Failed to allocate index");
    assert(0);
    return 0;
}

void NetworkManagerBase::EnsureIndex(unsigned index)
{
    assert(networkObjects_.size() == networkObjectVersions_.size());
    assert(networkObjects_.size() == networkObjectsDirty_.size());
    if (index >= networkObjects_.size())
    {
        networkObjects_.resize(index + 1);
        networkObjectVersions_.resize(index + 1);
        networkObjectsDirty_.resize(index + 1);
    }
}

bool NetworkManagerBase::IsValidComponent(unsigned index, unsigned version, NetworkObject* networkObject) const
{
    return index < networkObjects_.size()
        && networkObjectVersions_[index] == version
        && networkObjects_[index] == networkObject;
}

bool NetworkManagerBase::IsValidComponent(unsigned index, unsigned version) const
{
    return index < networkObjects_.size()
        && networkObjectVersions_[index] == version
        && networkObjects_[index];
}

void NetworkManagerBase::AddComponent(NetworkObject* networkObject)
{
    const bool needNewIndex = networkObject->GetNetworkId() == InvalidNetworkId;
    if (numComponents_ >= IndexMask && needNewIndex)
    {
        URHO3D_LOGERROR("Failed to register NetworkObject due to index overflow");
        assert(0);
        return;
    }

    // Assign network ID if missing
    if (needNewIndex)
    {
        const unsigned index = AllocateNewIndex();
        const unsigned version = networkObjectVersions_[index];
        networkObject->SetNetworkId(ComposeNetworkId(index, version));
    }
    else
    {
        const unsigned index = DecomposeNetworkId(networkObject->GetNetworkId()).first;
        EnsureIndex(index);
    }

    // Remove old component on collision
    const NetworkId networkId = networkObject->GetNetworkId();
    const auto [index, version] = DecomposeNetworkId(networkId);
    NetworkObject* oldNetworkObject = networkObjects_[index];
    if (oldNetworkObject)
    {
        URHO3D_LOGWARNING("NetworkObject {} is overriden by NetworkObject {}",
            FormatNetworkId(oldNetworkObject->GetNetworkId()),
            FormatNetworkId(networkId));

        RemoveComponent(oldNetworkObject);
    }

    // Add new component
    ++numComponents_;
    recentlyAddedComponents_.insert(networkId);
    networkObjects_[index] = networkObject;

    if (!IsReplicatedClient())
        networkObject->InitializeReliableDelta();

    URHO3D_LOGINFO("NetworkObject {} is added", FormatNetworkId(networkId));
}

void NetworkManagerBase::RemoveComponent(NetworkObject* networkObject)
{
    const NetworkId networkId = networkObject->GetNetworkId();
    if (networkId == InvalidNetworkId)
    {
        URHO3D_LOGERROR("Unexpected call to RemoveComponentAsServer");
        assert(0);
        return;
    }

    if (GetNetworkObject(networkId) != networkObject)
    {
        URHO3D_LOGWARNING("Cannot remove unknown NetworkObject {}", FormatNetworkId(networkId));
        return;
    }

    --numComponents_;
    if (recentlyAddedComponents_.erase(networkId) == 0)
        recentlyRemovedComponents_.insert(networkId);

    const auto [index, version] = DecomposeNetworkId(networkId);
    networkObjects_[index] = nullptr;
    networkObjectVersions_[index] = (networkObjectVersions_[index] + 1) & VersionMask;
    networkObjectsDirty_[index] = true;
    indexAllocator_.Release(index);

    URHO3D_LOGINFO("NetworkObject {} is removed", FormatNetworkId(networkId));
}

void NetworkManagerBase::QueueComponentUpdate(NetworkObject* networkObject)
{
    const NetworkId networkId = networkObject->GetNetworkId();
    if (GetNetworkObject(networkId) != networkObject)
    {
        URHO3D_LOGWARNING("Cannot queue update for unknown NetworkObject {}", FormatNetworkId(networkId));
        return;
    }

    const auto index = DecomposeNetworkId(networkId).first;
    networkObjectsDirty_[index] = true;
}

void NetworkManagerBase::RemoveAllComponents()
{
    ea::vector<WeakPtr<Node>> nodesToRemove;
    for (NetworkObject* networkObject : networkObjects_)
    {
        if (networkObject)
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

    numComponents_ = 0;
    networkObjects_.clear();
    networkObjectVersions_.clear();
    indexAllocator_.Clear();
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

        if (NetworkObject* networkObject = networkObjects_[index])
        {
            networkObject->UpdateParent();
            networkObject->GetNode()->GetWorldTransform();
        }
    }

    // Enumerate roots
    for (NetworkObject* networkObject : networkObjects_)
    {
        if (networkObject && !networkObject->GetParentNetworkObject())
            networkObjects.push_back(networkObject);
    }

    // Enumerate children: array may grow on iteration!
    for (unsigned i = 0; i < networkObjects.size(); ++i)
    {
        for (NetworkObject* childNetworkObject : networkObjects[i]->GetChildrenNetworkObjects())
            networkObjects.push_back(childNetworkObject);
    }
}

ea::string NetworkManagerBase::ToString() const
{
    if (server_)
        return server_->ToString();
    else if (client_)
        return client_->ToString();
    else
        return "";
}

NetworkObject* NetworkManagerBase::GetNetworkObject(NetworkId networkId) const
{
    const auto [index, version] = DecomposeNetworkId(networkId);
    const bool isValidId = index < networkObjects_.size() && networkObjectVersions_[index] == version;
    return isValidId ? networkObjects_[index] : nullptr;
}

NetworkObject* NetworkManagerBase::GetNetworkObjectByIndex(unsigned networkIndex) const
{
    return networkIndex < networkObjects_.size() ? networkObjects_[networkIndex] : nullptr;
}

NetworkId NetworkManagerBase::ComposeNetworkId(unsigned index, unsigned version)
{
    unsigned result = 0;
    result |= (index & IndexMask) << IndexOffset;
    result |= (version & VersionMask) << VersionOffset;
    return static_cast<NetworkId>(result);
}

ea::pair<unsigned, unsigned> NetworkManagerBase::DecomposeNetworkId(NetworkId networkId)
{
    const auto value = static_cast<unsigned>(networkId);
    return { (value >> IndexOffset) & IndexMask, (value >> VersionOffset) * VersionMask };
}

ea::string NetworkManagerBase::FormatNetworkId(NetworkId networkId)
{
    const auto [index, version] = DecomposeNetworkId(networkId);
    return networkId == InvalidNetworkId ? "Undefined" : Format("{}:{}", index, version);
}

NetworkManager::NetworkManager(Scene* scene)
    : NetworkManagerBase(scene)
{}

NetworkManager::~NetworkManager() = default;

void NetworkManager::MarkAsServer()
{
    if (client_)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from client to server mode");
        client_ = nullptr;
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

    if (client_ && client_->GetConnection() != connectionToServer)
    {
        URHO3D_LOGWARNING("Swiching NetworkManager from one server to another without scene recreation");
        client_ = nullptr;
        assert(0);
    }

    if (!client_)
    {
        client_ = MakeShared<ClientNetworkManager>(this, GetScene(), connectionToServer);
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
    assert(client_);
    return *client_;
}

void NetworkManager::ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData)
{
    if (!server_ && !client_)
    {
        URHO3D_LOGWARNING("Uninitialized NetworkManager cannot process incoming message");
        return;
    }

    if (server_)
        server_->ProcessMessage(connection, messageId, messageData);
    else
        client_->ProcessMessage(messageId, messageData);
}

}
