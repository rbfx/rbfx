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
#include "../Replica/BehaviorNetworkObject.h"
#include "../Scene/SceneEvents.h"

namespace Urho3D
{

NetworkBehavior::NetworkBehavior(Context* context, NetworkCallbackFlags callbackMask)
    : Component(context)
    , callbackMask_(callbackMask)
{
}

NetworkBehavior::~NetworkBehavior()
{
}

void NetworkBehavior::RegisterObject(Context* context)
{
    context->AddAbstractReflection<NetworkBehavior>(Category_Network);
}

void NetworkBehavior::SetNetworkObject(BehaviorNetworkObject* owner)
{
    owner_ = owner;
}

NetworkObject* NetworkBehavior::FindClosestNetworkObject() const
{
    if (!node_)
        return nullptr;
    return node_->FindComponent<NetworkObject>(ComponentSearchFlag::SelfOrParentRecursive | ComponentSearchFlag::Derived);
}

void NetworkBehavior::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (!node_ && owner_)
    {
        owner_->InvalidateBehaviors();
        owner_ = nullptr;
    }
}

BehaviorNetworkObject::BehaviorNetworkObject(Context* context)
    : StaticNetworkObject(context)
{
}

BehaviorNetworkObject::~BehaviorNetworkObject()
{
}

void BehaviorNetworkObject::RegisterObject(Context* context)
{
    context->AddFactoryReflection<BehaviorNetworkObject>(Category_Network);

    URHO3D_COPY_BASE_ATTRIBUTES(StaticNetworkObject);
}

void BehaviorNetworkObject::InitializeBehaviors()
{
    InvalidateBehaviors();

    ea::vector<NetworkBehavior*> networkBehaviors;
    node_->FindComponents(networkBehaviors, ComponentSearchFlag::SelfOrChildrenRecursive | ComponentSearchFlag::Derived);

    ea::erase_if(networkBehaviors,
        [this](NetworkBehavior* networkBehavior) { return networkBehavior->FindClosestNetworkObject() != this; });

    if (networkBehaviors.size() > MaxNumBehaviors)
    {
        URHO3D_LOGERROR("Cannot connect more than {} NetworkBehavior-s to exiting NetworkObject {}", MaxNumBehaviors,
            ToString(GetNetworkId()));
        return;
    }

    for (NetworkBehavior* networkBehavior : networkBehaviors)
    {
        const unsigned bit = 1 << behaviors_.size();
        WeakPtr<NetworkBehavior> weakPtr{networkBehavior};
        const auto callbackMask = networkBehavior->GetCallbackMask();

        networkBehavior->SetNetworkObject(this);
        behaviors_.push_back(ConnectedNetworkBehavior{bit, weakPtr, callbackMask});
        callbackMask_ |= callbackMask;
    }

    UnsubscribeFromEvent(E_SCENENETWORKUPDATE);
    if (callbackMask_.Test(NetworkCallbackMask::Update))
    {
        SubscribeToEvent(GetScene(), E_SCENENETWORKUPDATE,
            [this](VariantMap& eventData)
        {
            using namespace SceneNetworkUpdate;
            const float replicaTimeStep = eventData[P_TIMESTEP_REPLICA].GetFloat();
            const float inputTimeStep = eventData[P_TIMESTEP_INPUT].GetFloat();
            Update(replicaTimeStep, inputTimeStep);
        });
    }
}

void BehaviorNetworkObject::InvalidateBehaviors()
{
    callbackMask_ = NetworkCallbackMask::None;
    behaviors_.clear();
}

NetworkBehavior* BehaviorNetworkObject::GetNetworkBehavior(StringHash componentType, unsigned index) const
{
    for (const auto& connectedBehavior : behaviors_)
    {
        URHO3D_ASSERT(connectedBehavior.component_);
        if (connectedBehavior.component_->GetType() == componentType)
        {
            if (index == 0)
                return connectedBehavior.component_;
            else
                --index;
        }
    }
    return nullptr;
}

void BehaviorNetworkObject::InitializeStandalone()
{
    BaseClassName::InitializeStandalone();

    InitializeBehaviors();

    for (const auto& connectedBehavior : behaviors_)
        connectedBehavior.component_->InitializeStandalone();
}

void BehaviorNetworkObject::InitializeOnServer()
{
    BaseClassName::InitializeOnServer();

    InitializeBehaviors();

    for (const auto& connectedBehavior : behaviors_)
        connectedBehavior.component_->InitializeOnServer();
}

