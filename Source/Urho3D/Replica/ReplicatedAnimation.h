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

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

class Animation;
class AnimationController;
class AnimationParameters;

/// Behavior that replicates animation over network.
/// TODO: This behavior doesn't really replicate any animation now, it only does essential setup on the server.
class URHO3D_API ReplicatedAnimation : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedAnimation, NetworkBehavior);

public:
    static constexpr unsigned SmallSnapshotSize = 256;
    static constexpr unsigned DefaultNumUploadAttempts = 4;
    static constexpr float DefaultSmoothingTime = 0.2f;

    static constexpr NetworkCallbackFlags CallbackMask =
        NetworkCallbackMask::ReliableDelta | NetworkCallbackMask::UnreliableDelta | NetworkCallbackMask::InterpolateState | NetworkCallbackMask::Update;

    explicit ReplicatedAnimation(Context* context);
    ~ReplicatedAnimation() override;

    static void RegisterObject(Context* context);

    void SetNumUploadAttempts(unsigned value) { numUploadAttempts_ = value; }
    unsigned GetNumUploadAttempts() const { return numUploadAttempts_; }
    void SetReplicateOwner(bool value) { replicateOwner_ = value; }
    bool GetReplicateOwner() const { return replicateOwner_; }
    void SetSmoothingTime(float value) { smoothingTime_ = value; }
    float GetSmoothingTime() const { return smoothingTime_; }

    const StringMap& GetAnimationLookup() const { return animationLookup_; }

    /// Implement NetworkBehavior.
    /// @{
    void InitializeStandalone() override;
    void InitializeOnServer() override;
    void WriteSnapshot(NetworkFrame frame, Serializer& dest) override;
    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;

    bool PrepareReliableDelta(NetworkFrame frame) override;
    void WriteReliableDelta(NetworkFrame frame, Serializer& dest) override;
    void ReadReliableDelta(NetworkFrame frame, Deserializer& src) override;

    bool PrepareUnreliableDelta(NetworkFrame frame) override;
    void WriteUnreliableDelta(NetworkFrame frame, Serializer& dest) override;
    void ReadUnreliableDelta(NetworkFrame frame, Deserializer& src) override;

    void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime, const NetworkTime& inputTime) override;
    void Update(float replicaTimeStep, float inputTimeStep) override;
    /// @}

private:
    void InitializeCommon();
    void UpdateLookupsOnServer();
    void ReadLookupsOnClient(Deserializer& src);
    void OnServerFrameEnd(NetworkFrame frame);

    using AnimationSnapshot = ea::fixed_vector<unsigned char, SmallSnapshotSize>;

    Animation* GetAnimationByHash(StringHash nameHash) const;
    void WriteSnapshot(Serializer& dest);
    AnimationSnapshot ReadSnapshot(Deserializer& src) const;
    void DecodeSnapshot(const AnimationSnapshot& snapshot, ea::vector<AnimationParameters>& result) const;

    WeakPtr<AnimationController> animationController_;

    /// Attributes independent on the client and the server.
    /// @{
    unsigned numUploadAttempts_{DefaultNumUploadAttempts};
    bool replicateOwner_{};
    float smoothingTime_{DefaultSmoothingTime};
    /// @}

    StringMap animationLookup_;

    struct ServerData
    {
        unsigned pendingUploadAttempts_{};
        unsigned latestRevision_{};
        ea::vector<ea::string> newAnimationLookups_;
        VectorBuffer snapshotBuffer_;
    } server_;

    struct ClientData
    {
        float networkStepTime_{};

        ea::optional<NetworkFrame> latestAppliedFrame_;
        ea::vector<AnimationParameters> snapshotAnimations_;

        NetworkValue<AnimationSnapshot> animationTrace_;
    } client_;
};

};
