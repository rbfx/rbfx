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
#include "../Replica/BehaviorNetworkObject.h"

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
    context->RegisterFactory<NetworkBehavior>();
}

void NetworkBehavior::SetNetworkObject(BehaviorNetworkObject* owner)
{
    owner_ = owner;
}

void NetworkBehavior::OnNodeSet(Node* node)
{
    if (!node && owner_)
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
    context->RegisterFactory<BehaviorNetworkObject>();

    URHO3D_COPY_BASE_ATTRIBUTES(StaticNetworkObject);
}

void BehaviorNetworkObject::InitializeBehaviors()
{
    ea::vector<NetworkBehavior*> networkBehaviors;
    node_->GetDerivedComponents(networkBehaviors, true);

    if (networkBehaviors.size() > MaxNumBehaviors)
    {
        URHO3D_LOGERROR("Cannot connect more than {} NetworkBehavior-s to exiting NetworkObject {}", MaxNumBehaviors,
            ToString(GetNetworkId()));
        return;
    }

    callbackMask_ = NetworkCallback::None;
    for (NetworkBehavior* networkBehavior : networkBehaviors)
    {
        const unsigned bit = 1 << behaviors_.size();
        WeakPtr<NetworkBehavior> weakPtr{networkBehavior};
        const auto callbackMask = networkBehavior->GetCallbackMask();

        networkBehavior->SetNetworkObject(this);
        behaviors_.push_back(ConnectedNetworkBehavior{bit, weakPtr, callbackMask});
        callbackMask_ |= callbackMask;
    }
}

void BehaviorNetworkObject::InvalidateBehaviors()
{
    behaviors_.clear();
}

void BehaviorNetworkObject::InitializeOnServer()
{
    BaseClassName::InitializeOnServer();

    InitializeBehaviors();

    for (const auto& connectedBehavior : behaviors_)
        connectedBehavior.component_->InitializeOnServer();
}

void BehaviorNetworkObject::WriteSnapshot(unsigned frame, Serializer& dest)
{
    BaseClassName::WriteSnapshot(frame, dest);

    for (const auto& connectedBehavior : behaviors_)
    {
        connectedBehavior.component_->WriteSnapshot(frame, dest);
    }
}

void BehaviorNetworkObject::InitializeFromSnapshot(unsigned frame, Deserializer& src)
{
    BaseClassName::InitializeFromSnapshot(frame, src);

    InitializeBehaviors();

    // TODO(network): Add validation
    for (const auto& connectedBehavior : behaviors_)
        connectedBehavior.component_->InitializeFromSnapshot(frame, src);
}

bool BehaviorNetworkObject::IsRelevantForClient(AbstractConnection* connection)
{
    if (callbackMask_.Test(NetworkCallback::IsRelevantForClient))
    {
        for (const auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallback::IsRelevantForClient))
            {
                if (!connectedBehavior.component_->IsRelevantForClient(connection))
                    return false;
            }
        }
    }
    return true;
}

void BehaviorNetworkObject::UpdateTransformOnServer()
{
    BaseClassName::UpdateTransformOnServer();

    if (callbackMask_.Test(NetworkCallback::UpdateTransformOnServer))
    {
        for (const auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallback::UpdateTransformOnServer))
                connectedBehavior.component_->UpdateTransformOnServer();
        }
    }
}

void BehaviorNetworkObject::InterpolateState(float timeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    BaseClassName::InterpolateState(timeStep, replicaTime, inputTime);

    if (callbackMask_.Test(NetworkCallback::InterpolateState))
    {
        for (const auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallback::InterpolateState))
                connectedBehavior.component_->InterpolateState(timeStep, replicaTime, inputTime);
        }
    }
}

bool BehaviorNetworkObject::PrepareReliableDelta(unsigned frame)
{
    bool needUpdate = BaseClassName::PrepareReliableDelta(frame);

    reliableUpdateMask_ = 0;
    if (callbackMask_.Test(NetworkCallback::ReliableDelta))
    {
        for (auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallback::ReliableDelta))
            {
                if (connectedBehavior.component_->PrepareReliableDelta(frame))
                    reliableUpdateMask_ |= connectedBehavior.bit_;
            }
        }
    }

    needUpdate = needUpdate || reliableUpdateMask_ != 0;
    return needUpdate;
}

