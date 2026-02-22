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

#include "Urho3D/IO/IODefs.h"
#include "Urho3D/Replica/BehaviorNetworkObject.h"
#include "Urho3D/Replica/NetworkValue.h"

namespace Urho3D
{

/// Position-velocity pair, can be used to interpolate and extrapolate object position.
using PositionAndVelocity = ValueWithDerivative<DoubleVector3>;

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

    static constexpr VectorBinaryEncoding DefaultPositionEncoding = VectorBinaryEncoding::Float;
    static constexpr VectorBinaryEncoding DefaultRotationEncoding = VectorBinaryEncoding::Float;
    static constexpr VectorBinaryEncoding DefaultVelocityEncoding = VectorBinaryEncoding::Float;
    static constexpr VectorBinaryEncoding DefaultAngularVelocityEncoding = VectorBinaryEncoding::Float;
    static constexpr float DefaultPositionEncodingParameter = 1024000.0f;
    static constexpr float DefaultVelocityEncodingParameter = 100.0f;
    static constexpr float DefaultAngularVelocityEncodingParameter = 100.0f;

    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallbackMask::UpdateTransformOnServer | NetworkCallbackMask::UnreliableDelta | NetworkCallbackMask::InterpolateState;

public:
    explicit ReplicatedTransform(Context* context);
    ~ReplicatedTransform() override;

    static void RegisterObject(Context* context);

    /// Attributes.
    /// @{
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

    void SetPositionEncoding(VectorBinaryEncoding encoding) { positionEncoding_ = encoding; }
    VectorBinaryEncoding GetPositionEncoding() const { return positionEncoding_; }
    void SetRotationEncoding(VectorBinaryEncoding encoding) { rotationEncoding_ = encoding; }
    VectorBinaryEncoding GetRotationEncoding() const { return rotationEncoding_; }
    void SetVelocityEncoding(VectorBinaryEncoding encoding) { velocityEncoding_ = encoding; }
    VectorBinaryEncoding GetVelocityEncoding() const { return velocityEncoding_; }
    void SetAngularVelocityEncoding(VectorBinaryEncoding encoding) { angularVelocityEncoding_ = encoding; }
    VectorBinaryEncoding GetAngularVelocityEncoding() const { return angularVelocityEncoding_; }

    void SetPositionEncodingParameter(float value) { positionEncodingParameter_ = value; }
    float GetPositionEncodingParameter() const { return positionEncodingParameter_; }
    void SetVelocityEncodingParameter(float value) { velocityEncodingParameter_ = value; }
    float GetVelocityEncodingParameter() const { return velocityEncodingParameter_; }
    void SetAngularVelocityEncodingParameter(float value) { angularVelocityEncodingParameter_ = value; }
    float GetAngularVelocityEncodingParameter() const { return angularVelocityEncodingParameter_; }
    /// @}

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

    void WriteUnreliableDeltaForFrame(NetworkFrame frame, Serializer& dest);
    void ReadUnreliableDeltaForFrame(NetworkFrame frame, Deserializer& src);

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

    /// Attributes matching on the client and the server. Replicated automatically.
    /// @{
    bool synchronizePosition_{DefaultSynchronizePosition};
    ReplicatedRotationMode synchronizeRotation_{DefaultSynchronizeRotation};
    bool extrapolatePosition_{DefaultExtrapolatePosition};
    bool extrapolateRotation_{DefaultExtrapolateRotation};
    bool includePreviousFrame_{}; // Deduced from numUploadAttempts_
    /// @}

    /// Attributes matching on the client and the server.
    /// TODO: Replicate automatically.
    /// @{
    VectorBinaryEncoding positionEncoding_{DefaultPositionEncoding};
    VectorBinaryEncoding rotationEncoding_{DefaultRotationEncoding};
    VectorBinaryEncoding velocityEncoding_{DefaultVelocityEncoding};
    VectorBinaryEncoding angularVelocityEncoding_{DefaultAngularVelocityEncoding};

    float positionEncodingParameter_{DefaultPositionEncodingParameter};
    float velocityEncodingParameter_{DefaultVelocityEncodingParameter};
    float angularVelocityEncodingParameter_{DefaultAngularVelocityEncodingParameter};
    /// @}

    NetworkValue<PositionAndVelocity> positionTrace_;
    NetworkValue<RotationAndVelocity> rotationTrace_;

    struct ServerData
    {
        unsigned pendingUploadAttempts_{};

        DoubleVector3 previousPosition_;
        Quaternion previousRotation_;

        DoubleVector3 position_;
        Quaternion rotation_;
        DoubleVector3 velocity_;
        Vector3 angularVelocity_;

        bool movedDuringFrame_{};
        DoubleVector3 latestSentPosition_;
        Quaternion latestSentRotation_;
    } server_;

    struct ClientData
    {
        NetworkValueSampler<PositionAndVelocity> positionSampler_;
        NetworkValueSampler<RotationAndVelocity> rotationSampler_;

        bool previousPositionInvalid_{};
        bool previousRotationInvalid_{};
    } client_;
};

};
