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

bool CompareLayers(const SharedPtr<AnimationState>& lhs, const SharedPtr<AnimationState>& rhs)
{
    return lhs->GetLayer() < rhs->GetLayer();
}

}

static const unsigned char CTRL_LOOPED = 0x1;
static const unsigned char CTRL_STARTBONE = 0x2;
static const unsigned char CTRL_AUTOFADE = 0x4;
static const unsigned char CTRL_SETTIME = 0x08;
static const unsigned char CTRL_SETWEIGHT = 0x10;
static const unsigned char CTRL_REMOVEONCOMPLETION = 0x20;
static const unsigned char CTRL_ADDITIVE = 0x40;
static const float EXTRA_ANIM_FADEOUT_TIME = 0.1f;
static const float COMMAND_STAY_TIME = 0.25f;
static const unsigned MAX_NODE_ANIMATION_STATES = 256;

extern const char* LOGIC_CATEGORY;

AnimationParameters::AnimationParameters(Animation* animation)
    : animation_(animation)
    , animationName_(animation ? animation_->GetNameHash() : StringHash{})
    , time_{0.0f, 0.0f, animation_ ? animation_->GetLength() : 0.0f}
{
}

AnimationParameters::AnimationParameters(Context* context, const ea::string& animationName)
    : AnimationParameters(context->GetSubsystem<ResourceCache>()->GetResource<Animation>(animationName))
{
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

AnimationParameters& AnimationParameters::KeepOnCompletion()
{
    removeOnCompletion_ = false;
    return *this;
}

AnimationParameters AnimationParameters::FromVariantSpan(Context* context, ea::span<const Variant> variants)
{
    AnimationParameters result;
    unsigned index = 0;

    const ea::string animationName = variants[index++].GetString();
    result.animation_ = context->GetSubsystem<ResourceCache>()->GetResource<Animation>(animationName);
    result.animationName_ = result.animation_ ? result.animation_->GetNameHash() : StringHash{};

    result.looped_ = variants[index++].GetBool();
    result.removeOnCompletion_ = variants[index++].GetBool();
    result.layer_ = variants[index++].GetUInt();
    result.blendMode_ = static_cast<AnimationBlendMode>(variants[index++].GetUInt());
    result.startBone_ = variants[index++].GetString();
    result.autoFadeOutTime_ = variants[index++].GetFloat();

    const float time = variants[index++].GetFloat();
    const float minTime = variants[index++].GetFloat();
    const float maxTime = variants[index++].GetFloat();
    result.time_ = {time, minTime, maxTime};

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

    variants[index++] = animation_ ? animation_->GetName() : EMPTY_STRING;

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

AnimationController::AnimationController(Context* context) :
    AnimationStateSource(context)
{
}

AnimationController::~AnimationController() = default;

void AnimationController::RegisterObject(Context* context)
{
    context->RegisterFactory<AnimationController>(LOGIC_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Animations", GetAnimationsAttr, SetAnimationsAttr, VariantVector, Variant::emptyVariantVector,
        AM_FILE | AM_NOEDIT);
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
    // Update individual animations
    pendingTriggers_.clear();
    for (auto& [params, state] : animations_)
    {
        const bool isCompleted = UpdateAnimationTime(params, timeStep);
        if (isCompleted)
            ApplyAutoFadeOut(params);
        UpdateAnimationWeight(params, timeStep);
        ApplyAnimationRemoval(params, isCompleted);
        if (!params.removed_)
            CommitAnimationParams(params, state);
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

    // Node hierarchy animations need to be applied manually
    for (AnimationState* state : animationStates_)
    {
        state->ApplyNodeTracks();
        state->ApplyAttributeTracks();
    }

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

void AnimationController::AddAnimation(const AnimationParameters& params)
{
    if (!params.animation_)
    {
        URHO3D_ASSERTLOG(0, "Null animation in AnimationController::AddAnimation");
        return;
    }

    AnimationInstance& instance = animations_.emplace_back();
    instance.params_ = params;

    auto* model = GetComponent<AnimatedModel>();
    instance.state_ = model ? MakeShared<AnimationState>(this, model) : MakeShared<AnimationState>(this, node_);
    instance.state_->Initialize(instance.params_.animation_, instance.params_.startBone_, instance.params_.blendMode_);

    CommitAnimationParams(instance.params_, instance.state_);

    animationStatesDirty_ = true;
}

void AnimationController::UpdateAnimation(unsigned index, const AnimationParameters& params)
{
    if (index >= animations_.size())
    {
        URHO3D_ASSERTLOG(0, "Out-of-bounds animation index in AnimationController::UpdateAnimation");
        return;
    }

    AnimationInstance& instance = animations_[index];
    const bool initParamsChanged = instance.params_.animation_ != params.animation_
        || instance.params_.startBone_ != params.startBone_ || instance.params_.blendMode_ != params.blendMode_;
    if (initParamsChanged)
        instance.state_->Initialize(params.animation_, params.startBone_, params.blendMode_);
    if (instance.params_.layer_ != params.layer_)
        animationStatesDirty_ = true;

    instance.params_ = params;

    CommitAnimationParams(instance.params_, instance.state_);
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
        if (params.targetWeight_ != 0.0f)
        {
            params.targetWeight_ = 0.0f;
            params.targetWeightDelay_ = fadeTime;
            params.removeOnZeroWeight_ = true;
            UpdateAnimation(index, params);
        }
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
        if (params.targetWeight_ != 0.0f)
        {
            params.targetWeight_ = 0.0f;
            params.targetWeightDelay_ = fadeTime;
            params.removeOnZeroWeight_ = true;
            UpdateAnimation(index, params);
        }
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
        if (params.targetWeight_ != 0.0f)
        {
            params.targetWeight_ = 0.0f;
            params.targetWeightDelay_ = fadeTime;
            params.removeOnZeroWeight_ = true;
            UpdateAnimation(index, params);
        }
    }
}

void AnimationController::StopAll(float fadeTime)
{
    for (unsigned index = 0; index < animations_.size(); ++index)
    {
        AnimationParameters params = GetAnimationParameters(index);
        params.targetWeight_ = 0.0f;
        params.targetWeightDelay_ = fadeTime;
        params.removeOnZeroWeight_ = true;
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

bool AnimationController::UpdateAnimationWeight(Animation* animation, float weight)
{
    const unsigned index = FindLastAnimation(animation);
    if (index != M_MAX_UNSIGNED)
    {
        AnimationParameters params = GetAnimationParameters(index);
        params.weight_ = weight;
        params.targetWeight_ = weight;
        params.targetWeightDelay_ = 0.0f;
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

bool AnimationController::Play(const ea::string& name, unsigned char layer, bool looped, float fadeInTime)
{
    Animation* animation = GetAnimationByName(name);
    if (!animation)
        return false;

    auto params = AnimationParameters{animation}.Layer(layer);
    params.removeOnZeroWeight_ = true;
    params.looped_ = looped;
    PlayExisting(params, fadeInTime);
    return true;
}

bool AnimationController::PlayExclusive(const ea::string& name, unsigned char layer, bool looped, float fadeTime)
{
    StopLayer(layer, fadeTime);
    return Play(name, layer, looped, fadeTime);
}

bool AnimationController::Stop(const ea::string& name, float fadeOutTime)
{
    Animation* animation = GetAnimationByName(name);
    if (!animation)
        return false;

    Stop(animation, fadeOutTime);
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
    if (value.empty() || value[0].GetType() != VAR_INT)
        return;

    const unsigned numAnimations = value[0].GetUInt();
    if (value.size() != numAnimations * AnimationParameters::NumVariants + 1)
        return;

    const ea::span<const Variant> valueSpan{value};
    for (unsigned i = 0; i < numAnimations; ++i)
    {
        const auto span = valueSpan.subspan(1 + i * AnimationParameters::NumVariants, AnimationParameters::NumVariants);
        const auto params = AnimationParameters::FromVariantSpan(context_, span);
        if (!params.animation_)
        {
            URHO3D_LOGWARNING("Unknown Animation '{}' is skipped on load", span[0].ToString());
            continue;
        }

        AddAnimation(params);
    }
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

void AnimationController::OnNodeSet(Node* node)
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

bool AnimationController::UpdateAnimationTime(AnimationParameters& params, float timeStep)
{
    const float scaledTimeStep = timeStep * params.speed_;
    if (scaledTimeStep == 0.0f)
        return false;

    const auto delta = params.looped_
        ? params.time_.UpdateWrapped(scaledTimeStep)
        : params.time_.UpdateClamped(scaledTimeStep);

    const bool timeAtBegin = delta.End() == delta.Min();
    const bool timeAtEnd = delta.End() == delta.Max();
    const bool isCompleted = !params.looped_ && (params.speed_ > 0 ? timeAtEnd : timeAtBegin);

    const float length = params.animation_->GetLength();
    for (const AnimationTriggerPoint& trigger : params.animation_->GetTriggers())
    {
        if (delta.ContainsExcludingBegin(ea::min(trigger.time_, length)))
            pendingTriggers_.emplace_back(params.animation_, &trigger);
    }

    return isCompleted;
}

void AnimationController::ApplyAutoFadeOut(AnimationParameters& params) const
{
    // Apply auto fade out only if enabled and if not fading out already
    if (params.targetWeight_ == 0.0f || params.autoFadeOutTime_ == 0.0f)
        return;

    params.targetWeight_ = 0.0f;
    params.targetWeightDelay_ = params.autoFadeOutTime_;
    params.removeOnZeroWeight_ = true;
    params.removeOnCompletion_ = false;
}

void AnimationController::UpdateAnimationWeight(AnimationParameters& params, float timeStep) const
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

void AnimationController::ApplyAnimationRemoval(AnimationParameters& params, bool isCompleted) const
{
    if (params.removeOnCompletion_ && isCompleted)
        params.removed_ = true;
    if (params.removeOnZeroWeight_ && params.weight_ < M_LARGE_EPSILON)
        params.removed_ = true;
}

void AnimationController::CommitAnimationParams(const AnimationParameters& params, AnimationState* state) const
{
    state->SetTime(params.time_.Value());
    state->SetWeight(params.weight_);
}

void AnimationController::SortAnimationStates()
{
    const unsigned numAnimations = GetNumAnimations();
    animationStates_.resize(numAnimations);
    if (numAnimations == 0)
        return;

    animationsSortBuffer_.resize(numAnimations * 2);
    ea::copy(animations_.begin(), animations_.end(), animationsSortBuffer_.begin());

    const auto beginIter = animationsSortBuffer_.begin();
    const auto endIter = ea::next(animationsSortBuffer_.begin(), numAnimations);
    ea::merge_sort_buffer(beginIter, endIter, &animationsSortBuffer_[numAnimations],
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

    // Use root node or start bone node if specified
    // Note: bone tracks are resolved starting from root bone node,
    // variant tracks are resolved from the node itself.
    const ea::string& startBoneName = state->GetStartBone();
    Node* startNode = !startBoneName.empty() ? node_->GetChild(startBoneName, true) : nullptr;
    if (!startNode)
        startNode = node_;

    Node* startBone = startNode == node_ && model ? model->GetSkeleton().GetRootBone()->node_ : startNode;
    if (!startBone)
    {
        URHO3D_LOGWARNING("AnimatedModel skeleton is not initialized");
        return;
    }

    // Setup model and node tracks
    const auto& tracks = animation->GetTracks();
    for (const auto& item : tracks)
    {
        const AnimationTrack& track = item.second;
        Node* trackNode = GetTrackNodeByNameHash(track.nameHash_, startBone);
        const unsigned trackBoneIndex = trackNode && model ? model->GetSkeleton().GetBoneIndex(track.nameHash_) : M_MAX_UNSIGNED;
        Bone* trackBone = trackBoneIndex != M_MAX_UNSIGNED ? model->GetSkeleton().GetBone(trackBoneIndex) : nullptr;

        // Add model track
        if (trackBone && trackBone->node_)
        {
            ModelAnimationStateTrack stateTrack;
            stateTrack.track_ = &track;
            stateTrack.boneIndex_ = trackBoneIndex;
            stateTrack.node_ = trackBone->node_;
            stateTrack.bone_ = trackBone;
            state->AddModelTrack(stateTrack);
        }
        else if (trackNode)
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
