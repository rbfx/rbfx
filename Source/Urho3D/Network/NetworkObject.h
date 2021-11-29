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

#include "../Container/FlagSet.h"
#include "../Scene/Component.h"
#include "../Network/NetworkManager.h"

namespace Urho3D
{

class AbstractConnection;

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

    static void RegisterObject(Context* context);

    /// Update pointer to the parent NetworkObject.
    void UpdateParent();
    /// Assign NetworkId. On the Server, it's better to let the Server assign ID, to avoid unwanted side effects.
    void SetNetworkId(NetworkId networkId) { networkId_ = networkId; }
    /// Return current or last NetworkId. Return InvalidNetworkId if not registered.
    NetworkId GetNetworkId() const { return networkId_; }
    /// Return NetworkId of parent NetworkObject.
    NetworkId GetParentNetworkId() const { return parentNetworkObject_ ? parentNetworkObject_->GetNetworkId() : InvalidNetworkId; }
    /// Return parent NetworkObject.
    NetworkObject* GetParentNetworkObject() const { return parentNetworkObject_; }
    /// Return children NetworkObject.
    const auto& GetChildrenNetworkObjects() const { return childrenNetworkObjects_; }

    /// Return whether the component should be replicated for specified client connection.
    virtual bool IsRelevantForClient(AbstractConnection* connection);
    /// Callen when component is removed by server.
    virtual void OnRemovedOnClient();
    /// Write full snapshot on server.
    virtual void WriteSnapshot(VectorBuffer& dest);
    /// Write delta update on server. Delta is applied to previous delta or snapshot message.
    virtual bool WriteReliableDelta(VectorBuffer& dest);
    /// Read full snapshot on client.
    virtual void ReadSnapshot(VectorBuffer& src);
    /// Read delta update on client. Delta is applied to previous delta or snapshot message.
    virtual void ReadReliableDelta(VectorBuffer& src);

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
    /// Network ID, unique within Scene. May contain outdated value if NetworkObject is not registered in any NetworkManager.
    NetworkId networkId_{ InvalidNetworkId };

    /// NetworkObject hierarchy
    /// @{
    WeakPtr<NetworkObject> parentNetworkObject_;
    ea::vector<WeakPtr<NetworkObject>> childrenNetworkObjects_;
    /// @}
};

/// Default implementation of NetworkObject that does some basic replication.
class URHO3D_API DefaultNetworkObject : public NetworkObject
{
    URHO3D_OBJECT(DefaultNetworkObject, NetworkObject);

public:
    explicit DefaultNetworkObject(Context* context);
    ~DefaultNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Implementation of NetworkObject
    /// @{
    void WriteSnapshot(VectorBuffer& dest) override;
    bool WriteReliableDelta(VectorBuffer& dest) override;
    void ReadSnapshot(VectorBuffer& src) override;
    void ReadReliableDelta(VectorBuffer& src) override;
    /// @}

private:
    /// Delta updates cache
    /// @{
    NetworkId lastParentNetworkId_{ InvalidNetworkId };
    /// @}
};

}
