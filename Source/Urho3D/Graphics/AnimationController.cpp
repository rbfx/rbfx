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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationController.h"
#include "../Graphics/AnimationState.h"
#include "../Graphics/DrawableEvents.h"
#include "../Graphics/Renderer.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

#include <EASTL/sort.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

const StringVector animationParametersNames =
{
    "Animation Count",
    "   Animation",
    "   Is Looped",
    "   Remove On Completion",
    "   Layer",
    "   Blend Mode",
    "   Start Bone",
    "   Auto Fade Out Time",
    "   Time",
    "   Min Time",
    "   Max Time",
    "   Speed",
    "   Remove On Zero Weight",
    "   Weight",
    "   Target Weight",
    "   Target Weight Delay",
};

enum class AnimationParameterMask
{
    InstanceIndex       = 1 << 0,
    Looped              = 1 << 1,
    RemoveOnCompletion  = 1 << 2,
    Layer               = 1 << 3,
    Additive            = 1 << 4,
    StartBone           = 1 << 5,
    AutoFadeOutTime     = 1 << 6,
    Time                = 1 << 7,
    MinTime             = 1 << 8,
    MaxTime             = 1 << 9,
    Speed               = 1 << 10,
    RemoveOnZeroWeight  = 1 << 11,
    Weight              = 1 << 12,
    TargetWeight        = 1 << 13,
    TargetWeightDelay   = 1 << 14
};
URHO3D_FLAGSET(AnimationParameterMask, AnimationParameterFlags);

}

AnimationParameters::AnimationParameters(Animation* animation)
    : animation_(animation)
    , animationName_(animation ? animation_->GetNameHash() : StringHash::Empty)
    , time_{0.0f, 0.0f, animation_ ? animation_->GetLength() : M_LARGE_VALUE}
{
}

AnimationParameters::AnimationParameters(Context* context, const ea::string& animationName)
    : AnimationParameters(context->GetSubsystem<ResourceCache>()->GetResource<Animation>(animationName))
{
}

bool AnimationParameters::RemoveDelayed(float fadeTime)
{
    const bool alreadyRemoved = Equals(targetWeight_, 0.0f, M_LARGE_EPSILON);
    const float effectiveDelay = alreadyRemoved ? ea::min(targetWeightDelay_, fadeTime) : fadeTime;

    const bool changed = !removeOnZeroWeight_ || targetWeightDelay_ != effectiveDelay || targetWeight_ != 0.0f;

    removeOnZeroWeight_ = true;
    targetWeightDelay_ = effectiveDelay;
    targetWeight_ = 0.0f;

    return changed;
}

AnimationParameters& AnimationParameters::Looped()
{
    looped_ = true;
    return *this;
}

AnimationParameters& AnimationParameters::StartBone(const ea::string& startBone)
{
    startBone_ = startBone;
    return *this;
}

AnimationParameters& AnimationParameters::Layer(unsigned layer)
{
    layer_ = layer;
    return *this;
}

AnimationParameters& AnimationParameters::Time(float time)
{
    time_.Set(time);
    return *this;
}

AnimationParameters& AnimationParameters::Additive()
{
    blendMode_ = ABM_ADDITIVE;
    return *this;
}

AnimationParameters& AnimationParameters::Weight(float weight)
{
    weight_ = weight;
    targetWeight_ = weight;
    return *this;
}

AnimationParameters& AnimationParameters::Speed(float speed)
{
    speed_ = speed;
    return *this;
}

AnimationParameters& AnimationParameters::AutoFadeOut(float fadeOut)
{
    autoFadeOutTime_ = fadeOut;
    return *this;
}

AnimationParameters& AnimationParameters::KeepOnCompletion()
{
    removeOnCompletion_ = false;
    return *this;
}

AnimationParameters& AnimationParameters::KeepOnZeroWeight()
{
    removeOnZeroWeight_ = false;
    return *this;
}

AnimationParameters AnimationParameters::FromVariantSpan(Context* context, ea::span<const Variant> variants)
{
    AnimationParameters result;
    unsigned index = 0;

    const ea::string animationName = variants[index++].GetResourceRef().name_;
    result.animation_ = context->GetSubsystem<ResourceCache>()->GetResource<Animation>(animationName);
    result.animationName_ = result.animation_ ? result.animation_->GetNameHash() : StringHash::Empty;

    result.looped_ = variants[index++].GetBool();
    result.removeOnCompletion_ = variants[index++].GetBool();
    result.layer_ = variants[index++].GetUInt();
    result.blendMode_ = static_cast<AnimationBlendMode>(variants[index++].GetUInt());
    result.startBone_ = variants[index++].GetString();
    result.autoFadeOutTime_ = variants[index++].GetFloat();

    const float time = variants[index++].GetFloat();
    const float minTime = variants[index++].GetFloat();
    const float maxTime = variants[index++].GetFloat();
    const float effectiveMaxTime = maxTime == M_LARGE_VALUE && result.animation_ ? result.animation_->GetLength() : maxTime;
    result.time_ = {time, minTime, effectiveMaxTime};

    result.speed_ = variants[index++].GetFloat();
    result.removeOnZeroWeight_ = variants[index++].GetBool();

    result.weight_ = variants[index++].GetFloat();
    result.targetWeight_ = variants[index++].GetFloat();
    result.targetWeightDelay_ = variants[index++].GetFloat();

    URHO3D_ASSERT(index == NumVariants);
    return result;
}