void BehaviorNetworkObject::WriteSnapshot(NetworkFrame frame, Serializer& dest)
{
    BaseClassName::WriteSnapshot(frame, dest);

    // Write metadata of connected behaviors for validation
    dest.WriteVLE(behaviors_.size());
    for (const auto& connectedBehavior : behaviors_)
        dest.WriteStringHash(connectedBehavior.component_->GetType());

    // Write actual behaviors data
    for (const auto& connectedBehavior : behaviors_)
        connectedBehavior.component_->WriteSnapshot(frame, dest);
}

void BehaviorNetworkObject::InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned)
{
    BaseClassName::InitializeFromSnapshot(frame, src, isOwned);

    InitializeBehaviors();

    // Read and check behaviors
    const unsigned expectedNumBehaviors = src.ReadVLE();
    if (expectedNumBehaviors != behaviors_.size())
    {
        URHO3D_LOGERROR("Number of behaviors in NetworkObject {} doesn't match on the client ({}) and the server ({})",
            ToString(GetNetworkId()), behaviors_.size(), expectedNumBehaviors);
        InvalidateBehaviors();
        return;
    }

    for (unsigned i = 0; i < expectedNumBehaviors; ++i)
    {
        const StringHash expectedType = src.ReadStringHash();
        NetworkBehavior* behavior = behaviors_[i].component_;
        if (expectedType != behavior->GetType())
        {
            URHO3D_LOGERROR("Type of behavior #{} in NetworkObject {} doesn't match on the client ({}) and the server ({})",
                i, ToString(GetNetworkId()), behavior->GetType().ToDebugString(), expectedType.ToDebugString());
            InvalidateBehaviors();
            return;
        }
    }

    // Read behaviors only if matching
    for (const auto& connectedBehavior : behaviors_)
        connectedBehavior.component_->InitializeFromSnapshot(frame, src, isOwned);
}

ea::optional<NetworkObjectRelevance> BehaviorNetworkObject::GetRelevanceForClient(AbstractConnection* connection)
{
    if (callbackMask_.Test(NetworkCallbackMask::GetRelevanceForClient))
    {
        for (const auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallbackMask::GetRelevanceForClient))
            {
                if (const auto relevance = connectedBehavior.component_->GetRelevanceForClient(connection))
                    return relevance;
            }
        }
    }
    return ea::nullopt;
}

void BehaviorNetworkObject::UpdateTransformOnServer()
{
    BaseClassName::UpdateTransformOnServer();

    if (callbackMask_.Test(NetworkCallbackMask::UpdateTransformOnServer))
    {
        for (const auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallbackMask::UpdateTransformOnServer))
                connectedBehavior.component_->UpdateTransformOnServer();
        }
    }
}

void BehaviorNetworkObject::InterpolateState(
    float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    BaseClassName::InterpolateState(replicaTimeStep, inputTimeStep, replicaTime, inputTime);

    if (callbackMask_.Test(NetworkCallbackMask::InterpolateState))
    {
        for (const auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallbackMask::InterpolateState))
                connectedBehavior.component_->InterpolateState(replicaTimeStep, inputTimeStep, replicaTime, inputTime);
        }
    }
}

bool BehaviorNetworkObject::PrepareReliableDelta(NetworkFrame frame)
{
    bool needUpdate = BaseClassName::PrepareReliableDelta(frame);

    reliableUpdateMask_ = 0;
    if (callbackMask_.Test(NetworkCallbackMask::ReliableDelta))
    {
        for (auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallbackMask::ReliableDelta))
            {
                if (connectedBehavior.component_->PrepareReliableDelta(frame))
                    reliableUpdateMask_ |= connectedBehavior.bit_;
            }
        }
    }

    needUpdate = needUpdate || reliableUpdateMask_ != 0;
    return needUpdate;
}

void BehaviorNetworkObject::WriteReliableDelta(NetworkFrame frame, Serializer& dest)
{
    BaseClassName::WriteReliableDelta(frame, dest);

    if (callbackMask_.Test(NetworkCallbackMask::ReliableDelta))
    {
        dest.WriteVLE(reliableUpdateMask_);
        for (const auto& connectedBehavior : behaviors_)
        {
            if (reliableUpdateMask_ & connectedBehavior.bit_)
                connectedBehavior.component_->WriteReliableDelta(frame, dest);
        }
    }
}

