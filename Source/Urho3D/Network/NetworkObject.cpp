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
#include "../IO/Log.h"
#if defined(URHO3D_PHYSICS) || defined(URHO3D_URHO2D)
#include "../Physics/PhysicsEvents.h"
#endif
#include "../Network/NetworkObject.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{

NetworkObject::NetworkObject(Context* context) : Component(context) {}

NetworkObject::~NetworkObject() = default;

void NetworkObject::RegisterObject(Context* context)
{
    context->RegisterFactory<NetworkObject>();
}

void NetworkObject::UpdateParent()
{
    NetworkObject* newParentNetworkObject = FindParentNetworkObject();
    if (newParentNetworkObject != parentNetworkObject_)
    {
        if (parentNetworkObject_)
            parentNetworkObject_->RemoveChildNetworkObject(this);

        parentNetworkObject_ = newParentNetworkObject;

        if (parentNetworkObject_)
            parentNetworkObject_->AddChildNetworkObject(this);
    }
}

void NetworkObject::OnNodeSet(Node* node)
{
    if (node)
    {
        UpdateCurrentScene(node->GetScene());
        node->AddListener(this);
        node->MarkDirty();
    }
    else
    {
        UpdateCurrentScene(nullptr);
        for (NetworkObject* childNetworkObject : childrenNetworkObjects_)
        {
            if (!childNetworkObject)
                continue;

            childNetworkObject->GetNode()->MarkDirty();
        }
    }
}

void NetworkObject::UpdateCurrentScene(Scene* scene)
{
    NetworkManager* newNetworkManager = scene ? scene->GetNetworkManager() : nullptr;
    if (newNetworkManager != networkManager_)
    {
        if (networkManager_)
        {
            // Remove only if still valid
            if (networkManager_->GetNetworkObject(networkId_) == this)
                networkManager_->RemoveComponent(this);
            networkManager_ = nullptr;
        }

        if (scene)
        {
            networkManager_ = scene->GetNetworkManager();
            networkManager_->AddComponent(this);
        }
    }
}

void NetworkObject::OnMarkedDirty(Node* node)
{
    if (networkManager_)
        networkManager_->QueueComponentUpdate(this);
}

NetworkObject* NetworkObject::GetOtherNetworkObject(NetworkId networkId) const
{
    return networkManager_ ? networkManager_->GetNetworkObject(networkId) : nullptr;
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
                NetworkManagerBase::FormatNetworkId(GetNetworkId()), NetworkManagerBase::FormatNetworkId(parentNetworkId));
        }
    }
    else
    {
        Node* parentNode = GetScene();
        if (node_->GetParent() != parentNode)
            node_->SetParent(parentNode);
    }
}

NetworkObject* NetworkObject::FindParentNetworkObject() const
{
    Node* parent = node_->GetParent();
    while (parent)
    {
        if (auto networkObject = parent->GetDerivedComponent<NetworkObject>())
            return networkObject;
        parent = parent->GetParent();
    }
    return nullptr;
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

bool NetworkObject::IsRelevantForClient(AbstractConnection* connection)
{
    return true;
}

void NetworkObject::InitializeReliableDelta()
{
}

void NetworkObject::OnRemovedOnClient()
{
    if (node_)
        node_->Remove();
}

void NetworkObject::WriteSnapshot(VectorBuffer& dest)
{
}

bool NetworkObject::WriteReliableDelta(VectorBuffer& dest)
{
    return false;
}

void NetworkObject::ReadSnapshot(VectorBuffer& src)
{
}

void NetworkObject::ReadReliableDelta(VectorBuffer& src)
{
}

DefaultNetworkObject::DefaultNetworkObject(Context* context) : NetworkObject(context) {}

DefaultNetworkObject::~DefaultNetworkObject() = default;

void DefaultNetworkObject::RegisterObject(Context* context)
{
    context->RegisterFactory<DefaultNetworkObject>();
}

void DefaultNetworkObject::InitializeReliableDelta()
{
    lastParentNetworkId_ = GetParentNetworkId();
}

void DefaultNetworkObject::WriteSnapshot(VectorBuffer& dest)
{
    const auto parentNetworkId = GetParentNetworkId();
    dest.WriteUInt(static_cast<unsigned>(parentNetworkId));

    dest.WriteString(node_->GetName());

    dest.WriteVector3(node_->GetWorldPosition());
    dest.WritePackedQuaternion(node_->GetWorldRotation());
    dest.WriteVector3(node_->GetSignedWorldScale());
}

bool DefaultNetworkObject::WriteReliableDelta(VectorBuffer& dest)
{
    const auto parentNetworkId = GetParentNetworkId();

    if (lastParentNetworkId_ != parentNetworkId)
    {
        lastParentNetworkId_ = parentNetworkId;
        dest.WriteUInt(static_cast<unsigned>(parentNetworkId));
    }

    return dest.Tell() != 0;
}

void DefaultNetworkObject::ReadSnapshot(VectorBuffer& src)
{
    const auto parentNetworkId = static_cast<NetworkId>(src.ReadUInt());
    SetParentNetworkObject(parentNetworkId);

    node_->SetName(src.ReadString());

    const Vector3 worldPosition = src.ReadVector3();
    const Quaternion worldRotation = src.ReadPackedQuaternion();
    const Vector3 worldScale = src.ReadVector3();
    const Matrix3x4 worldTransform{ worldPosition, worldRotation, worldScale };
    const Matrix3x4 localTransform = node_->IsTransformHierarchyRoot()
        ? worldTransform
        : node_->GetParent()->GetWorldTransform().Inverse() * worldTransform;
    node_->SetTransform(localTransform);
}

void DefaultNetworkObject::ReadReliableDelta(VectorBuffer& src)
{
    const auto parentNetworkId = static_cast<NetworkId>(src.ReadUInt());
    SetParentNetworkObject(parentNetworkId);
}

}
