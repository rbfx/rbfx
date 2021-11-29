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

/// \file

#pragma once

#include "../Container/IndexAllocator.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/ClientNetworkManager.h"
#include "../Network/ProtocolMessages.h"
#include "../Network/ServerNetworkManager.h"

#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class Connection;
class Network;
class NetworkObject;

/// Part of NetworkManager used by both client and server, and referenced by components.
class URHO3D_API NetworkManagerBase : public Object
{
    URHO3D_OBJECT(NetworkManagerBase, Object);

public:
    static constexpr unsigned IndexBits = 24;
    static constexpr unsigned VersionBits = 8;
    static constexpr unsigned IndexMask = (1u << IndexBits) - 1;
    static constexpr unsigned VersionMask = (1u << VersionBits) - 1;
    static constexpr unsigned IndexOffset = 0;
    static constexpr unsigned VersionOffset = IndexOffset + IndexBits;
    static_assert(VersionOffset + VersionBits == 32, "Unexpected mask layout");

    explicit NetworkManagerBase(Scene* scene);

    /// Process components
    /// @{
    void AddComponent(NetworkObject* networkObject);
    void RemoveComponent(NetworkObject* networkObject);
    void QueueComponentUpdate(NetworkObject* networkObject);
    void RemoveAllComponents();

    void ClearRecentActions();
    const auto& GetRecentlyRemovedComponents() const { return recentlyRemovedComponents_; }

    void UpdateAndSortNetworkObjects(ea::vector<NetworkObject*>& networkObjects) const;
    /// @}

    bool IsReplicatedClient() const { return !!client_; }
    Scene* GetScene() const { return scene_; }
    const auto& GetUnorderedNetworkObjects() const { return networkObjects_; }
    ea::string ToString() const;
    NetworkObject* GetNetworkObject(NetworkId networkId) const;

    /// NetworkId utilities
    /// @{
    static NetworkId ComposeNetworkId(unsigned index, unsigned version);
    static ea::pair<unsigned, unsigned> DecomposeNetworkId(NetworkId networkId);
    static ea::string FormatNetworkId(NetworkId networkId);
    /// @}

private:
    unsigned AllocateNewIndex();
    void EnsureIndex(unsigned index);
    bool IsValidComponent(unsigned index, unsigned version, NetworkObject* networkObject) const;
    bool IsValidComponent(unsigned index, unsigned version) const;

    Scene* scene_{};

    unsigned numComponents_{};
    ea::vector<NetworkObject*> networkObjects_;
    ea::vector<unsigned> networkObjectVersions_;
    ea::vector<bool> networkObjectsDirty_;
    IndexAllocator<DummyMutex> indexAllocator_;

    ea::unordered_set<NetworkId> recentlyRemovedComponents_;
    ea::unordered_set<NetworkId> recentlyAddedComponents_;

protected:
    SharedPtr<ServerNetworkManager> server_;
    SharedPtr<ClientNetworkManager> client_;
};

/// Subsystem that keeps trace of all NetworkObject in the Scene.
/// Built-in in Scene instead of being independent component for quick access and easier management.
class URHO3D_API NetworkManager : public NetworkManagerBase
{
    URHO3D_OBJECT(NetworkManager, NetworkManagerBase);

public:
    using NetworkObjectById = ea::unordered_map<unsigned, NetworkObject*>;

    explicit NetworkManager(Scene* scene);
    ~NetworkManager() override;

    /// Switch network manager to server mode. It's not supposed to be called on NetworkManager in client mode.
    void MarkAsServer();
    /// Switch network manager to client mode. It's not supposed to be called on NetworkManager in server mode.
    void MarkAsClient(AbstractConnection* connectionToServer);
    /// Return expected client or server network manager.
    ServerNetworkManager& AsServer();
    ClientNetworkManager& AsClient();

    /// Process network message either as client or as server.
    void ProcessMessage(AbstractConnection* connection, NetworkMessageId messageId, MemoryBuffer& messageData);
};

}
