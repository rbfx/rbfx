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

#include "../Core/Assert.h"
#include "../Container/FlagSet.h"
#include "../Scene/Component.h"
#include "../Network/AbstractConnection.h"
#include "../Network/NetworkManager.h"

#include <EASTL/fixed_vector.h>
#include <EASTL/optional.h>

namespace Urho3D
{

class AbstractConnection;

enum class NetworkObjectMode
{
    /// Default state of NetworkObject.
    /// If scene is not replicated from/to, NetworkObject in such scene stays as Draft.
    /// If scene is replicated, NetworkObject is a draft until it's processed by Network subsystem.
    Draft,
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
class URHO3D_API NetworkObject : public Component
{
    URHO3D_OBJECT(NetworkObject, Component);

public:
    explicit NetworkObject(Context* context);
    ~NetworkObject() override;

    /// Server-only: set owner connection which is allowed to send feedback for this object.
    void SetOwner(AbstractConnection* owner);

    static void RegisterObject(Context* context);

    /// Internal API for NetworkManager.
    /// @{
    void UpdateObjectHierarchy();
    void SetNetworkId(NetworkId networkId) { networkId_ = networkId; }
    void SetNetworkMode(NetworkObjectMode mode) { networkMode_ = mode; }
    /// @}

    /// Return current or last NetworkId. Return InvalidNetworkId if not registered.
    NetworkId GetNetworkId() const { return networkId_; }
    NetworkId GetParentNetworkId() const { return parentNetworkObject_ ? parentNetworkObject_->GetNetworkId() : InvalidNetworkId; }
    NetworkObject* GetParentNetworkObject() const { return parentNetworkObject_; }
    const auto& GetChildrenNetworkObjects() const { return childrenNetworkObjects_; }
    NetworkObjectMode GetNetworkMode() const { return networkMode_; }
    AbstractConnection* GetOwnerConnection() const { return ownerConnection_; }
    unsigned GetOwnerConnectionId() const { return ownerConnection_ ? ownerConnection_->GetObjectID() : 0; }
    ClientNetworkManager* GetClientNetworkManager() const;
    ServerNetworkManager* GetServerNetworkManager() const;

    /// Called on server side only. ServerNetworkManager is guaranteed to be available.
    /// @{

    /// Return whether the component should be replicated for specified client connection.
    virtual bool IsRelevantForClient(AbstractConnection* connection);
    /// Perform server-side initialization. Called once.
    virtual void InitializeOnServer();
    /// Called when transform of the object is dirtied.
    virtual void UpdateTransformOnServer();
    /// Write full snapshot on server.
    virtual void WriteSnapshot(unsigned frame, Serializer& dest);
    /// Return mask for reliable delta update. If mask is zero, write will be omitted.
    /// TODO(network): Rename this shit!
    virtual unsigned GetReliableDeltaMask(unsigned frame);
    /// Write reliable delta update on server. Delta is applied to previous delta or snapshot message.
    virtual void WriteReliableDelta(unsigned frame, unsigned mask, Serializer& dest);
    /// Return mask for unreliable delta update. If mask is zero, write will be omitted.
    virtual unsigned GetUnreliableDeltaMask(unsigned frame);
    /// Write unreliable delta update on server.
    virtual void WriteUnreliableDelta(unsigned frame, unsigned mask, Serializer& dest);
    /// Read unreliable feedback from client.
    virtual void ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src);

    /// @}

    /// Called on client side only. ClientNetworkManager is guaranteed to be available and synchronized.
    /// @{

    /// Interpolate replicated state.
    virtual void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, const ea::optional<unsigned>& isNewInputFrame);
    /// Prepare to this compnent being removed by the authority of the server.
    virtual void PrepareToRemove();
    /// Read full snapshot.
    virtual void ReadSnapshot(unsigned frame, Deserializer& src); // TODO(network): Rename to InitializeFromSnapshot?
    /// Read reliable delta update. Delta is applied to previous reliable delta or snapshot message.
    virtual void ReadReliableDelta(unsigned frame, Deserializer& src);
    /// Read unreliable delta update.
    virtual void ReadUnreliableDelta(unsigned frame, Deserializer& src);
    /// Return mask for unreliable feedback. If mask is zero, write will be omitted.
    virtual unsigned GetUnreliableFeedbackMask(unsigned frame);
    /// Write unreliable feedback from client.
    virtual void WriteUnreliableFeedback(unsigned frame, unsigned mask, Serializer& dest);

    /// @}

protected:
    /// Component implementation
    /// @{
    void OnNodeSet(Node* node) override;
    void OnMarkedDirty(Node* node) override;
    /// @}

    NetworkObject* GetOtherNetworkObject(NetworkId networkId) const;
    void SetParentNetworkObject(NetworkId parentNetworkId);

private:
    void UpdateCurrentScene(Scene* scene);
    NetworkObject* FindParentNetworkObject() const;
    void AddChildNetworkObject(NetworkObject* networkObject);
    void RemoveChildNetworkObject(NetworkObject* networkObject);

    /// NetworkManager corresponding to the NetworkObject.
    WeakPtr<NetworkManager> networkManager_;
    /// Network ID, unique within Scene.
    /// May contain outdated value if NetworkObject is not registered in any NetworkManager.
    NetworkId networkId_{InvalidNetworkId};
    NetworkObjectMode networkMode_{};
    AbstractConnection* ownerConnection_{};

    /// NetworkObject hierarchy
    /// @{
    WeakPtr<NetworkObject> parentNetworkObject_;
    ea::vector<WeakPtr<NetworkObject>> childrenNetworkObjects_;
    /// @}
};

}
