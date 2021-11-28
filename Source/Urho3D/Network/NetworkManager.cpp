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
#include "../Network/NetworkComponent.h"
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
        if (!networkComponents_[index])
            return index;
    }

    URHO3D_LOGERROR("Failed to allocate index");
    assert(0);
    return 0;
}

void NetworkManagerBase::EnsureIndex(unsigned index)
{
    assert(networkComponents_.size() == networkComponentVersions_.size());
    if (index >= networkComponents_.size())
    {
        networkComponents_.resize(index + 1);
        networkComponentVersions_.resize(index + 1);
    }
}

bool NetworkManagerBase::IsValidComponent(unsigned index, unsigned version, NetworkComponent* networkComponent) const
{
    return index < networkComponents_.size()
        && networkComponentVersions_[index] == version
        && networkComponents_[index] == networkComponent;
}

bool NetworkManagerBase::IsValidComponent(unsigned index, unsigned version) const
{
    return index < networkComponents_.size()
        && networkComponentVersions_[index] == version
        && networkComponents_[index];
}

void NetworkManagerBase::AddComponent(NetworkComponent* networkComponent)
{
    const bool needNewIndex = networkComponent->GetNetworkId() == InvalidNetworkId;
    if (numComponents_ >= IndexMask && needNewIndex)
    {
        URHO3D_LOGERROR("Failed to register NetworkComponent due to index overflow");
        assert(0);
        return;
    }

    // Assign network ID if missing
    if (needNewIndex)
    {
        const unsigned index = AllocateNewIndex();
        const unsigned version = networkComponentVersions_[index];
        networkComponent->SetNetworkId(ComposeNetworkId(index, version));
    }
    else
    {
        const unsigned index = networkComponent->GetNetworkIndex();
        EnsureIndex(index);
    }

    // Remove old component on collision
    const NetworkId networkId = networkComponent->GetNetworkId();
    const auto [index, version] = DecomposeNetworkId(networkId);
    NetworkComponent* oldNetworkComponent = networkComponents_[index];
    if (oldNetworkComponent)
    {
        URHO3D_LOGWARNING("NetworkComponent {} is overriden by NetworkComponent {}",
            FormatNetworkId(oldNetworkComponent->GetNetworkId()),
            FormatNetworkId(networkId));
        RemoveComponent(oldNetworkComponent);
    }

    // Add new component
    ++numComponents_;
    recentlyAddedComponents_.insert(networkId);
    networkComponents_[index] = networkComponent;
    URHO3D_LOGINFO("NetworkComponent {} is added", FormatNetworkId(networkId));
}

void NetworkManagerBase::RemoveComponent(NetworkComponent* networkComponent)
{
    const NetworkId networkId = networkComponent->GetNetworkId();
    if (networkId == InvalidNetworkId)
    {
        URHO3D_LOGERROR("Unexpected call to RemoveComponentAsServer");
        assert(0);
        return;
    }

    if (GetNetworkComponent(networkId) != networkComponent)
    {
        URHO3D_LOGWARNING("Cannot remove unknown NetworkComponent {}", FormatNetworkId(networkId));
        return;
    }

    --numComponents_;
    if (recentlyAddedComponents_.erase(networkId) == 0)
        recentlyRemovedComponents_.insert(networkId);

    const auto [index, version] = DecomposeNetworkId(networkId);
    networkComponents_[index] = nullptr;
    networkComponentVersions_[index] = (networkComponentVersions_[index] + 1) & VersionMask;
    indexAllocator_.Release(index);

    URHO3D_LOGINFO("NetworkComponent {} is removed", FormatNetworkId(networkId));
}

void NetworkManagerBase::RemoveAllComponents()
{
    ea::vector<WeakPtr<Node>> nodesToRemove;
    for (NetworkComponent* networkComponent : networkComponents_)
    {
        if (networkComponent)
            nodesToRemove.emplace_back(networkComponent->GetNode());
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
    networkComponents_.clear();
    networkComponentVersions_.clear();
    indexAllocator_.Clear();
    ClearRecentActions();

    URHO3D_LOGINFO("{} nodes removed on NetworkComponent cleanup", numRemovedNodes);
}

void NetworkManagerBase::ClearRecentActions()
{
    recentlyAddedComponents_.clear();
    recentlyRemovedComponents_.clear();
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

NetworkComponent* NetworkManagerBase::GetNetworkComponent(NetworkId networkId) const
{
    const auto [index, version] = DecomposeNetworkId(networkId);
    const bool isValidId = index < networkComponents_.size() && networkComponentVersions_[index] == version;
    return isValidId ? networkComponents_[index] : nullptr;
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