void AnimationParameters::ToVariantSpan(ea::span<Variant> variants) const
{
    unsigned index = 0;

    variants[index++] = ResourceRef{Animation::GetTypeStatic(), animation_ ? animation_->GetName() : EMPTY_STRING};

    variants[index++] = looped_;
    variants[index++] = removeOnCompletion_;
    variants[index++] = layer_;
    variants[index++] = blendMode_;
    variants[index++] = startBone_;
    variants[index++] = autoFadeOutTime_;

    variants[index++] = time_.Value();
    variants[index++] = time_.Min();
    variants[index++] = time_.Max();

    variants[index++] = speed_;
    variants[index++] = removeOnZeroWeight_;

    variants[index++] = weight_;
    variants[index++] = targetWeight_;
    variants[index++] = targetWeightDelay_;

    URHO3D_ASSERT(index == NumVariants);
}

AnimationParameters AnimationParameters::Deserialize(Animation* animation, Deserializer& src)
{
    AnimationParameters result{animation};

    const auto flags = static_cast<AnimationParameterFlags>(src.ReadVLE());

    if (flags.Test(AnimationParameterMask::InstanceIndex))
        result.instanceIndex_ = src.ReadVLE();

    result.looped_ = flags.Test(AnimationParameterMask::Looped);

    result.removeOnCompletion_ = flags.Test(AnimationParameterMask::RemoveOnCompletion);

    if (flags.Test(AnimationParameterMask::Layer))
        result.layer_ = src.ReadVLE();

    result.blendMode_ = flags.Test(AnimationParameterMask::Additive) ? ABM_ADDITIVE : ABM_LERP;

    if (flags.Test(AnimationParameterMask::StartBone))
        result.startBone_ = src.ReadString();

    if (flags.Test(AnimationParameterMask::AutoFadeOutTime))
        result.autoFadeOutTime_ = src.ReadFloat();

    float time = 0.0f;
    float minTime = 0.0f;
    float maxTime = result.animation_ ? result.animation_->GetLength() : 0.0f;

    if (flags.Test(AnimationParameterMask::Time))
        time = src.ReadFloat();
    if (flags.Test(AnimationParameterMask::MinTime))
        minTime = src.ReadFloat();
    if (flags.Test(AnimationParameterMask::MaxTime))
        maxTime = src.ReadFloat();

    result.time_ = {time, minTime, maxTime};

    if (flags.Test(AnimationParameterMask::Speed))
        result.speed_ = src.ReadFloat();

    result.removeOnZeroWeight_ = flags.Test(AnimationParameterMask::RemoveOnZeroWeight);

    if (flags.Test(AnimationParameterMask::Weight))
        result.weight_ = src.ReadFloat();

    if (flags.Test(AnimationParameterMask::TargetWeight))
        result.targetWeight_ = src.ReadFloat();

    if (flags.Test(AnimationParameterMask::TargetWeightDelay))
        result.targetWeightDelay_ = src.ReadFloat();

    return result;
}

void AnimationParameters::Serialize(Serializer& dest) const
{
    AnimationParameterFlags flags;
    flags.Set(AnimationParameterMask::InstanceIndex, instanceIndex_ != 0);
    flags.Set(AnimationParameterMask::Looped, looped_);
    flags.Set(AnimationParameterMask::RemoveOnCompletion, removeOnCompletion_);
    flags.Set(AnimationParameterMask::Layer, layer_ != 0);
    flags.Set(AnimationParameterMask::Additive, blendMode_ == ABM_ADDITIVE);
    flags.Set(AnimationParameterMask::StartBone, startBone_.empty());
    flags.Set(AnimationParameterMask::AutoFadeOutTime, autoFadeOutTime_ != 0.0f);
    flags.Set(AnimationParameterMask::Time, time_.Value() != 0.0f);
    flags.Set(AnimationParameterMask::MinTime, time_.Min() != 0.0f);
    flags.Set(AnimationParameterMask::MaxTime, animation_ && time_.Max() != animation_->GetLength());
    flags.Set(AnimationParameterMask::Speed, speed_ != 1.0f);
    flags.Set(AnimationParameterMask::RemoveOnZeroWeight, removeOnZeroWeight_);
    flags.Set(AnimationParameterMask::Weight, weight_ != 1.0f);
    flags.Set(AnimationParameterMask::TargetWeight, targetWeight_ != 1.0f);
    flags.Set(AnimationParameterMask::TargetWeightDelay, targetWeightDelay_ != 0.0f);

    dest.WriteVLE(flags.AsInteger());
    if (flags.Test(AnimationParameterMask::InstanceIndex))
        dest.WriteVLE(instanceIndex_);
    if (flags.Test(AnimationParameterMask::Layer))
        dest.WriteVLE(layer_);
    if (flags.Test(AnimationParameterMask::StartBone))
        dest.WriteString(startBone_);
    if (flags.Test(AnimationParameterMask::AutoFadeOutTime))
        dest.WriteFloat(autoFadeOutTime_);
    if (flags.Test(AnimationParameterMask::Time))
        dest.WriteFloat(time_.Value());
    if (flags.Test(AnimationParameterMask::MinTime))
        dest.WriteFloat(time_.Min());
    if (flags.Test(AnimationParameterMask::MaxTime))
        dest.WriteFloat(time_.Max());
    if (flags.Test(AnimationParameterMask::Speed))
        dest.WriteFloat(speed_);
    if (flags.Test(AnimationParameterMask::Weight))
        dest.WriteFloat(weight_);
    if (flags.Test(AnimationParameterMask::TargetWeight))
        dest.WriteFloat(targetWeight_);
    if (flags.Test(AnimationParameterMask::TargetWeightDelay))
        dest.WriteFloat(targetWeightDelay_);
}

