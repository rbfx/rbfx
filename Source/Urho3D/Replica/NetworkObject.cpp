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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "../Replica/NetworkObject.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

NetworkObject::NetworkObject(Context* context) : TrackedComponent<NetworkObjectRegistry>(context) {}

NetworkObject::~NetworkObject() = default;

void NetworkObject::SetOwner(AbstractConnection* owner)
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
    context->RegisterFactory<NetworkObject>();
}

void NetworkObject::UpdateObjectHierarchy()
{
    NetworkObject* newParentNetworkObject = node_->GetParentDerivedComponent<NetworkObject>(true);
    if (newParentNetworkObject != parentNetworkObject_)
    {
        if (parentNetworkObject_)
            parentNetworkObject_->RemoveChildNetworkObject(this);

        parentNetworkObject_ = newParentNetworkObject;

        if (parentNetworkObject_)
            parentNetworkObject_->AddChildNetworkObject(this);
    }

    if (IsServer())
        UpdateTransformOnServer();
}

void NetworkObject::OnNodeSet(Node* node)
{
    if (node)
    {
        URHO3D_ASSERT(!node->IsReplicated(), "NetworkObject can be assigned only to LOCAL Node");

        node->AddListener(this);
        node->MarkDirty();
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

void NetworkObject::SetParentNetworkObject(NetworkId parentNetworkId)
{
    if (parentNetworkId != InvalidNetworkId)
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

}
