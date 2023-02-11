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

/// \file

#pragma once

#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/Core/Assert.h>
#include <Urho3D/Network/AbstractConnection.h>
#include <Urho3D/Replica/NetworkCallbacks.h>
#include <Urho3D/Replica/ReplicationManager.h>
#include <Urho3D/Scene/Component.h>

#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>

namespace Urho3D
{

class AbstractConnection;

enum class NetworkObjectMode
{
    /// Default state of NetworkObject.
    /// If scene is not replicated from/to, NetworkObject in such scene stays Standalone.
    /// If scene is replicated, NetworkObject is Standalone until it's processed by Network subsystem.
    Standalone,
    /// Object is on server and is replicated to clients.
    Server,
    /// Object is on client and is replicated from the server.
    ClientReplicated,
    /// Object is on client and is owned by this client. Client may send feedback from owned objects.
    ClientOwned
};

/// Base component of Network-replicated object.
///
/// Each NetworkObject has ID unique within the owner Scene.
/// Derive from NetworkObject to have custom network logic.
/// Don't create more than one NetworkObject per Node.
///
/// Hierarchy is updated after NetworkObject node is dirtied.
class URHO3D_API NetworkObject
    : public TrackedComponent<ReferencedComponentBase, NetworkObjectRegistry>
    , public NetworkCallback
{
    URHO3D_OBJECT(NetworkObject, ReferencedComponentBase);

public:
    explicit NetworkObject(Context* context);
    ~NetworkObject() override;

    /// Server-only: set owner connection which is allowed to send feedback for this object.
    void SetOwner(AbstractConnection* owner);

    static void RegisterObject(Context* context);

    /// Internal API for ReplicationManager.
    /// @{
    void UpdateObjectHierarchy();
    void SetNetworkId(NetworkId networkId) { SetReference(networkId); }
    void SetNetworkMode(NetworkObjectMode mode) { networkMode_ = mode; }
    /// @}

    /// Return current or last NetworkId. Return NetworkId::None if not registered.
    NetworkId GetNetworkId() const { return GetReference(); }
    ReplicationManager* GetReplicationManager() const { return static_cast<ReplicationManager*>(GetRegistry()); }
    NetworkId GetParentNetworkId() const;
    NetworkObject* GetParentNetworkObject() const { return parentNetworkObject_; }
    const ea::vector<WeakPtr<NetworkObject>>& GetChildrenNetworkObjects() const { return childrenNetworkObjects_; }
    AbstractConnection* GetOwnerConnection() const { return ownerConnection_; }
    unsigned GetOwnerConnectionId() const { return ownerConnection_ ? ownerConnection_->GetObjectID() : 0; }

    /// Return network mode.
    /// Network mode is configured only *after* InitializeOnServer and InitializeFromSnapshot callbacks.
    /// Before these callbacks the object is considered Standalone.
    /// This is useful to prevent changes in already initialized objects.
    NetworkObjectMode GetNetworkMode() const { return networkMode_; }
    bool IsStandalone() const { return networkMode_ == NetworkObjectMode::Standalone; }
    bool IsServer() const { return networkMode_ == NetworkObjectMode::Server; }
    bool IsOwnedByThisClient() const { return networkMode_ == NetworkObjectMode::ClientOwned; }
    bool IsReplicatedClient() const { return networkMode_ == NetworkObjectMode::ClientReplicated; }

    /// Implement ClientNetworkCallback.
    /// @{
    void PrepareToRemove() override;
    /// @}

protected:
    /// Component implementation
    /// @{
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    void OnMarkedDirty(Node* node) override;
    /// @}

    NetworkObject* GetOtherNetworkObject(NetworkId networkId) const;
    void SetParentNetworkObject(NetworkId parentNetworkId);

private:
    NetworkObject* FindParentNetworkObject() const;
    void AddChildNetworkObject(NetworkObject* networkObject);
    void RemoveChildNetworkObject(NetworkObject* networkObject);

    /// ReplicationManager corresponding to the NetworkObject.
    NetworkObjectMode networkMode_{};
    WeakPtr<AbstractConnection> ownerConnection_{};

    /// NetworkObject hierarchy
    /// @{
    WeakPtr<NetworkObject> parentNetworkObject_;
    ea::vector<WeakPtr<NetworkObject>> childrenNetworkObjects_;
    /// @}
};

} // namespace Urho3D
