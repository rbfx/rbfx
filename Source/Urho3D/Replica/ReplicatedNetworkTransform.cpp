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
#include "../Network/NetworkEvents.h"
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
    URHO3D_ATTRIBUTE("Smoothing Constant", float, smoothingConstant_, DefaultSmoothingConstant, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Movement Threshold", float, movementThreshold_, DefaultMovementThreshold, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Snap Threshold", float, snapThreshold_, DefaultSnapThreshold, AM_DEFAULT);
    // TODO(network): Configure interpolation and extrapolation too
}

void ReplicatedNetworkTransform::InitializeOnServer()
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

    positionTrace_.Resize(traceDuration);
    rotationTrace_.Resize(traceDuration);

    server_.previousPosition_ = node_->GetWorldPosition();
    server_.previousRotation_ = node_->GetWorldRotation();
    server_.latestSentPosition_ = server_.previousPosition_;
    server_.latestSentRotation_ = server_.previousRotation_;

    SubscribeToEvent(E_ENDSERVERNETWORKFRAME,
        [this](StringHash, VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const unsigned serverFrame = eventData[P_FRAME].GetUInt();
        OnServerFrameEnd(serverFrame);
    });
}

void ReplicatedNetworkTransform::InitializeFromSnapshot(unsigned frame, Deserializer& src)
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();

    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();
    positionTrace_.Resize(traceDuration);
    rotationTrace_.Resize(traceDuration);

    const unsigned updateFrequency = replicationManager->GetUpdateFrequency();
    const float extrapolationInSeconds = replicationManager->GetSetting(NetworkSettings::ExtrapolationLimit).GetFloat();
    const unsigned extrapolationInFrames = CeilToInt(extrapolationInSeconds * updateFrequency);
    client_.positionSampler_.Setup(extrapolationInFrames, smoothingConstant_, snapThreshold_);
    client_.rotationSampler_.Setup(extrapolationInFrames, smoothingConstant_, M_LARGE_VALUE);
}

void ReplicatedNetworkTransform::UpdateTransformOnServer()
{
    server_.movedDuringFrame_ = true;
}

void ReplicatedNetworkTransform::OnServerFrameEnd(unsigned frame)
{
    server_.previousPosition_ = server_.position_;
    server_.previousRotation_ = server_.rotation_;

    server_.position_ = node_->GetWorldPosition();
    server_.rotation_ = node_->GetWorldRotation();

    if (server_.movedDuringFrame_)
    {
        server_.velocity_ = (server_.position_ - server_.previousPosition_);
        server_.angularVelocity_ = (server_.rotation_ * server_.previousRotation_.Inverse()).AngularVelocity();
    }
    else
    {
        server_.velocity_ = Vector3::ZERO;
        server_.angularVelocity_ = Vector3::ZERO;
    }

    positionTrace_.Set(frame, {server_.position_, server_.velocity_});
    rotationTrace_.Set(frame, {server_.rotation_, server_.angularVelocity_});

    if (server_.movedDuringFrame_)
    {
        server_.movedDuringFrame_ = false;

        const float positionErrorSquare = (server_.latestSentPosition_ - server_.position_).LengthSquared();
        const bool isPositionDirty = positionErrorSquare > movementThreshold_ * movementThreshold_;
        const bool isRotationDirty = server_.latestSentRotation_.Equivalent(server_.rotation_, M_LARGE_EPSILON);
        if (isPositionDirty || isRotationDirty)
        {
            server_.pendingUploadAttempts_ = NumUploadAttempts;
            server_.latestSentPosition_ = server_.position_;
            server_.latestSentRotation_ = server_.rotation_;
        }
    }
}

void ReplicatedNetworkTransform::InterpolateState(float timeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (trackOnly_)
        return;

    if (auto newPosition = client_.positionSampler_.UpdateAndSample(positionTrace_, replicaTime, timeStep))
        node_->SetWorldPosition(*newPosition);

    if (auto newRotation = client_.rotationSampler_.UpdateAndSample(rotationTrace_, replicaTime, timeStep))
        node_->SetWorldRotation(*newRotation);
}

bool ReplicatedNetworkTransform::PrepareUnreliableDelta(unsigned frame)
{
    if (server_.pendingUploadAttempts_ > 0)
    {
        --server_.pendingUploadAttempts_;
        return true;
    }
    return false;
}

void ReplicatedNetworkTransform::WriteUnreliableDelta(unsigned frame, Serializer& dest)
{
    dest.WriteVector3(server_.position_);
    dest.WriteVector3(server_.velocity_);
    dest.WriteQuaternion(server_.rotation_);
    dest.WriteVector3(server_.angularVelocity_);
}

void ReplicatedNetworkTransform::ReadUnreliableDelta(unsigned frame, Deserializer& src)
{
    const Vector3 position = src.ReadVector3();
    const Vector3 velocity = src.ReadVector3();
    const Quaternion rotation = src.ReadQuaternion();
    const Vector3 angularVelocity = src.ReadVector3();

    positionTrace_.Set(frame, {position, velocity});
    rotationTrace_.Set(frame, {rotation, angularVelocity});
}

Vector3 ReplicatedNetworkTransform::GetTemporalWorldPosition(const NetworkTime& time) const
{
    return positionTrace_.SampleValid(time).value_;
}

Quaternion ReplicatedNetworkTransform::GetTemporalWorldRotation(const NetworkTime& time) const
{
    return rotationTrace_.SampleValid(time).value_;
}

ea::optional<Vector3> ReplicatedNetworkTransform::GetRawTemporalWorldPosition(unsigned frame) const
{
    const auto value = positionTrace_.GetRaw(frame);
    return value ? ea::make_optional(value->value_) : ea::nullopt;
}

ea::optional<Quaternion> ReplicatedNetworkTransform::GetRawTemporalWorldRotation(unsigned frame) const
{
    const auto value = rotationTrace_.GetRaw(frame);
    return value ? ea::make_optional(value->value_) : ea::nullopt;
}

ea::optional<unsigned> ReplicatedNetworkTransform::GetLatestReceivedFrame() const
{
    return positionTrace_.IsInitialized() ? ea::make_optional(positionTrace_.GetLastFrame()) : ea::nullopt;
}

}