bool AnimationParameters::IsMergeableWith(const AnimationParameters& rhs) const
{
    return animation_ == rhs.animation_ && instanceIndex_ == rhs.instanceIndex_;
}

bool AnimationParameters::operator==(const AnimationParameters& rhs) const
{
    return animation_ == rhs.animation_
        && animationName_ == rhs.animationName_
        && instanceIndex_ == rhs.instanceIndex_
        && looped_ == rhs.looped_
        && removeOnCompletion_ == rhs.removeOnCompletion_
        && layer_ == rhs.layer_
        && blendMode_ == rhs.blendMode_
        && startBone_ == rhs.startBone_
        && autoFadeOutTime_ == rhs.autoFadeOutTime_
        && time_ == rhs.time_
        && speed_ == rhs.speed_
        && removeOnZeroWeight_ == rhs.removeOnZeroWeight_
        && weight_ == rhs.weight_
        && targetWeight_ == rhs.targetWeight_
        && targetWeightDelay_ == rhs.targetWeightDelay_;
}

AnimationController::AnimationController(Context* context) :
    AnimationStateSource(context)
{
}

AnimationController::~AnimationController() = default;

void AnimationController::RegisterObject(Context* context)
{
    context->AddFactoryReflection<AnimationController>(Category_Logic);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Reset Skeleton", bool, resetSkeleton_, false, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Animations", GetAnimationsAttr, SetAnimationsAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT)
        .SetMetadata(AttributeMetadata::VectorStructElements, animationParametersNames);
}

void AnimationController::ApplyAttributes()
{
    ConnectToAnimatedModel();
}

void AnimationController::OnSetEnabled()
{
    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(AnimationController, HandleScenePostUpdate));
        else
            UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
    }
}

void AnimationController::Update(float timeStep)
{
    // Update stats
    if (auto renderer = GetSubsystem<Renderer>())
    {
        FrameStatistics& stats = renderer->GetMutableFrameStats();
        ++stats.animations_;
        if (revisionDirty_)
            ++stats.changedAnimations_;
    }

    // Update revision
    if (revisionDirty_)
    {
        ++revision_;
        revisionDirty_ = false;
    }

    // Update individual animations
    pendingTriggers_.clear();
    for (auto& [params, state] : animations_)
    {
        if (!params.animation_)
            continue;
        UpdateInstance(params, timeStep, true);
        if (!params.removed_)
            UpdateState(state, params);
    }
    ea::erase_if(animations_, [](const AnimationInstance& value) { return value.params_.removed_; });

    // Sort animation states if necessary
    if (animationStatesDirty_)
        SortAnimationStates();

    // Update animation tracks if necessary
    for (AnimationState* state : animationStates_)
    {
        if (state->AreTracksDirty())
            UpdateAnimationStateTracks(state);
    }

    // Reset skeleton if necessary
    if (resetSkeleton_)
    {
        if (auto model = GetComponent<AnimatedModel>())
            model->GetSkeleton().Reset();
    }

    // Node and attribute animations need to be applied manually
    CommitNodeAndAttributeAnimations();
}

void AnimationController::CommitNodeAndAttributeAnimations()
{
    for (auto& [node, value] : animatedNodes_)
        value.dirty_ = {};
    for (auto& [attribute, value] : animatedAttributes_)
        value = Variant::EMPTY;

    for (AnimationState* state : animationStates_)
    {
        state->CalculateNodeTracks(animatedNodes_);
        state->CalculateAttributeTracks(animatedAttributes_);
    }

    for (const auto& [node, value] : animatedNodes_)
    {
        if (value.dirty_)
        {
            if (value.dirty_.Test(CHANNEL_POSITION))
                node->SetPosition(value.localToParent_.position_);
            if (value.dirty_.Test(CHANNEL_ROTATION))
                node->SetRotation(value.localToParent_.rotation_);
            if (value.dirty_.Test(CHANNEL_SCALE))
                node->SetScale(value.localToParent_.scale_);
        }
    }
    for (const auto& [attribute, value] : animatedAttributes_)
    {
        if (value.IsEmpty())
            continue;
        attribute.SetValue(value);
    }
}

