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
#include "../Network/NetworkEvents.h"
#include "../Replica/ReplicatedTransform.h"
#include "../Replica/NetworkSettingsConsts.h"

namespace Urho3D
{

namespace
{

const StringVector replicatedRotationModeNames = {
    "None",
    "XYZ",
    //"Y"
};

}

ReplicatedTransform::ReplicatedTransform(Context* context)
    : NetworkBehavior(context, CallbackMask)
{
}

ReplicatedTransform::~ReplicatedTransform()
{
}

void ReplicatedTransform::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ReplicatedTransform>(Category_Network);

    URHO3D_COPY_BASE_ATTRIBUTES(NetworkBehavior);

    URHO3D_ATTRIBUTE("Num Upload Attempts", unsigned, numUploadAttempts_, DefaultNumUploadAttempts, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Replicate Owner", bool, replicateOwner_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Position Track Only", bool, positionTrackOnly_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation Track Only", bool, rotationTrackOnly_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Smoothing Constant", float, smoothingConstant_, DefaultSmoothingConstant, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Movement Threshold", float, movementThreshold_, DefaultMovementThreshold, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Snap Threshold", float, snapThreshold_, DefaultSnapThreshold, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Synchronize Position", bool, synchronizePosition_, DefaultSynchronizePosition, AM_DEFAULT);
    URHO3D_ENUM_ATTRIBUTE("Synchronize Rotation", synchronizeRotation_, replicatedRotationModeNames, DefaultSynchronizeRotation, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Extrapolate Position", bool, extrapolatePosition_, DefaultExtrapolatePosition, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Extrapolate Rotation", bool, extrapolateRotation_, DefaultExtrapolateRotation, AM_DEFAULT);
}

void ReplicatedTransform::InitializeOnServer()
{
    InitializeCommon();

    server_.previousPosition_ = node_->GetWorldPosition();
    server_.previousRotation_ = node_->GetWorldRotation();
    server_.latestSentPosition_ = server_.previousPosition_;
    server_.latestSentRotation_ = server_.previousRotation_;

    SubscribeToEvent(E_ENDSERVERNETWORKFRAME,
        [this](VariantMap& eventData)
    {
        using namespace BeginServerNetworkFrame;
        const auto serverFrame = static_cast<NetworkFrame>(eventData[P_FRAME].GetInt64());
        OnServerFrameEnd(serverFrame);
    });
}

void ReplicatedTransform::WriteSnapshot(NetworkFrame frame, Serializer& dest)
{
    ea::bitset<32> flags;
    flags[0] = synchronizePosition_;
    flags[1] = synchronizeRotation_ != ReplicatedRotationMode::None;
    flags[2] = extrapolatePosition_;
    flags[3] = extrapolateRotation_;
    dest.WriteVLE(flags.to_uint32());
}

void ReplicatedTransform::InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned)
{
    InitializeCommon();

    ea::bitset<32> flags = src.ReadVLE();
    synchronizePosition_ = flags[0];
    synchronizeRotation_ = flags[1] ? ReplicatedRotationMode::XYZ : ReplicatedRotationMode::None;
    extrapolatePosition_ = flags[2];
    extrapolateRotation_ = flags[3];

    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned updateFrequency = replicationManager->GetUpdateFrequency();
    const float extrapolationInSeconds = replicationManager->GetSetting(NetworkSettings::ExtrapolationLimit).GetFloat();
    const unsigned extrapolationInFrames = CeilToInt(extrapolationInSeconds * updateFrequency);
    client_.positionSampler_.Setup(extrapolatePosition_ ? extrapolationInFrames : 0, smoothingConstant_, snapThreshold_);
    client_.rotationSampler_.Setup(extrapolateRotation_ ? extrapolationInFrames : 0, smoothingConstant_, M_LARGE_VALUE);
}

void ReplicatedTransform::UpdateTransformOnServer()
{
    server_.movedDuringFrame_ = true;
}

