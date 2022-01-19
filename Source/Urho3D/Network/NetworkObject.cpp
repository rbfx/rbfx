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
#include "../Network/NetworkObject.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

NetworkObject::NetworkObject(Context* context) : Component(context) {}

NetworkObject::~NetworkObject() = default;

void NetworkObject::SetOwner(AbstractConnection* owner)
{
    if (networkMode_ != NetworkObjectMode::Draft)
    {
        URHO3D_ASSERTLOG(0, "NetworkObject::SetOwner may be called only for NetworkObject in Draft mode");
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
    NetworkObject* newParentNetworkObject = FindParentNetworkObject();
    if (newParentNetworkObject != parentNetworkObject_)
    {
        if (parentNetworkObject_)
            parentNetworkObject_->RemoveChildNetworkObject(this);

        parentNetworkObject_ = newParentNetworkObject;

        if (parentNetworkObject_)
            parentNetworkObject_->AddChildNetworkObject(this);
    }

    UpdateTransformOnServer();
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

ClientNetworkManager* NetworkObject::GetClientNetworkManager() const
{
    return networkManager_ && networkManager_->IsReplicatedClient() ? &networkManager_->AsClient() : nullptr;
}

ServerNetworkManager* NetworkObject::GetServerNetworkManager() const
{
    return networkManager_ && !networkManager_->IsReplicatedClient() ? &networkManager_->AsServer() : nullptr;
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

void NetworkObject::InitializeOnServer()
{
}

void NetworkObject::UpdateTransformOnServer()
{
}

void NetworkObject::WriteSnapshot(unsigned frame, Serializer& dest)
{
}

unsigned NetworkObject::GetReliableDeltaMask(unsigned frame)
{
    return 0;
}

void NetworkObject::WriteReliableDelta(unsigned frame, unsigned mask, Serializer& dest)
{
}

unsigned NetworkObject::GetUnreliableDeltaMask(unsigned frame)
{
    return 0;
}

void NetworkObject::WriteUnreliableDelta(unsigned frame, unsigned mask, Serializer& dest)
{
}

void NetworkObject::ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src)
{
}

void NetworkObject::InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime, const ea::optional<unsigned>& isNewInputFrame)
{
}

void NetworkObject::PrepareToRemove()
{
    if (node_)
        node_->Remove();
}

void NetworkObject::ReadSnapshot(unsigned frame, Deserializer& src)
{
}

void NetworkObject::ReadReliableDelta(unsigned frame, Deserializer& src)
{
}

void NetworkObject::ReadUnreliableDelta(unsigned frame, Deserializer& src)
{
}

unsigned NetworkObject::GetUnreliableFeedbackMask(unsigned frame)
{
    return 0;
}

void NetworkObject::WriteUnreliableFeedback(unsigned frame, unsigned mask, Serializer& dest)
{
}

}