void AnimationController::SendTriggerEvents()
{
    for (const auto& [animation, trigger] : pendingTriggers_)
    {
        using namespace AnimationTrigger;

        VariantMap& eventData = node_->GetEventDataMap();
        eventData[P_NODE] = node_;
        eventData[P_ANIMATION] = animation;
        eventData[P_NAME] = animation->GetAnimationName();
        eventData[P_TIME] = trigger->time_;
        eventData[P_DATA] = trigger->data_;

        node_->SendEvent(E_ANIMATIONTRIGGER, eventData);
    }
}

void AnimationController::ReplaceAnimations(ea::span<const AnimationParameters> newAnimations, float elapsedTime, float fadeTime)
{
    // Upload new parameters and correct them according to elapsed time, some states may be removed
    const unsigned numNewAnimations = newAnimations.size();
    tempAnimations_.clear();
    tempAnimations_.resize(numNewAnimations);

    for (unsigned i = 0; i < numNewAnimations; ++i)
    {
        tempAnimations_[i].params_ = newAnimations[i];
        UpdateInstance(tempAnimations_[i].params_, elapsedTime, false);
    }

    // Try to match old animation states for all remaining states
    for (unsigned i = 0; i < numNewAnimations; ++i)
    {
        AnimationParameters& newParams = tempAnimations_[i].params_;
        if (newParams.removed_)
            continue;

        const auto oldIter = ea::find_if(animations_.begin(), animations_.end(),
            [&](const AnimationInstance& instance) { return !instance.params_.merged_ && instance.params_.IsMergeableWith(newParams); });

        if (oldIter != animations_.end())
        {
            tempAnimations_[i].state_ = oldIter->state_;
            oldIter->params_.merged_ = true;

            // Keep original weight for smooth transition when merged
            newParams.weight_ = oldIter->params_.weight_;
            // Keep original delay if weight didn't change, extend delay up to minimum value otherwise
            const bool weightChanged = !Equals(newParams.targetWeight_, oldIter->params_.targetWeight_, M_LARGE_EPSILON);
            newParams.targetWeightDelay_ = weightChanged ? ea::max(newParams.targetWeightDelay_, fadeTime) : oldIter->params_.targetWeightDelay_;
        }
    }

    // Remove all merged states from the animations, and fade out unmerged animations
    ea::erase_if(animations_, [](const AnimationInstance& instance) { return instance.params_.merged_; });
    for (AnimationInstance& instance : animations_)
        instance.params_.RemoveDelayed(fadeTime);

    // Append new animations
    for (const AnimationInstance& instance : tempAnimations_)
    {
        if (instance.params_.removed_)
            continue;

        if (!instance.state_)
        {
            PlayNew(instance.params_, fadeTime);
        }
        else
        {
            EnsureStateInitialized(instance.state_, instance.params_);
            animations_.push_back(instance);
        }
    }

    animationStatesDirty_ = true;
    revisionDirty_ = true;
}

void AnimationController::AddAnimation(const AnimationParameters& params)
{
    const unsigned instanceIndex = ea::count_if(animations_.begin(), animations_.end(),
        [&](const AnimationInstance& instance) { return instance.params_.animation_ == params.animation_; });

    AnimationInstance& instance = animations_.emplace_back();
    instance.params_ = params;
    instance.params_.instanceIndex_ = instanceIndex;

    auto* model = GetComponent<AnimatedModel>();
    instance.state_ = model ? MakeShared<AnimationState>(this, model) : MakeShared<AnimationState>(this, node_);
    EnsureStateInitialized(instance.state_, instance.params_);

    animationStatesDirty_ = true;
    revisionDirty_ = true;
}

void AnimationController::UpdateAnimation(unsigned index, const AnimationParameters& params)
{
    if (index >= animations_.size())
    {
        URHO3D_ASSERTLOG(0, "Out-of-bounds animation index in AnimationController::UpdateAnimation");
        return;
    }

    AnimationInstance& instance = animations_[index];
    if (instance.params_.animation_ != params.animation_ || instance.params_.instanceIndex_ != params.instanceIndex_)
    {
        URHO3D_ASSERTLOG(0, "Animation and instance index cannot be changed by AnimationController::UpdateAnimation");
        return;
    }

    EnsureStateInitialized(instance.state_, params);
    if (instance.params_.layer_ != params.layer_)
        animationStatesDirty_ = true;

    if (instance.params_ != params)
    {
        instance.params_ = params;
        revisionDirty_ = true;
    }
}

void AnimationController::RemoveAnimation(unsigned index)
{
    if (index >= animations_.size())
    {
        URHO3D_ASSERTLOG(0, "Out-of-bounds animation index in AnimationController::UpdateAnimation");
        return;
    }

    animations_.erase_at(index);
    animationStatesDirty_ = true;
    revisionDirty_ = true;
}

void AnimationController::GetAnimationParameters(ea::vector<AnimationParameters>& result) const
{
    result.clear();
    for (const AnimationInstance& instance : animations_)
        result.push_back(instance.params_);
}

ea::vector<AnimationParameters> AnimationController::GetAnimationParameters() const
{
    ea::vector<AnimationParameters> params;
    GetAnimationParameters(params);
    return params;
}

unsigned AnimationController::FindLastAnimation(Animation* animation) const
{
    const auto iter = ea::find_if(animations_.rbegin(), animations_.rend(),
        [&](const AnimationInstance& value) { return value.params_.animation_ == animation; });
    return iter != animations_.rend() ? (iter.base() - animations_.begin()) - 1 : M_MAX_UNSIGNED;
}

