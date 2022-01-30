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
#include "../Replica/ReplicatedNetworkTransform.h"
#include "../Replica/NetworkSettingsConsts.h"

namespace Urho3D
{

ReplicatedNetworkTransform::ReplicatedNetworkTransform(Context* context)
    : NetworkBehavior(context, CallbackMask)
{
}

ReplicatedNetworkTransform::~ReplicatedNetworkTransform()
{
}

void ReplicatedNetworkTransform::RegisterObject(Context* context)
{
    context->RegisterFactory<ReplicatedNetworkTransform>();

    URHO3D_COPY_BASE_ATTRIBUTES(NetworkBehavior);
    URHO3D_ATTRIBUTE("Track Only", bool, trackOnly_, false, AM_DEFAULT);
}

void ReplicatedNetworkTransform::InitializeOnServer()
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

    worldPositionTrace_.Resize(traceDuration);
    worldRotationTrace_.Resize(traceDuration);
}

void ReplicatedNetworkTransform::InitializeFromSnapshot(unsigned frame, Deserializer& src)
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

    worldPositionTrace_.Resize(traceDuration);
    worldRotationTrace_.Resize(traceDuration);
}

void ReplicatedNetworkTransform::UpdateTransformOnServer()
{
    pendingUploadAttempts_ = NumUploadAttempts;
}

void ReplicatedNetworkTransform::InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (trackOnly_)
        return;

    const NetworkManager* replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned updateFrequency = replicationManager->GetUpdateFrequency();
    const float extrapolationInSeconds = replicationManager->GetSetting(NetworkSettings::ExtrapolationDuration).GetFloat();
    const unsigned extrapolationInFrames = CeilToInt(extrapolationInSeconds * updateFrequency);

    if (auto newWorldPosition = worldPositionTrace_.ReconstructAndSample(replicaTime, {extrapolationInFrames}))
        node_->SetWorldPosition(*newWorldPosition);

    if (auto newWorldRotation = worldRotationTrace_.ReconstructAndSample(replicaTime, {extrapolationInFrames}))
        node_->SetWorldRotation(*newWorldRotation);
}

bool ReplicatedNetworkTransform::PrepareUnreliableDelta(unsigned frame)
{
    worldPositionTrace_.Set(frame, node_->GetWorldPosition());
    worldRotationTrace_.Set(frame, node_->GetWorldRotation());
    if (pendingUploadAttempts_ > 0)
    {
        --pendingUploadAttempts_;
        return true;
    }
    return false;
}

void ReplicatedNetworkTransform::WriteUnreliableDelta(unsigned frame, Serializer& dest)
{
    dest.WriteVector3(node_->GetWorldPosition());
    dest.WriteQuaternion(node_->GetWorldRotation());
}

void ReplicatedNetworkTransform::ReadUnreliableDelta(unsigned frame, Deserializer& src)
{
    worldPositionTrace_.Set(frame, src.ReadVector3());
    worldRotationTrace_.Set(frame, src.ReadQuaternion());
}

}
