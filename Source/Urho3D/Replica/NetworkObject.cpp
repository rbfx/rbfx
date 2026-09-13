// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Network/NetworkDefs.h"
#include "Urho3D/Replica/NetworkObject.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

NetworkObject::NetworkObject(Context* context)
    : TrackedComponent<ReferencedComponentBase, NetworkObjectRegistry>(context)
{
}

NetworkObject::~NetworkObject() = default;

void NetworkObject::SetOwner(ReplicatedPeerPtr owner)
{
    if (networkMode_ != NetworkObjectMode::Standalone)
    {
        URHO3D_ASSERTLOG(0, "NetworkObject::SetOwner may be called only for NetworkObject in Standalone mode");
        return;
    }

    ownerConnection_ = owner;
}

void NetworkObject::RegisterObject(Context* context)
{
    context->AddAbstractReflection<NetworkObject>(Category_Network);
}

void NetworkObject::UpdateObjectHierarchy()
{
    NetworkObject* newParentNetworkObject = node_->FindComponent<NetworkObject>(ComponentSearchFlag::ParentRecursive | ComponentSearchFlag::Derived);
    if (newParentNetworkObject != parentNetworkObject_)
    {
        if (parentNetworkObject_)
            parentNetworkObject_->RemoveChildNetworkObject(this);

        parentNetworkObject_ = newParentNetworkObject;

        if (parentNetworkObject_)
            parentNetworkObject_->AddChildNetworkObject(this);
    }

    // Remove expired children
    ea::erase_if(childrenNetworkObjects_, [](const WeakPtr<NetworkObject>& child) { return !child; });

    if (IsServer())
        UpdateTransformOnServer();
}

NetworkId NetworkObject::GetParentNetworkId() const
{
    return parentNetworkObject_ ? parentNetworkObject_->GetNetworkId() : NetworkId::None;
}

void NetworkObject::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
    {
        node_->AddListener(this);
        node_->MarkDirty();
    }
    else
    {
        for (NetworkObject* childNetworkObject : childrenNetworkObjects_)
        {
            if (!childNetworkObject)
                continue;

            childNetworkObject->GetNode()->MarkDirty();
        }
    }
}

void NetworkObject::OnMarkedDirty(Node* node)
{
    if (auto replicationManager = GetReplicationManager())
        replicationManager->QueueNetworkObjectUpdate(this);
}

NetworkObject* NetworkObject::GetOtherNetworkObject(NetworkId networkId) const
{
    return GetReplicationManager() ? GetReplicationManager()->GetNetworkObject(networkId) : nullptr;
}

ea::optional<float> NetworkObject::CalculateDistanceForFiltering(NetworkObject* otherNetworkObject)
{
    const Vector3 thisPosition = GetNode()->GetWorldPosition();
    const Vector3 otherPosition = otherNetworkObject->GetNode()->GetWorldPosition();
    return (thisPosition - otherPosition).Length();
}

void NetworkObject::SetParentNetworkObject(NetworkId parentNetworkId)
{
    if (parentNetworkId != NetworkId::None)
    {
        if (auto parentNetworkObject = GetOtherNetworkObject(parentNetworkId))
        {
            Node* parentNode = parentNetworkObject->GetNode();
            if (node_->GetParent() != parentNode)
                node_->SetParent(parentNode);
        }
        else
        {
            URHO3D_LOGERROR("Cannot assign NetworkObject {} to unknown parent NetworkObject {}",
                ToString(GetNetworkId()), ToString(parentNetworkId));
        }
    }
    else
    {
        Node* parentNode = GetScene();
        if (node_->GetParent() != parentNode)
            node_->SetParent(parentNode);
    }
}

void NetworkObject::AddChildNetworkObject(NetworkObject* networkObject)
{
    childrenNetworkObjects_.emplace_back(networkObject);
}

void NetworkObject::RemoveChildNetworkObject(NetworkObject* networkObject)
{
    const auto iter = childrenNetworkObjects_.find(WeakPtr<NetworkObject>(networkObject));
    if (iter != childrenNetworkObjects_.end())
        childrenNetworkObjects_.erase(iter);
}

void NetworkObject::PrepareToRemove()
{
    if (node_)
        node_->Remove();
}

} // namespace Urho3D