const AnimationParameters* AnimationController::GetLastAnimationParameters(Animation* animation) const
{
    const unsigned index = FindLastAnimation(animation);
    return index != M_MAX_UNSIGNED ? &animations_[index].params_ : nullptr;
}

bool AnimationController::IsPlaying(Animation* animation) const
{
    return FindLastAnimation(animation) != M_MAX_UNSIGNED;
}

unsigned AnimationController::PlayNew(const AnimationParameters& params, float fadeInTime)
{
    AnimationParameters adjustedParams = params;
    if (fadeInTime > 0.0f)
    {
        adjustedParams.weight_ = 0.0f;
        adjustedParams.targetWeightDelay_ = fadeInTime;
    }
    AddAnimation(adjustedParams);
    return animations_.size() - 1;
}

unsigned AnimationController::PlayNewExclusive(const AnimationParameters& params, float fadeInTime)
{
    const unsigned index = PlayNew(params, fadeInTime);
    StopLayerExcept(params.layer_, index, fadeInTime);
    return index;
}

unsigned AnimationController::PlayExisting(const AnimationParameters& params, float fadeInTime)
{
    const unsigned index = FindLastAnimation(params.animation_);
    if (index == M_MAX_UNSIGNED)
    {
        PlayNew(params, fadeInTime);
        return animations_.size() - 1;
    }

    AnimationParameters existingParams = GetAnimationParameters(index);
    existingParams.speed_ = params.speed_;
    existingParams.removeOnZeroWeight_ = params.removeOnZeroWeight_;
    if (existingParams.targetWeight_ != params.targetWeight_)
    {
        existingParams.targetWeight_ = params.targetWeight_;
        existingParams.targetWeightDelay_ = fadeInTime;
    }
    UpdateAnimation(index, existingParams);
    return index;
}

unsigned AnimationController::PlayExistingExclusive(const AnimationParameters& params, float fadeInTime)
{
    const unsigned index = PlayExisting(params, fadeInTime);
    StopLayerExcept(params.layer_, index, fadeInTime);
    return index;
}

void AnimationController::Fade(Animation* animation, float targetWeight, float fadeTime)
{
    const unsigned index = FindLastAnimation(animation);
    if (index != M_MAX_UNSIGNED)
    {
        AnimationParameters params = GetAnimationParameters(index);
        params.targetWeight_ = targetWeight;
        params.targetWeightDelay_ = fadeTime;
        UpdateAnimation(index, params);
    }
}

void AnimationController::Stop(Animation* animation, float fadeTime)
{
    const unsigned index = FindLastAnimation(animation);
    if (index != M_MAX_UNSIGNED)
    {
        AnimationParameters params = GetAnimationParameters(index);
        if (params.RemoveDelayed(fadeTime))
            UpdateAnimation(index, params);
    }
}

void AnimationController::StopLayer(unsigned layer, float fadeTime)
{
    for (unsigned index = 0; index < animations_.size(); ++index)
    {
        const AnimationInstance& instance = animations_[index];
        if (instance.params_.layer_ != layer)
            continue;

        AnimationParameters params = GetAnimationParameters(index);
        if (params.RemoveDelayed(fadeTime))
            UpdateAnimation(index, params);
    }
}

void AnimationController::StopLayerExcept(unsigned layer, unsigned exceptionIndex, float fadeTime)
{
    for (unsigned index = 0; index < animations_.size(); ++index)
    {
        const AnimationInstance& instance = animations_[index];
        if (instance.params_.layer_ != layer || index == exceptionIndex)
            continue;

        AnimationParameters params = GetAnimationParameters(index);
        if (params.RemoveDelayed(fadeTime))
            UpdateAnimation(index, params);
    }
}

void AnimationController::StopAll(float fadeTime)
{
    for (unsigned index = 0; index < animations_.size(); ++index)
    {
        AnimationParameters params = GetAnimationParameters(index);
        if (params.RemoveDelayed(fadeTime))
            UpdateAnimation(index, params);
    }
}

bool AnimationController::UpdateAnimationTime(Animation* animation, float time)
{
    const unsigned index = FindLastAnimation(animation);
    if (index != M_MAX_UNSIGNED)
    {
        AnimationParameters params = GetAnimationParameters(index);
        params.time_.Set(time);
        UpdateAnimation(index, params);
        return true;
    }
    return false;
}

bool AnimationController::UpdateAnimationWeight(Animation* animation, float weight, float fadeTime)
{
    const unsigned index = FindLastAnimation(animation);
    if (index != M_MAX_UNSIGNED)
    {
        AnimationParameters params = GetAnimationParameters(index);
        if (fadeTime == 0.0f)
            params.weight_ = weight;
        params.targetWeight_ = weight;
        params.targetWeightDelay_ = fadeTime;
        UpdateAnimation(index, params);
        return true;
    }
    return false;
}

