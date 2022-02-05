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

/// Behavior that replicates transform of the node.
class URHO3D_API ReplicatedNetworkTransform : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedNetworkTransform, NetworkBehavior);

public:
    static constexpr float DefaultSmoothingConstant = 15.0f;
    static constexpr float DefaultMovementThreshold = 0.001f;
    static constexpr float DefaultSnapThreshold = 5.0f;
    static const unsigned NumUploadAttempts = 8;

    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallbackMask::UpdateTransformOnServer | NetworkCallbackMask::UnreliableDelta | NetworkCallbackMask::InterpolateState;

    explicit ReplicatedNetworkTransform(Context* context);
    ~ReplicatedNetworkTransform() override;

    static void RegisterObject(Context* context);

    void SetTrackOnly(bool value) { trackOnly_ = value; }
    bool GetTrackOnly() const { return trackOnly_; }
    void SetSmoothingConstant(float value) { smoothingConstant_ = value; }
    float GetSmoothingConstant() const { return smoothingConstant_; }
    void SetMovementThreshold(float value) { movementThreshold_ = value; }
    float GetMovementThreshold() const { return movementThreshold_; }
    void SetSnapThreshold(float value) { snapThreshold_ = value; }
    float GetSnapThreshold() const { return snapThreshold_; }

    /// Implement NetworkBehavior.
    /// @{
    void InitializeOnServer() override;
    void InitializeFromSnapshot(unsigned frame, Deserializer& src) override;

    void UpdateTransformOnServer() override;
    void InterpolateState(float timeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime) override;

    bool PrepareUnreliableDelta(unsigned frame) override;
    void WriteUnreliableDelta(unsigned frame, Serializer& dest) override;
    void ReadUnreliableDelta(unsigned frame, Deserializer& src) override;
    /// @}

    /// Getters for network properties
    /// @{
    Vector3 GetTemporalWorldPosition(const NetworkTime& time) const;
    Quaternion GetTemporalWorldRotation(const NetworkTime& time) const;
    ea::optional<Vector3> GetRawTemporalWorldPosition(unsigned frame) const;
    ea::optional<Quaternion> GetRawTemporalWorldRotation(unsigned frame) const;
    ea::optional<unsigned> GetLatestReceivedFrame() const;
    /// @}

private:
    void OnServerFrameEnd(unsigned frame);

    /// Attributes independent on the client and the server.
    /// @{
    bool trackOnly_{};
    float smoothingConstant_{DefaultSmoothingConstant};
    float movementThreshold_{DefaultMovementThreshold};
    float snapThreshold_{DefaultSnapThreshold};
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
