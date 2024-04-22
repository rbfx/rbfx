//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include "../IO/VectorBuffer.h"
#include "../Math/WrappedScalar.h"
#include "../Scene/Component.h"
#include "../Graphics/AnimationState.h"
#include "../Graphics/AnimationStateSource.h"

#include <EASTL/optional.h>
#include <EASTL/span.h>

namespace Urho3D
{

class AnimatedModel;
class Animation;
struct AnimationTriggerPoint;
struct Bone;

/// State and parameters of playing Animation.
class URHO3D_API AnimationParameters
{
public:
    AnimationParameters() = default;
    explicit AnimationParameters(Animation* animation);
    AnimationParameters(Animation* animation, float minTime, float maxTime);
    AnimationParameters(Context* context, const ea::string& animationName);
    AnimationParameters(Context* context, const ea::string& animationName, float minTime, float maxTime);

    /// Helper utility to fade animation out and remove it later.
    bool RemoveDelayed(float fadeTime);

    /// Factory helpers.
    /// @{
    AnimationParameters& Looped();
    AnimationParameters& StartBone(ea::string_view startBone);
    AnimationParameters& Layer(unsigned layer);
    AnimationParameters& Time(float time);
    AnimationParameters& Additive();
    AnimationParameters& Weight(float weight);
    AnimationParameters& Speed(float speed);
    AnimationParameters& AutoFadeOut(float fadeOut);
    AnimationParameters& KeepOnCompletion();
    AnimationParameters& KeepOnZeroWeight();
    /// @}

    /// Getters for read-only properties.
    /// Use constructor to set Animation to the parameters.
    /// @{
    Animation* GetAnimation() const { return animation_; }
    StringHash GetAnimationName() const { return animationName_; }
    const WrappedScalar<float>& GetAnimationTime() const { return time_; }
    /// @}


    /// Time operations.
    /// @{
    float GetTime() const { return time_.Value(); }
    void SetTime(float time) { time_.Set(time); }
    WrappedScalarRange<float> Update(float scaledTimeStep);
    /// @}

    unsigned instanceIndex_{};

    /// Static animation parameters that change rarely.
    /// @{
    bool looped_{};
    bool removeOnCompletion_{true};
    unsigned layer_{};
    AnimationBlendMode blendMode_{};
    ea::string startBone_;
    float autoFadeOutTime_{};
    /// @}

    /// Dynamic animation parameters that change often.
    /// @{
    float speed_{1.0f};

    bool removeOnZeroWeight_{};
    float weight_{1.0f};
    float targetWeight_{1.0f};
    float targetWeightDelay_{};
    /// @}

    /// Internal flags for easier algorithms. Never stored between API calls.
    bool removed_{};
    bool merged_{};

    /// Serialization, test and misc utilities.
    /// @{
    static constexpr unsigned NumVariants = 15;
    static AnimationParameters FromVariantSpan(Context* context, ea::span<const Variant> variants);
    void ToVariantSpan(ea::span<Variant> variants) const;

    static AnimationParameters Deserialize(Animation* animation, Deserializer& src);
    void Serialize(Serializer& dest) const;

    bool IsMergeableWith(const AnimationParameters& rhs) const;

    bool operator==(const AnimationParameters& rhs) const;
    bool operator!=(const AnimationParameters& rhs) const { return !(*this == rhs); }
    /// @}

    /// Empty AnimationParameters.
    static const AnimationParameters EMPTY;

private:
    /// Animation to be played.
    /// Animation can be replicated over network only if is exists as named Resource on all machines.
    Animation* animation_{};
    StringHash animationName_;
    WrappedScalar<float> time_{0.0f, 0.0f, M_LARGE_VALUE};
};

/// %Component that drives an AnimatedModel's animations.
class URHO3D_API AnimationController : public AnimationStateSource
{
    URHO3D_OBJECT(AnimationController, AnimationStateSource);

public:
    /// Construct.
    explicit AnimationController(Context* context);
    /// Destruct.
    ~AnimationController() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately. Called after scene load or a network update.
    void ApplyAttributes() override;
    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;
    /// Mark that animation state tracks are dirty and should be reconnected.
    /// Should be called on every substantial change in animated structure.
    void MarkAnimationStateTracksDirty() override;

    /// Update the animations. Is called from HandleScenePostUpdate().
    virtual void Update(float timeStep);
    /// Smoothly replace existing animations with animations from external source.
    void ReplaceAnimations(ea::span<const AnimationParameters> newAnimations, float elapsedTime, float fadeTime);

    /// Manage played animations on low level.
    /// @{
    void AddAnimation(const AnimationParameters& params);
    void UpdateAnimation(unsigned index, const AnimationParameters& params);
    void RemoveAnimation(unsigned index);
    void GetAnimationParameters(ea::vector<AnimationParameters>& result) const;
    ea::vector<AnimationParameters> GetAnimationParameters() const;
    unsigned GetNumAnimations() const { return animations_.size(); }
    unsigned GetAnimationLayer(unsigned index) const { return (index < animations_.size()) ? animations_[index].params_.layer_ : 0u; }
    const AnimationParameters& GetAnimationParameters(unsigned index) const;
    unsigned GetRevision() const { return revision_; }
    void UpdatePose();
    /// @}