void BehaviorNetworkObject::ReadReliableDelta(NetworkFrame frame, Deserializer& src)
{
    BaseClassName::ReadReliableDelta(frame, src);

    if (callbackMask_.Test(NetworkCallbackMask::ReliableDelta))
    {
        const unsigned mask = src.ReadVLE();
        for (const auto& connectedBehavior : behaviors_)
        {
            if (mask & connectedBehavior.bit_)
                connectedBehavior.component_->ReadReliableDelta(frame, src);
        }
    }
}

bool BehaviorNetworkObject::PrepareUnreliableDelta(NetworkFrame frame)
{
    bool needUpdate = BaseClassName::PrepareUnreliableDelta(frame);

    unreliableUpdateMask_ = 0;
    if (callbackMask_.Test(NetworkCallbackMask::UnreliableDelta))
    {
        for (auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallbackMask::UnreliableDelta))
            {
                if (connectedBehavior.component_->PrepareUnreliableDelta(frame))
                    unreliableUpdateMask_ |= connectedBehavior.bit_;
            }
        }
    }

    needUpdate = needUpdate || unreliableUpdateMask_ != 0;
    return needUpdate;
}

void BehaviorNetworkObject::WriteUnreliableDelta(NetworkFrame frame, Serializer& dest)
{
    BaseClassName::WriteUnreliableDelta(frame, dest);

    if (callbackMask_.Test(NetworkCallbackMask::UnreliableDelta))
    {
        dest.WriteVLE(unreliableUpdateMask_);
        for (const auto& connectedBehavior : behaviors_)
        {
            if (unreliableUpdateMask_ & connectedBehavior.bit_)
                connectedBehavior.component_->WriteUnreliableDelta(frame, dest);
        }
    }
}

void BehaviorNetworkObject::ReadUnreliableDelta(NetworkFrame frame, Deserializer& src)
{
    BaseClassName::ReadUnreliableDelta(frame, src);

    if (callbackMask_.Test(NetworkCallbackMask::UnreliableDelta))
    {
        const unsigned mask = src.ReadVLE();
        for (const auto& connectedBehavior : behaviors_)
        {
            if (mask & connectedBehavior.bit_)
                connectedBehavior.component_->ReadUnreliableDelta(frame, src);
        }
    }
}

bool BehaviorNetworkObject::PrepareUnreliableFeedback(NetworkFrame frame)
{
    bool needUpdate = BaseClassName::PrepareUnreliableFeedback(frame);

    unreliableFeedbackMask_ = 0;
    if (callbackMask_.Test(NetworkCallbackMask::UnreliableFeedback))
    {
        for (auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallbackMask::UnreliableFeedback))
            {
                if (connectedBehavior.component_->PrepareUnreliableFeedback(frame))
                    unreliableFeedbackMask_ |= connectedBehavior.bit_;
            }
        }
    }

    needUpdate = needUpdate || unreliableFeedbackMask_ != 0;
    return needUpdate;
}

void BehaviorNetworkObject::WriteUnreliableFeedback(NetworkFrame frame, Serializer& dest)
{
    BaseClassName::WriteUnreliableFeedback(frame, dest);

    if (callbackMask_.Test(NetworkCallbackMask::UnreliableFeedback))
    {
        dest.WriteVLE(unreliableFeedbackMask_);
        for (const auto& connectedBehavior : behaviors_)
        {
            if (unreliableFeedbackMask_ & connectedBehavior.bit_)
                connectedBehavior.component_->WriteUnreliableFeedback(frame, dest);
        }
    }
}

void BehaviorNetworkObject::ReadUnreliableFeedback(NetworkFrame feedbackFrame, Deserializer& src)
{
    BaseClassName::ReadUnreliableFeedback(feedbackFrame, src);

    if (callbackMask_.Test(NetworkCallbackMask::UnreliableFeedback))
    {
        const unsigned mask = src.ReadVLE();
        for (const auto& connectedBehavior : behaviors_)
        {
            if (mask & connectedBehavior.bit_)
                connectedBehavior.component_->ReadUnreliableFeedback(feedbackFrame, src);
        }
    }
}

void BehaviorNetworkObject::Update(float replicaTimeStep, float inputTimeStep)
{
    BaseClassName::Update(replicaTimeStep, inputTimeStep);

    for (const auto& connectedBehavior : behaviors_)
    {
        if (connectedBehavior.callbackMask_.Test(NetworkCallbackMask::Update))
            connectedBehavior.component_->Update(replicaTimeStep, inputTimeStep);
    }
}

}