void ReplicatedTransform::InitializeCommon()
{
    const auto replicationManager = GetNetworkObject()->GetReplicationManager();
    const unsigned traceDuration = replicationManager->GetTraceDurationInFrames();

    positionTrace_.Resize(traceDuration);
    rotationTrace_.Resize(traceDuration);
}

void ReplicatedTransform::OnServerFrameEnd(NetworkFrame frame)
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

    if (server_.pendingUploadAttempts_ > 0)
        --server_.pendingUploadAttempts_;

    if (server_.movedDuringFrame_)
    {
        server_.movedDuringFrame_ = false;

        const float positionErrorSquare = (server_.latestSentPosition_ - server_.position_).LengthSquared();
        const bool isPositionDirty = positionErrorSquare > movementThreshold_ * movementThreshold_;
        const bool isRotationDirty = !server_.latestSentRotation_.Equivalent(server_.rotation_, M_LARGE_EPSILON);
        if (isPositionDirty || isRotationDirty)
        {
            server_.pendingUploadAttempts_ = numUploadAttempts_;
            server_.latestSentPosition_ = server_.position_;
            server_.latestSentRotation_ = server_.rotation_;
        }
    }
}

void ReplicatedTransform::InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime)
{
    if (!replicateOwner_ && GetNetworkObject()->IsOwnedByThisClient())
        return;

    if (!positionTrackOnly_ && synchronizePosition_)
    {
        if (auto newPosition = client_.positionSampler_.UpdateAndSample(positionTrace_, replicaTime, replicaTimeStep))
            node_->SetWorldPosition(*newPosition);
    }

    if (!rotationTrackOnly_ && synchronizeRotation_ != ReplicatedRotationMode::None)
    {
        if (auto newRotation = client_.rotationSampler_.UpdateAndSample(rotationTrace_, replicaTime, replicaTimeStep))
            node_->SetWorldRotation(*newRotation);
    }
}

bool ReplicatedTransform::PrepareUnreliableDelta(NetworkFrame frame)
{
    return server_.pendingUploadAttempts_ > 0 || numUploadAttempts_ == 0;
}

void ReplicatedTransform::WriteUnreliableDelta(NetworkFrame frame, Serializer& dest)
{
    if (synchronizePosition_)
    {
        dest.WriteVector3(server_.position_);
        dest.WriteVector3(server_.velocity_);
    }

    if (synchronizeRotation_ == ReplicatedRotationMode::XYZ)
    {
        dest.WriteQuaternion(server_.rotation_);
        dest.WriteVector3(server_.angularVelocity_);
    }
}

void ReplicatedTransform::ReadUnreliableDelta(NetworkFrame frame, Deserializer& src)
{
    if (synchronizePosition_)
    {
        const Vector3 position = src.ReadVector3();
        const Vector3 velocity = src.ReadVector3();

        positionTrace_.Set(frame, {position, velocity});
    }

    if (synchronizeRotation_ == ReplicatedRotationMode::XYZ)
    {
        const Quaternion rotation = src.ReadQuaternion();
        const Vector3 angularVelocity = src.ReadVector3();

        rotationTrace_.Set(frame, {rotation, angularVelocity});
    }
}

PositionAndVelocity ReplicatedTransform::SampleTemporalPosition(const NetworkTime& time) const
{
    return positionTrace_.SampleValid(time);
}

RotationAndVelocity ReplicatedTransform::SampleTemporalRotation(const NetworkTime& time) const
{
    return rotationTrace_.SampleValid(time);
}

ea::optional<PositionAndVelocity> ReplicatedTransform::GetTemporalPosition(NetworkFrame frame) const
{
    return positionTrace_.GetRaw(frame);
}

ea::optional<RotationAndVelocity> ReplicatedTransform::GetTemporalRotation(NetworkFrame frame) const
{
    return rotationTrace_.GetRaw(frame);
}

ea::optional<NetworkFrame> ReplicatedTransform::GetLatestFrame() const
{
    return positionTrace_.IsInitialized() ? ea::make_optional(positionTrace_.GetLastFrame()) : ea::nullopt;
}

}
