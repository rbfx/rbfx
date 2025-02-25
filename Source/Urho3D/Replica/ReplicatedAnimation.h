// Copyright (c) 2017-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Replica/BehaviorNetworkObject.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

class Animation;
class AnimationController;
class AnimationParameters;

/// Behavior that replicates animation over network.
class URHO3D_API ReplicatedAnimation : public NetworkBehavior
{
    URHO3D_OBJECT(ReplicatedAnimation, NetworkBehavior);

public:
    static constexpr unsigned SmallSnapshotSize = 256;
    static constexpr unsigned DefaultNumUploadAttempts = 4;
    static constexpr float DefaultSmoothingTime = 0.2f;

    static constexpr NetworkCallbackFlags CallbackMask = NetworkCallbackMask::ReliableDelta
        | NetworkCallbackMask::UnreliableDelta | NetworkCallbackMask::InterpolateState
        | NetworkCallbackMask::PostUpdate;

    explicit ReplicatedAnimation(Context* context);
    ~ReplicatedAnimation() override;

    static void RegisterObject(Context* context);

    void SetNumUploadAttempts(unsigned value) { numUploadAttempts_ = value; }
    unsigned GetNumUploadAttempts() const { return numUploadAttempts_; }
    void SetReplicateOwner(bool value) { replicateOwner_ = value; }
    bool GetReplicateOwner() const { return replicateOwner_; }
    void SetSmoothingTime(float value) { smoothingTime_ = value; }
    float GetSmoothingTime() const { return smoothingTime_; }
    void SetLayers(const ea::vector<unsigned>& layers) { layers_ = layers; }
    const ea::vector<unsigned>& GetLayers() const { return layers_; }
    void SetLayersAttr(const VariantVector& layers);
    const VariantVector& GetLayersAttr() const;

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

    void InterpolateState(float replicaTimeStep, float inputTimeStep, const NetworkTime& replicaTime,
        const NetworkTime& inputTime) override;
    void PostUpdate(float replicaTimeStep, float inputTimeStep) override;
    /// @}

private:
    void InitializeCommon();
    void UpdateLookupsOnServer();
    void ReadLookupsOnClient(Deserializer& src);
    void OnServerFrameEnd(NetworkFrame frame);

    using AnimationSnapshot = ea::fixed_vector<unsigned char, SmallSnapshotSize>;

    bool IsAnimationReplicated() const;
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
    ea::vector<unsigned> layers_;
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

}; // namespace Urho3D