bool AnimationController::UpdateAnimationSpeed(Animation* animation, float speed)
{
    const unsigned index = FindLastAnimation(animation);
    if (index != M_MAX_UNSIGNED)
    {
        AnimationParameters params = GetAnimationParameters(index);
        params.speed_ = speed;
        UpdateAnimation(index, params);
        return true;
    }
    return false;
}

bool AnimationController::Play(const ea::string& name, unsigned char layer, bool looped, float fadeTime)
{
    Animation* animation = GetAnimationByName(name);
    if (!animation)
        return false;

    auto params = AnimationParameters{animation}.Layer(layer);
    params.removeOnZeroWeight_ = true;
    params.looped_ = looped;
    PlayExisting(params, fadeTime);
    return true;
}

bool AnimationController::PlayExclusive(const ea::string& name, unsigned char layer, bool looped, float fadeTime)
{
    Animation* animation = GetAnimationByName(name);
    if (!animation)
        return false;

    auto params = AnimationParameters{animation}.Layer(layer);
    params.removeOnZeroWeight_ = true;
    params.looped_ = looped;
    PlayExistingExclusive(params, fadeTime);
    return true;
}

bool AnimationController::Fade(const ea::string& name, float targetWeight, float fadeTime)
{
    Animation* animation = GetAnimationByName(name);
    if (!animation)
        return false;

    Fade(animation, targetWeight, fadeTime);
    return true;
}

bool AnimationController::Stop(const ea::string& name, float fadeTime)
{
    Animation* animation = GetAnimationByName(name);
    if (!animation)
        return false;

    Stop(animation, fadeTime);
    return true;
}

bool AnimationController::SetTime(const ea::string& name, float time)
{
    Animation* animation = GetAnimationByName(name);
    return animation && UpdateAnimationTime(animation, time);
}

bool AnimationController::SetSpeed(const ea::string& name, float speed)
{
    Animation* animation = GetAnimationByName(name);
    return animation && UpdateAnimationSpeed(animation, speed);
}

bool AnimationController::SetWeight(const ea::string& name, float weight)
{
    Animation* animation = GetAnimationByName(name);
    return animation && UpdateAnimationWeight(animation, weight);
}

bool AnimationController::IsPlaying(const ea::string& name) const
{
    Animation* animation = GetAnimationByName(name);
    return animation && IsPlaying(animation);
}

float AnimationController::GetTime(const ea::string& name) const
{
    Animation* animation = GetAnimationByName(name);
    const AnimationParameters* params = GetLastAnimationParameters(animation);
    return params ? params->time_.Value() : 0.0f;
}

float AnimationController::GetWeight(const ea::string& name) const
{
    Animation* animation = GetAnimationByName(name);
    const AnimationParameters* params = GetLastAnimationParameters(animation);
    return params ? params->weight_ : 0.0f;
}

float AnimationController::GetSpeed(const ea::string& name) const
{
    Animation* animation = GetAnimationByName(name);
    const AnimationParameters* params = GetLastAnimationParameters(animation);
    return params ? params->speed_ : 0.0f;
}

void AnimationController::SetAnimationsAttr(const VariantVector& value)
{
    animations_.clear();
    revisionDirty_ = true;
    if (value.empty() || value[0].GetType() != VAR_INT)
        return;

    const auto numAnimations = static_cast<unsigned>(ea::max(0, value[0].GetInt()));
    const unsigned numAnimationsSet = (value.size() - 1) / AnimationParameters::NumVariants;
    const unsigned numLoadedAnimations = ea::min(numAnimations, numAnimationsSet);

    const ea::span<const Variant> valueSpan{value};
    for (unsigned i = 0; i < numLoadedAnimations; ++i)
    {
        const auto span = valueSpan.subspan(1 + i * AnimationParameters::NumVariants, AnimationParameters::NumVariants);
        const auto params = AnimationParameters::FromVariantSpan(context_, span);
        AddAnimation(params);
    }
    for (unsigned i = numLoadedAnimations; i < numAnimations; ++i)
        AddAnimation(AnimationParameters{nullptr});
}

VariantVector AnimationController::GetAnimationsAttr() const
{
    const unsigned numAnimations = animations_.size();

    VariantVector ret;
    ret.resize(1 + numAnimations * AnimationParameters::NumVariants);
    ret[0] = numAnimations;

    const ea::span<Variant> valueSpan{ret};
    for (unsigned i = 0; i < numAnimations; ++i)
    {
        const auto span = valueSpan.subspan(1 + i * AnimationParameters::NumVariants, AnimationParameters::NumVariants);
        animations_[i].params_.ToVariantSpan(span);
    }

    return ret;
}

void AnimationController::OnNodeSet(Node* previousNode, Node* currentNode)
{
    ConnectToAnimatedModel();
}

void AnimationController::OnSceneSet(Scene* scene)
{
    if (scene && IsEnabledEffective())
        SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(AnimationController, HandleScenePostUpdate));
    else if (!scene)
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
}

void AnimationController::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace ScenePostUpdate;

    Update(eventData[P_TIMESTEP].GetFloat());
}

void AnimationController::MarkAnimationStateTracksDirty()
{
    for (AnimationState* state : animationStates_)
        state->MarkTracksDirty();
}

