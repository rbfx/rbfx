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

/// Behavior that replicates transform of the node.
class URHO3D_API ReplicatedNetworkTransform : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedNetworkTransform, NetworkBehavior);

public:
    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallback::UpdateTransformOnServer | NetworkCallback::UnreliableDelta | NetworkCallback::InterpolateState;
    static const unsigned NumUploadAttempts = 8;

    explicit ReplicatedNetworkTransform(Context* context);
    ~ReplicatedNetworkTransform() override;

    static void RegisterObject(Context* context);

    void SetTrackOnly(bool value) { trackOnly_ = value; }
    bool GetTrackOnly() const { return trackOnly_; }

    /// Implement NetworkBehavior.
    /// @{
    void InitializeOnServer() override;
    void InitializeFromSnapshot(unsigned frame, Deserializer& src) override;

    void UpdateTransformOnServer() override;
    void InterpolateState(const NetworkTime& replicaTime, const NetworkTime& inputTime) override;

    bool PrepareUnreliableDelta(unsigned frame) override;
    void WriteUnreliableDelta(unsigned frame, Serializer& dest) override;
    void ReadUnreliableDelta(unsigned frame, Deserializer& src) override;
    /// @}

    /// Getters for network properties
    /// @{
    Vector3 GetTemporalWorldPosition(const NetworkTime& time) const { return worldPositionTrace_.SampleValid(time); }
    Quaternion GetTemporalWorldRotation(const NetworkTime& time) const { return worldRotationTrace_.SampleValid(time); }
    ea::optional<Vector3> GetRawTemporalWorldPosition(unsigned frame) const { return worldPositionTrace_.GetRaw(frame); }
    ea::optional<Quaternion> GetRawTemporalWorldRotation(unsigned frame) const { return worldRotationTrace_.GetRaw(frame); }
    /// @}

private:
    bool trackOnly_{};

    unsigned pendingUploadAttempts_{};

    NetworkValue<Vector3> worldPositionTrace_;
    NetworkValue<Quaternion> worldRotationTrace_;
};

};