    /// Manage played animations on high level.
    /// @{
    unsigned FindLastAnimation(Animation* animation, unsigned layer = M_MAX_UNSIGNED) const;
    const AnimationParameters* GetLastAnimationParameters(Animation* animation, unsigned layer = M_MAX_UNSIGNED) const;
    bool IsPlaying(Animation* animation) const;
    unsigned PlayNew(const AnimationParameters& params, float fadeInTime = 0.0f);
    unsigned PlayNewExclusive(const AnimationParameters& params, float fadeInTime = 0.0f);
    unsigned PlayExisting(const AnimationParameters& params, float fadeInTime = 0.0f);
    unsigned PlayExistingExclusive(const AnimationParameters& params, float fadeInTime = 0.0f);
    void Fade(Animation* animation, float targetWeight, float fadeTime);
    void Stop(Animation* animation, float fadeTime = 0.0f);
    void StopLayer(unsigned layer, float fadeTime = 0.0f);
    void StopLayerExcept(unsigned layer, unsigned exceptionIndex, float fadeTime = 0.0f);
    void StopAll(float fadeTime = 0.0f);

    bool UpdateAnimationTime(Animation* animation, float time);
    bool UpdateAnimationWeight(Animation* animation, float weight, float fadeTime = 0.0f);
    bool UpdateAnimationSpeed(Animation* animation, float speed);
    /// @}

    /// Deprecated.
    /// @{
    bool Play(const ea::string& name, unsigned char layer, bool looped, float fadeTime = 0.0f);
    bool PlayExclusive(const ea::string& name, unsigned char layer, bool looped, float fadeTime = 0.0f);
    bool Fade(const ea::string& name, float targetWeight, float fadeTime = 0.0f);
    bool Stop(const ea::string& name, float fadeTime = 0.0f);

    bool SetTime(const ea::string& name, float time);
    bool SetWeight(const ea::string& name, float weight);
    bool SetSpeed(const ea::string& name, float speed);

    bool IsPlaying(const ea::string& name) const;
    float GetTime(const ea::string& name) const;
    float GetWeight(const ea::string& name) const;
    float GetSpeed(const ea::string& name) const;
    /// @}

    bool IsSkeletonReset() const { return resetSkeleton_; }
    void SetSkeletonReset(bool resetSkeleton) { resetSkeleton_ = resetSkeleton; }

    /// Set animation parameters attribute.
    void SetAnimationsAttr(const VariantVector& value);
    /// Return animation parameters attribute.
    VariantVector GetAnimationsAttr() const;

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;

private:
    /// Handle scene post-update event.
    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);
    /// Sort animations states according to the layers.
    void SortAnimationStates();
    /// Update animation state tracks so they are connected to correct animatable objects.
    void UpdateAnimationStateTracks(AnimationState* state);
    /// Connect to AnimatedModel if possible.
    void ConnectToAnimatedModel();
    /// Parse animatable path from starting node.
    AnimatedAttributeReference ParseAnimatablePath(ea::string_view path, Node* startNode);
    /// Get animated node (bone or owner node) by track name hash
    Node* GetTrackNodeByNameHash(StringHash trackNameHash, Node* startBone) const;
    /// Deprecated. Get animation by resource name.
    Animation* GetAnimationByName(const ea::string& name) const;

    /// Update single animation.
    /// @{
    void UpdateInstance(AnimationParameters& params, float timeStep, bool fireTriggers);
    ea::optional<float> UpdateInstanceTime(AnimationParameters& params, float timeStep, bool fireTriggers);
    void ApplyInstanceAutoFadeOut(AnimationParameters& params, float overtime) const;
    void UpdateInstanceWeight(AnimationParameters& params, float timeStep) const;
    void ApplyInstanceRemoval(AnimationParameters& params, bool isCompleted) const;

    void EnsureStateInitialized(AnimationState* state, const AnimationParameters& params);
    void UpdateState(AnimationState* state, const AnimationParameters& params) const;
    /// @}

    void CommitNodeAndAttributeAnimations();
    void SendTriggerEvents();

    /// Whether to reset AnimatedModel skeleton to bind pose every frame.
    bool resetSkeleton_{};

    /// Currently playing animations.
    struct AnimationInstance
    {
        AnimationParameters params_;
        SharedPtr<AnimationState> state_;
    };
    ea::vector<AnimationInstance> animations_;
    ea::vector<AnimationInstance> tempAnimations_;

    /// Triggers to be fired. Processed all at once due to possible side effects.
    ea::vector<ea::pair<Animation*, const AnimationTriggerPoint*>> pendingTriggers_;
    /// Revision that tracks substantial updates of animation state.
    unsigned revision_{};

    /// Internal dirty flags cleaned on Update.
    /// @{
    bool animationStatesDirty_{};
    bool revisionDirty_{};
    /// @}

    /// Temporary buffers for animated values.
    /// TODO: Nodes may be expired for unused elements!
    /// TODO: Revisit allocations?
    ea::unordered_map<Node*, NodeAnimationOutput> animatedNodes_;
    ea::unordered_map<AnimatedAttributeReference, Variant> animatedAttributes_;
};

}