AnimatedAttributeReference AnimationController::ParseAnimatablePath(ea::string_view path, Node* startNode)
{
    if (path.empty())
    {
        URHO3D_LOGWARNING("Variant animation track name must not be empty");
        return {};
    }

    Node* animatedNode = startNode;
    ea::string_view attributePath = path;

    // Resolve path to node if necessary
    if (path[0] != '@')
    {
        const auto sep = path.find("/@");
        if (sep == ea::string_view::npos)
        {
            URHO3D_LOGWARNING("Path must end with attribute reference like /@StaticModel/Model");
            return {};
        }

        const ea::string_view nodePath = path.substr(0, sep);
        animatedNode = startNode->FindChild(nodePath);
        if (!animatedNode)
        {
            URHO3D_LOGWARNING("Path to node '{}' cannot be resolved", nodePath);
            return {};
        }

        attributePath = path.substr(sep + 1);
    }

    AnimatedAttributeReference result;

    // Special cases:
    // 1) if Node variables are referenced, individual variables are supported
    // 2) if AnimatedModel morphs are referenced, individual morphs are supported
    static const ea::string_view variablesPath = "@/Variables/";
    static const ea::string_view animatedModelPath = "@AnimatedModel";
    if (attributePath.starts_with(variablesPath))
    {
        const StringHash variableNameHash = attributePath.substr(variablesPath.size());
        result.attributeType_ = AnimatedAttributeType::NodeVariables;
        result.subAttributeKey_ = variableNameHash.Value();
        attributePath = attributePath.substr(0, variablesPath.size() - 1);
    }
    else if (attributePath.starts_with(animatedModelPath))
    {
        if (const auto sep1 = attributePath.find('/'); sep1 != ea::string_view::npos)
        {
            if (const auto sep2 = attributePath.find('/', sep1 + 1); sep2 != ea::string_view::npos)
            {
                // TODO(string): Refactor StringUtils
                result.attributeType_ = AnimatedAttributeType::AnimatedModelMorphs;
                result.subAttributeKey_ = ToUInt(ea::string(attributePath.substr(sep2 + 1)));
                attributePath = attributePath.substr(0, sep2);
            }
        }
    }

    // Parse path to component and attribute
    const auto& [serializable, attributeIndex] = animatedNode->FindComponentAttribute(attributePath);
    if (!serializable)
    {
        URHO3D_LOGWARNING("Path to attribute '{}' cannot be resolved", attributePath);
        return {};
    }

    result.serializable_ = serializable;
    result.attributeIndex_ = attributeIndex;
    return result;
}

Node* AnimationController::GetTrackNodeByNameHash(StringHash trackNameHash, Node* startBone) const
{
    // Empty track name means that we animate the node itself
    if (!trackNameHash)
        return node_;

    // If track name hash matches start bone name hash then we animate the start bone
    if (trackNameHash == startBone->GetNameHash())
        return startBone;

    return startBone->GetChild(trackNameHash, true);
}

Animation* AnimationController::GetAnimationByName(const ea::string& name) const
{
    auto cache = GetSubsystem<ResourceCache>();
    return cache->GetResource<Animation>(name);
}

void AnimationController::UpdateInstance(AnimationParameters& params, float timeStep, bool fireTriggers)
{
    const auto overtime = UpdateInstanceTime(params, timeStep, fireTriggers);
    const bool isCompleted = overtime.has_value();
    UpdateInstanceWeight(params, timeStep);

    if (isCompleted)
        ApplyInstanceAutoFadeOut(params, *overtime);
    ApplyInstanceRemoval(params, isCompleted);
}

ea::optional<float> AnimationController::UpdateInstanceTime(AnimationParameters& params, float timeStep, bool fireTriggers)
{
    const float scaledTimeStep = timeStep * params.speed_;
    if (scaledTimeStep == 0.0f)
        return ea::nullopt;

    const auto delta = params.looped_
        ? params.time_.UpdateWrapped(scaledTimeStep)
        : params.time_.UpdateClamped(scaledTimeStep, true);

    const float positiveOvertime = delta.End() - delta.Max();
    const float negativeOvertime = delta.End() - delta.Min();
    const bool isCompleted = !params.looped_ && (params.speed_ > 0 ? positiveOvertime >= 0 : negativeOvertime <= 0);

    const float scaledOvertime = isCompleted ? (params.speed_ > 0 ? positiveOvertime : negativeOvertime) : 0.0f;

    if (fireTriggers)
    {
        const float length = params.animation_->GetLength();
        for (const AnimationTriggerPoint& trigger : params.animation_->GetTriggers())
        {
            if (delta.ContainsExcludingBegin(ea::min(trigger.time_, length)))
                pendingTriggers_.emplace_back(params.animation_, &trigger);
        }
    }

    return isCompleted ? ea::make_optional(scaledOvertime / params.speed_) : ea::nullopt;
}

void AnimationController::ApplyInstanceAutoFadeOut(AnimationParameters& params, float overtime) const
{
    // Apply auto fade out only if enabled and if not fading out already
    if (params.targetWeight_ == 0.0f || params.autoFadeOutTime_ == 0.0f)
        return;

    params.targetWeight_ = 0.0f;
    params.targetWeightDelay_ = ea::max(0.0f, params.autoFadeOutTime_ - overtime);
    params.removeOnZeroWeight_ = true;

    const float fraction = ea::min(1.0f, overtime / params.autoFadeOutTime_);
    params.weight_ = Lerp(params.weight_, 0.0f, fraction);
}

