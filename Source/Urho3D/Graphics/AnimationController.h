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

#include <EASTL/span.h>

namespace Urho3D
{

class AnimatedModel;
class Animation;
struct AnimationTriggerPoint;
struct Bone;

/// State and parameters of playing Animation.
struct URHO3D_API AnimationParameters
{
    AnimationParameters() = default;
    explicit AnimationParameters(Animation* animation);
    AnimationParameters(Context* context, const ea::string& animationName);

    /// Factory helpers.
    /// @{
    AnimationParameters& Looped();
    AnimationParameters& StartBone(const ea::string& startBone);
    AnimationParameters& Layer(unsigned layer);
    AnimationParameters& Time(float time);
    AnimationParameters& Additive();
    AnimationParameters& Weight(float weight);
    AnimationParameters& Speed(float speed);
    AnimationParameters& KeepOnCompletion();
    /// @}

    /// Animation to be played.
    /// Animation can be replicated over network only if is exists as named Resource on all machines.
    Animation* animation_{};
    StringHash animationName_;

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
    WrappedScalar<float> time_{0.0f, 0.0f, M_LARGE_VALUE};
    float speed_{1.0f};

    bool removeOnZeroWeight_{};
    float weight_{1.0f};
    float targetWeight_{1.0f};
    float targetWeightDelay_{};
    /// @}

    /// Internal.
    /// @{
    bool removed_{};
    /// @}

    /// Serialization utilities.
    /// @{
    static constexpr unsigned NumVariants = 15;
    static AnimationParameters FromVariantSpan(Context* context, ea::span<const Variant> variants);
    void ToVariantSpan(ea::span<Variant> variants) const;
    /// @}
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

    /// Manage played animations on low level.
    /// @{
    void AddAnimation(const AnimationParameters& params);
    void UpdateAnimation(unsigned index, const AnimationParameters& params);
    void RemoveAnimation(unsigned index);
    unsigned GetNumAnimations() const { return animations_.size(); }
    unsigned GetAnimationLayer(unsigned index) const { return animations_[index].params_.layer_; }
    const AnimationParameters& GetAnimationParameters(unsigned index) const { return animations_[index].params_; }
    /// @}

    /// Manage played animations on high level.
    /// @{
    unsigned FindLastAnimation(Animation* animation) const;
    const AnimationParameters* GetLastAnimationParameters(Animation* animation) const;
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
    bool UpdateAnimationWeight(Animation* animation, float weight);
    bool UpdateAnimationSpeed(Animation* animation, float speed);
    /// @}

    /// Deprecated.
    /// @{
    bool Play(const ea::string& name, unsigned char layer, bool looped, float fadeInTime = 0.0f);
    bool PlayExclusive(const ea::string& name, unsigned char layer, bool looped, float fadeTime = 0.0f);
    bool Stop(const ea::string& name, float fadeOutTime = 0.0f);

    bool SetTime(const ea::string& name, float time);
    bool SetWeight(const ea::string& name, float weight);
    bool SetSpeed(const ea::string& name, float speed);

    bool IsPlaying(const ea::string& name) const;
    float GetTime(const ea::string& name) const;
    float GetWeight(const ea::string& name) const;
    float GetSpeed(const ea::string& name) const;
    /// @}

    /// Set animation parameters attribute.
    void SetAnimationsAttr(const VariantVector& value);
    /// Return animation parameters attribute.
    VariantVector GetAnimationsAttr() const;

    /// Mark animation state order dirty. For internal use only.
    void MarkAnimationStateOrderDirty() { animationStatesDirty_ = true; }

protected:
    /// Handle node being assigned.
    void OnNodeSet(Node* node) override;
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
    bool UpdateAnimationTime(AnimationParameters& params, float timeStep);
    void ApplyAutoFadeOut(AnimationParameters& params) const;
    void UpdateAnimationWeight(AnimationParameters& params, float timeStep) const;
    void ApplyAnimationRemoval(AnimationParameters& params, bool isCompleted) const;
    void CommitAnimationParams(const AnimationParameters& params, AnimationState* state) const;
    /// @}

    /// Currently playing animations.
    struct AnimationInstance
    {
        AnimationParameters params_;
        SharedPtr<AnimationState> state_;
    };
    ea::vector<AnimationInstance> animations_;
    ea::vector<AnimationInstance> animationsSortBuffer_;

    /// Triggers to be fired. Processed all at once due to possible side effects.
    ea::vector<ea::pair<Animation*, const AnimationTriggerPoint*>> pendingTriggers_;

    /// Internal dirty flags cleaned on Update.
    /// @{
    bool animationStatesDirty_{};
    /// @}
};

}