void BehaviorNetworkObject::WriteReliableDelta(unsigned frame, Serializer& dest)
{
    BaseClassName::WriteReliableDelta(frame, dest);

    if (callbackMask_.Test(NetworkCallback::ReliableDelta))
    {
        dest.WriteVLE(reliableUpdateMask_);
        for (const auto& connectedBehavior : behaviors_)
        {
            if (reliableUpdateMask_ & connectedBehavior.bit_)
                connectedBehavior.component_->WriteReliableDelta(frame, dest);
        }
    }
}

void BehaviorNetworkObject::ReadReliableDelta(unsigned frame, Deserializer& src)
{
    BaseClassName::ReadReliableDelta(frame, src);

    if (callbackMask_.Test(NetworkCallback::ReliableDelta))
    {
        const unsigned mask = src.ReadVLE();
        for (const auto& connectedBehavior : behaviors_)
        {
            if (mask & connectedBehavior.bit_)
                connectedBehavior.component_->ReadReliableDelta(frame, src);
        }
    }
}

bool BehaviorNetworkObject::PrepareUnreliableDelta(unsigned frame)
{
    bool needUpdate = BaseClassName::PrepareUnreliableDelta(frame);

    unreliableUpdateMask_ = 0;
    if (callbackMask_.Test(NetworkCallback::UnreliableDelta))
    {
        for (auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallback::UnreliableDelta))
            {
                if (connectedBehavior.component_->PrepareUnreliableDelta(frame))
                    unreliableUpdateMask_ |= connectedBehavior.bit_;
            }
        }
    }

    needUpdate = needUpdate || unreliableUpdateMask_ != 0;
    return needUpdate;
}

void BehaviorNetworkObject::WriteUnreliableDelta(unsigned frame, Serializer& dest)
{
    BaseClassName::WriteUnreliableDelta(frame, dest);

    if (callbackMask_.Test(NetworkCallback::UnreliableDelta))
    {
        dest.WriteVLE(unreliableUpdateMask_);
        for (const auto& connectedBehavior : behaviors_)
        {
            if (unreliableUpdateMask_ & connectedBehavior.bit_)
                connectedBehavior.component_->WriteUnreliableDelta(frame, dest);
        }
    }
}

void BehaviorNetworkObject::ReadUnreliableDelta(unsigned frame, Deserializer& src)
{
    BaseClassName::ReadUnreliableDelta(frame, src);

    if (callbackMask_.Test(NetworkCallback::UnreliableDelta))
    {
        const unsigned mask = src.ReadVLE();
        for (const auto& connectedBehavior : behaviors_)
        {
            if (mask & connectedBehavior.bit_)
                connectedBehavior.component_->ReadUnreliableDelta(frame, src);
        }
    }
}

bool BehaviorNetworkObject::PrepareUnreliableFeedback(unsigned frame)
{
    bool needUpdate = BaseClassName::PrepareUnreliableFeedback(frame);

    unreliableFeedbackMask_ = 0;
    if (callbackMask_.Test(NetworkCallback::UnreliableFeedback))
    {
        for (auto& connectedBehavior : behaviors_)
        {
            if (connectedBehavior.callbackMask_.Test(NetworkCallback::UnreliableFeedback))
            {
                if (connectedBehavior.component_->PrepareUnreliableFeedback(frame))
                    unreliableFeedbackMask_ |= connectedBehavior.bit_;
            }
        }
    }

    needUpdate = needUpdate || unreliableFeedbackMask_ != 0;
    return needUpdate;
}

void BehaviorNetworkObject::WriteUnreliableFeedback(unsigned frame, Serializer& dest)
{
    BaseClassName::WriteUnreliableFeedback(frame, dest);

    if (callbackMask_.Test(NetworkCallback::UnreliableFeedback))
    {
        dest.WriteVLE(unreliableFeedbackMask_);
        for (const auto& connectedBehavior : behaviors_)
        {
            if (unreliableFeedbackMask_ & connectedBehavior.bit_)
                connectedBehavior.component_->WriteUnreliableFeedback(frame, dest);
        }
    }
}

void BehaviorNetworkObject::ReadUnreliableFeedback(unsigned feedbackFrame, Deserializer& src)
{
    BaseClassName::ReadUnreliableFeedback(feedbackFrame, src);

    if (callbackMask_.Test(NetworkCallback::UnreliableFeedback))
    {
        const unsigned mask = src.ReadVLE();
        for (const auto& connectedBehavior : behaviors_)
        {
            if (mask & connectedBehavior.bit_)
                connectedBehavior.component_->ReadUnreliableFeedback(feedbackFrame, src);
        }
    }
}

}
