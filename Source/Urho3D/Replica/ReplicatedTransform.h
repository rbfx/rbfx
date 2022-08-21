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

/// \file

#pragma once

#include "../Replica/BehaviorNetworkObject.h"
#include "../Replica/NetworkValue.h"

namespace Urho3D
{

/// Position-velocity pair, can be used to interpolate and extrapolate object position.
using PositionAndVelocity = ValueWithDerivative<Vector3>;

/// Rotation-velocity pair, can be used to interpolate and extrapolate object rotation.
using RotationAndVelocity = ValueWithDerivative<Quaternion>;

/// Mode of rotation replication.
enum class ReplicatedRotationMode
{
    None,
    XYZ,
    // TODO: Support
    //Y,
};

/// Behavior that replicates transform of the node.
class URHO3D_API ReplicatedTransform : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedTransform, NetworkBehavior);

public:
    static constexpr unsigned DefaultNumUploadAttempts = 8;
    static constexpr float DefaultSmoothingConstant = 15.0f;
    static constexpr float DefaultMovementThreshold = 0.001f;
    static constexpr float DefaultSnapThreshold = 5.0f;
    static constexpr bool DefaultSynchronizePosition = true;
    static constexpr ReplicatedRotationMode DefaultSynchronizeRotation = ReplicatedRotationMode::XYZ;
    static constexpr bool DefaultExtrapolatePosition = true;
    static constexpr bool DefaultExtrapolateRotation = false;

    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallbackMask::UpdateTransformOnServer | NetworkCallbackMask::UnreliableDelta | NetworkCallbackMask::InterpolateState;

    explicit ReplicatedTransform(Context* context);
    ~ReplicatedTransform() override;

    static void RegisterObject(Context* context);

    void SetNumUploadAttempts(unsigned value) { numUploadAttempts_ = value; }
    unsigned GetNumUploadAttempts() const { return numUploadAttempts_; }
    void SetReplicateOwner(bool value) { replicateOwner_ = value; }
    bool GetReplicateOwner() const { return replicateOwner_; }
    void SetPositionTrackOnly(bool value) { positionTrackOnly_ = value; }
    bool GetPositionTrackOnly() const { return positionTrackOnly_; }
    void SetRotationTrackOnly(bool value) { rotationTrackOnly_ = value; }
    bool GetRotationTrackOnly() const { return rotationTrackOnly_; }
    void SetSmoothingConstant(float value) { smoothingConstant_ = value; }
    float GetSmoothingConstant() const { return smoothingConstant_; }
    void SetMovementThreshold(float value) { movementThreshold_ = value; }
    float GetMovementThreshold() const { return movementThreshold_; }
    void SetSnapThreshold(float value) { snapThreshold_ = value; }
    float GetSnapThreshold() const { return snapThreshold_; }

    void SetSynchronizePosition(bool value) { synchronizePosition_ = value; }
    bool GetSynchronizePosition() const { return synchronizePosition_; }
    void SetSynchronizeRotation(ReplicatedRotationMode value) { synchronizeRotation_ = value; }
    ReplicatedRotationMode GetSynchronizeRotation() const { return synchronizeRotation_; }
    void SetExtrapolatePosition(bool value) { extrapolatePosition_ = value; }
    bool GetExtrapolatePosition() const { return extrapolatePosition_; }
    void SetExtrapolateRotation(bool value) { extrapolateRotation_ = value; }
    bool GetExtrapolateRotation() const { return extrapolateRotation_; }

    /// Implement NetworkBehavior.
    /// @{
    void InitializeOnServer() override;
    void WriteSnapshot(NetworkFrame frame, Serializer& dest) override;
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;

    void UpdateTransformOnServer() override;
    void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime) override;

    bool PrepareUnreliableDelta(NetworkFrame frame) override;
    void WriteUnreliableDelta(NetworkFrame frame, Serializer& dest) override;
    void ReadUnreliableDelta(NetworkFrame frame, Deserializer& src) override;
    /// @}

    /// Getters for network properties
    /// @{
    PositionAndVelocity SampleTemporalPosition(const NetworkTime& time) const;
    RotationAndVelocity SampleTemporalRotation(const NetworkTime& time) const;
    ea::optional<PositionAndVelocity> GetTemporalPosition(NetworkFrame frame) const;
    ea::optional<RotationAndVelocity> GetTemporalRotation(NetworkFrame frame) const;
    ea::optional<NetworkFrame> GetLatestFrame() const;
    /// @}

private:
    void InitializeCommon();
    void OnServerFrameEnd(NetworkFrame frame);

    /// Attributes independent on the client and the server.
    /// @{
    unsigned numUploadAttempts_{DefaultNumUploadAttempts};
    bool replicateOwner_{};
    bool positionTrackOnly_{};
    bool rotationTrackOnly_{};
    float smoothingConstant_{DefaultSmoothingConstant};
    float movementThreshold_{DefaultMovementThreshold};
    float snapThreshold_{DefaultSnapThreshold};
    /// @}

    /// Attributes matching on the client and the server.
    /// @{
    bool synchronizePosition_{DefaultSynchronizePosition};
    ReplicatedRotationMode synchronizeRotation_{DefaultSynchronizeRotation};
    bool extrapolatePosition_{DefaultExtrapolatePosition};
    bool extrapolateRotation_{DefaultExtrapolateRotation};
    /// @}

    NetworkValue<PositionAndVelocity> positionTrace_;
    NetworkValue<RotationAndVelocity> rotationTrace_;

    struct ServerData
    {
        unsigned pendingUploadAttempts_{};

        Vector3 previousPosition_;
        Quaternion previousRotation_;

        Vector3 position_;
        Quaternion rotation_;
        Vector3 velocity_;
        Vector3 angularVelocity_;

        bool movedDuringFrame_{};
        Vector3 latestSentPosition_;
        Quaternion latestSentRotation_;
    } server_;

    struct ClientData
    {
        NetworkValueSampler<PositionAndVelocity> positionSampler_;
        NetworkValueSampler<RotationAndVelocity> rotationSampler_;
    } client_;
};

};