void AnimationController::UpdateInstanceWeight(AnimationParameters& params, float timeStep) const
{
    if (params.targetWeightDelay_ <= 0.0f)
    {
        params.weight_ = params.targetWeight_;
        return;
    }

    const float fraction = ea::min(1.0f, timeStep / params.targetWeightDelay_);
    params.weight_ = Lerp(params.weight_, params.targetWeight_, fraction);
    params.targetWeightDelay_ = ea::max(0.0f, params.targetWeightDelay_ - timeStep);
}

void AnimationController::ApplyInstanceRemoval(AnimationParameters& params, bool isCompleted) const
{
    if (params.removeOnCompletion_ && params.autoFadeOutTime_ == 0.0f && isCompleted)
        params.removed_ = true;
    if (params.removeOnZeroWeight_ && params.weight_ < M_LARGE_EPSILON)
        params.removed_ = true;
}

void AnimationController::EnsureStateInitialized(AnimationState* state, const AnimationParameters& params)
{
    state->Initialize(params.animation_, params.startBone_, params.blendMode_);
}

void AnimationController::UpdateState(AnimationState* state, const AnimationParameters& params) const
{
    state->Update(params.looped_, params.time_.Value(), params.weight_);
}

void AnimationController::SortAnimationStates()
{
    const unsigned numAnimations = GetNumAnimations();
    animationStates_.resize(numAnimations);
    if (numAnimations == 0)
        return;

    tempAnimations_.resize(numAnimations * 2);
    ea::copy(animations_.begin(), animations_.end(), tempAnimations_.begin());

    const auto beginIter = tempAnimations_.begin();
    const auto endIter = ea::next(tempAnimations_.begin(), numAnimations);
    ea::merge_sort_buffer(beginIter, endIter, &tempAnimations_[numAnimations],
        [](const AnimationInstance& lhs, const AnimationInstance& rhs) { return lhs.params_.layer_ < rhs.params_.layer_; });
    ea::transform(beginIter, endIter, animationStates_.begin(),
        [](const AnimationInstance& value) { return value.state_; });

    animationStatesDirty_ = false;
}

void AnimationController::UpdateAnimationStateTracks(AnimationState* state)
{
    // TODO(animation): Cache lookups?
    auto model = GetComponent<AnimatedModel>();
    const Animation* animation = state->GetAnimation();

    // Reset internal state. Shouldn't cause any reallocations due to simple vectors inside.
    state->ClearAllTracks();

    if (!animation)
    {
        state->OnTracksReady();
        return;
    }

    // Use root node or start bone node if specified
    // Note: bone tracks are resolved starting from root bone node,
    // variant tracks are resolved from the node itself.
    const ea::string& startBoneName = state->GetStartBone();
    Node* startNode = !startBoneName.empty() ? node_->GetChild(startBoneName, true) : nullptr;
    if (!startNode)
        startNode = node_;

    // Setup model and node tracks
    const auto& tracks = animation->GetTracks();
    for (const auto& item : tracks)
    {
        const AnimationTrack& track = item.second;

        // Try to find bone first, filter by start bone node
        const unsigned trackBoneIndex = model ? model->GetSkeleton().GetBoneIndex(track.nameHash_) : M_MAX_UNSIGNED;
        Bone* trackBone = trackBoneIndex != M_MAX_UNSIGNED ? model->GetSkeleton().GetBone(trackBoneIndex) : nullptr;
        if (trackBone && trackBone->node_ && (startNode == node_ || trackBone->node_->IsChildOf(startNode)))
        {
            ModelAnimationStateTrack stateTrack;
            stateTrack.track_ = &track;
            stateTrack.boneIndex_ = trackBoneIndex;
            stateTrack.node_ = trackBone->node_;
            stateTrack.bone_ = trackBone;
            state->AddModelTrack(stateTrack);
            continue;
        }

        // Find stray node otherwise
        Node* trackNode = GetTrackNodeByNameHash(track.nameHash_, startNode);
        if (trackNode)
        {
            NodeAnimationStateTrack stateTrack;
            stateTrack.track_ = &track;
            stateTrack.node_ = trackNode;
            state->AddNodeTrack(stateTrack);
        }
    }

    // Setup generic tracks
    const auto& variantTracks = animation->GetVariantTracks();
    for (const auto& item : variantTracks)
    {
        const VariantAnimationTrack& track = item.second;
        const ea::string& trackName = track.name_;

        AttributeAnimationStateTrack stateTrack;
        stateTrack.track_ = &track;
        stateTrack.attribute_ = ParseAnimatablePath(trackName, startNode);

        if (stateTrack.attribute_.serializable_)
        {
            state->AddAttributeTrack(stateTrack);
        }
    }

    // Mark tracks as ready
    state->OnTracksReady();
}

void AnimationController::ConnectToAnimatedModel()
{
    auto model = GetComponent<AnimatedModel>();
    if (model)
        model->ConnectToAnimationStateSource(this);
}

}
