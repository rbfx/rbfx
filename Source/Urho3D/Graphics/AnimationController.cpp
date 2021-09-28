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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationController.h"
#include "../Graphics/AnimationState.h"
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
    URHO3D_ACCESSOR_ATTRIBUTE("Network Animations", GetNetAnimationsAttr, SetNetAnimationsAttr, ea::vector<unsigned char>,
        Variant::emptyBuffer, AM_NET | AM_LATESTDATA | AM_NOEDIT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Node Animation States", GetNodeAnimationStatesAttr, SetNodeAnimationStatesAttr, VariantVector,
        Variant::emptyVariantVector, AM_FILE | AM_NOEDIT | AM_READONLY);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Animation States", GetAnimationStatesAttr, SetAnimationStatesAttr, VariantVector,
        Variant::emptyVariantVector, AM_FILE | AM_NOEDIT);
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
    // Loop through animations
    for (unsigned i = 0; i < animations_.size();)
    {
        AnimationControl& ctrl = animations_[i];
        AnimationState* state = GetAnimationState(ctrl.hash_);
        bool remove = false;

        if (!state)
            remove = true;
        else
        {
            // Advance the animation
            if (ctrl.speed_ != 0.0f)
                state->AddTime(ctrl.speed_ * timeStep);

            float targetWeight = ctrl.targetWeight_;
            float fadeTime = ctrl.fadeTime_;

            // If non-looped animation at the end, activate autofade as applicable
            if (!state->IsLooped() && state->GetTime() >= state->GetLength() && ctrl.autoFadeTime_ > 0.0f)
            {
                targetWeight = 0.0f;
                fadeTime = ctrl.autoFadeTime_;
            }

            // Process weight fade
            float currentWeight = state->GetWeight();
            if (currentWeight != targetWeight)
            {
                if (fadeTime > 0.0f)
                {
                    float weightDelta = 1.0f / fadeTime * timeStep;
                    if (currentWeight < targetWeight)
                        currentWeight = Min(currentWeight + weightDelta, targetWeight);
                    else if (currentWeight > targetWeight)
                        currentWeight = Max(currentWeight - weightDelta, targetWeight);
                    state->SetWeight(currentWeight);
                }
                else
                    state->SetWeight(targetWeight);
            }

            // Remove if weight zero and target weight zero
            if (state->GetWeight() == 0.0f && (targetWeight == 0.0f || fadeTime == 0.0f) && ctrl.removeOnCompletion_)
                remove = true;
        }

        // Decrement the command time-to-live values
        if (ctrl.setTimeTtl_ > 0.0f)
            ctrl.setTimeTtl_ = Max(ctrl.setTimeTtl_ - timeStep, 0.0f);
        if (ctrl.setWeightTtl_ > 0.0f)
            ctrl.setWeightTtl_ = Max(ctrl.setWeightTtl_ - timeStep, 0.0f);

        if (remove)
        {
            if (state)
                RemoveAnimationState(state);
            animations_.erase_at(i);
            MarkNetworkUpdate();
        }
        else
            ++i;
    }

    // Sort animation states if necessary
    if (animationStateOrderDirty_)
    {
        ea::sort(animationStates_.begin(), animationStates_.end(), CompareLayers);
        animationStateOrderDirty_ = false;
    }

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
}

bool AnimationController::Play(const ea::string& name, unsigned char layer, bool looped, float fadeInTime)
{
    // Get the animation resource first to be able to get the canonical resource name
    // (avoids potential adding of duplicate animations)
    auto* newAnimation = GetSubsystem<ResourceCache>()->GetResource<Animation>(name);
    if (!newAnimation)
        return false;

    // Check if already exists
    unsigned index;
    AnimationState* state;
    FindAnimation(newAnimation->GetName(), index, state);

    if (!state)
    {
        state = AddAnimationState(newAnimation);
        if (!state)
            return false;
    }

    if (index == M_MAX_UNSIGNED)
    {
        AnimationControl newControl;
        newControl.name_ = newAnimation->GetName();
        newControl.hash_ = newAnimation->GetNameHash();
        animations_.push_back(newControl);
        index = animations_.size() - 1;
    }

    state->SetLayer(layer);
    state->SetLooped(looped);
    animations_[index].targetWeight_ = 1.0f;
    animations_[index].fadeTime_ = fadeInTime;

    MarkNetworkUpdate();
    return true;
}

bool AnimationController::PlayExclusive(const ea::string& name, unsigned char layer, bool looped, float fadeTime)
{
    bool success = Play(name, layer, looped, fadeTime);

    // Fade other animations only if successfully started the new one
    if (success)
        FadeOthers(name, 0.0f, fadeTime);

    return success;
}

bool AnimationController::Stop(const ea::string& name, float fadeOutTime)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index != M_MAX_UNSIGNED)
    {
        animations_[index].targetWeight_ = 0.0f;
        animations_[index].fadeTime_ = fadeOutTime;
        MarkNetworkUpdate();
    }

    return index != M_MAX_UNSIGNED || state != nullptr;
}

void AnimationController::StopLayer(unsigned char layer, float fadeOutTime)
{
    bool needUpdate = false;
    for (auto i = animations_.begin(); i != animations_.end(); ++i)
    {
        AnimationState* state = GetAnimationState(i->hash_);
        if (state && state->GetLayer() == layer)
        {
            i->targetWeight_ = 0.0f;
            i->fadeTime_ = fadeOutTime;
            needUpdate = true;
        }
    }

    if (needUpdate)
        MarkNetworkUpdate();
}

void AnimationController::StopAll(float fadeOutTime)
{
    if (animations_.size())
    {
        for (auto i = animations_.begin(); i != animations_.end(); ++i)
        {
            i->targetWeight_ = 0.0f;
            i->fadeTime_ = fadeOutTime;
        }

        MarkNetworkUpdate();
    }
}

bool AnimationController::Fade(const ea::string& name, float targetWeight, float fadeTime)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED)
        return false;

    animations_[index].targetWeight_ = Clamp(targetWeight, 0.0f, 1.0f);
    animations_[index].fadeTime_ = fadeTime;
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::FadeOthers(const ea::string& name, float targetWeight, float fadeTime)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED || !state)
        return false;

    unsigned char layer = state->GetLayer();

    bool needUpdate = false;
    for (unsigned i = 0; i < animations_.size(); ++i)
    {
        if (i != index)
        {
            AnimationControl& control = animations_[i];
            AnimationState* otherState = GetAnimationState(control.hash_);
            if (otherState && otherState->GetLayer() == layer)
            {
                control.targetWeight_ = Clamp(targetWeight, 0.0f, 1.0f);
                control.fadeTime_ = fadeTime;
                needUpdate = true;
            }
        }
    }

    if (needUpdate)
        MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetLayer(const ea::string& name, unsigned char layer)
{
    AnimationState* state = GetAnimationState(name);
    if (!state)
        return false;

    state->SetLayer(layer);
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetStartBone(const ea::string& name, const ea::string& startBoneName)
{
    AnimationState* state = GetAnimationState(name);
    if (!state)
        return false;

    state->SetStartBone(startBoneName);
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetTime(const ea::string& name, float time)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED || !state)
        return false;

    time = Clamp(time, 0.0f, state->GetLength());
    state->SetTime(time);
    // Prepare "set time" command for network replication
    animations_[index].setTime_ = (unsigned short)(time / state->GetLength() * 65535.0f);
    animations_[index].setTimeTtl_ = COMMAND_STAY_TIME;
    ++animations_[index].setTimeRev_;
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetSpeed(const ea::string& name, float speed)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED)
        return false;

    animations_[index].speed_ = speed;
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetWeight(const ea::string& name, float weight)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED || !state)
        return false;

    weight = Clamp(weight, 0.0f, 1.0f);
    state->SetWeight(weight);
    // Prepare "set weight" command for network replication
    animations_[index].setWeight_ = (unsigned char)(weight * 255.0f);
    animations_[index].setWeightTtl_ = COMMAND_STAY_TIME;
    ++animations_[index].setWeightRev_;
    // Cancel any ongoing weight fade
    animations_[index].targetWeight_ = weight;
    animations_[index].fadeTime_ = 0.0f;

    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetRemoveOnCompletion(const ea::string& name, bool removeOnCompletion)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED || !state)
        return false;

    animations_[index].removeOnCompletion_ = removeOnCompletion;
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetLooped(const ea::string& name, bool enable)
{
    AnimationState* state = GetAnimationState(name);
    if (!state)
        return false;

    state->SetLooped(enable);
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetBlendMode(const ea::string& name, AnimationBlendMode mode)
{
    AnimationState* state = GetAnimationState(name);
    if (!state)
        return false;

    state->SetBlendMode(mode);
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::SetAutoFade(const ea::string& name, float fadeOutTime)
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED)
        return false;

    animations_[index].autoFadeTime_ = Max(fadeOutTime, 0.0f);
    MarkNetworkUpdate();
    return true;
}

bool AnimationController::IsPlaying(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    return index != M_MAX_UNSIGNED;
}

bool AnimationController::IsPlaying(unsigned char layer) const
{
    for (auto i = animations_.begin(); i != animations_.end(); ++i)
    {
        AnimationState* state = GetAnimationState(i->hash_);
        if (state && state->GetLayer() == layer)
            return true;
    }

    return false;
}

bool AnimationController::IsFadingIn(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED || !state)
        return false;

    return animations_[index].fadeTime_ && animations_[index].targetWeight_ > state->GetWeight();
}

bool AnimationController::IsFadingOut(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED || !state)
        return false;

    return (animations_[index].fadeTime_ && animations_[index].targetWeight_ < state->GetWeight())
           || (!state->IsLooped() && state->GetTime() >= state->GetLength() && animations_[index].autoFadeTime_);
}

bool AnimationController::IsAtEnd(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    if (index == M_MAX_UNSIGNED || !state)
        return false;
    else
        return state->GetTime() >= state->GetLength();
}

unsigned char AnimationController::GetLayer(const ea::string& name) const
{
    AnimationState* state = GetAnimationState(name);
    return (unsigned char)(state ? state->GetLayer() : 0);
}

float AnimationController::GetTime(const ea::string& name) const
{
    AnimationState* state = GetAnimationState(name);
    return state ? state->GetTime() : 0.0f;
}

float AnimationController::GetWeight(const ea::string& name) const
{
    AnimationState* state = GetAnimationState(name);
    return state ? state->GetWeight() : 0.0f;
}

bool AnimationController::IsLooped(const ea::string& name) const
{
    AnimationState* state = GetAnimationState(name);
    return state ? state->IsLooped() : false;
}

AnimationBlendMode AnimationController::GetBlendMode(const ea::string& name) const
{
    AnimationState* state = GetAnimationState(name);
    return state ? state->GetBlendMode() : ABM_LERP;
}

float AnimationController::GetLength(const ea::string& name) const
{
    AnimationState* state = GetAnimationState(name);
    return state ? state->GetLength() : 0.0f;
}

float AnimationController::GetSpeed(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    return index != M_MAX_UNSIGNED ? animations_[index].speed_ : 0.0f;
}

float AnimationController::GetFadeTarget(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    return index != M_MAX_UNSIGNED ? animations_[index].targetWeight_ : 0.0f;
}

float AnimationController::GetFadeTime(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    return index != M_MAX_UNSIGNED ? animations_[index].targetWeight_ : 0.0f;
}

float AnimationController::GetAutoFade(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    return index != M_MAX_UNSIGNED ? animations_[index].autoFadeTime_ : 0.0f;
}

bool AnimationController::GetRemoveOnCompletion(const ea::string& name) const
{
    unsigned index;
    AnimationState* state;
    FindAnimation(name, index, state);
    return index != M_MAX_UNSIGNED ? animations_[index].removeOnCompletion_ : false;
}

AnimationState* AnimationController::GetAnimationState(const ea::string& name) const
{
    return GetAnimationState(StringHash(name));
}

AnimationState* AnimationController::GetAnimationState(StringHash nameHash) const
{
    for (auto i = animationStates_.begin(); i != animationStates_.end(); ++i)
    {
        Animation* animation = (*i)->GetAnimation();
        if (animation->GetNameHash() == nameHash || animation->GetAnimationNameHash() == nameHash)
            return *i;
    }

    return nullptr;
}

void AnimationController::SetAnimationsAttr(const VariantVector& value)
{
    animations_.clear();
    animations_.reserve(value.size() / 5);  // Incomplete data is discarded
    unsigned index = 0;
    while (index + 4 < value.size())    // Prevent out-of-bound index access
    {
        AnimationControl newControl;
        newControl.name_ = value[index++].GetString();
        newControl.hash_ = StringHash(newControl.name_);
        newControl.speed_ = value[index++].GetFloat();
        newControl.targetWeight_ = value[index++].GetFloat();
        newControl.fadeTime_ = value[index++].GetFloat();
        newControl.autoFadeTime_ = value[index++].GetFloat();
        animations_.push_back(newControl);
    }
}

void AnimationController::SetNetAnimationsAttr(const ea::vector<unsigned char>& value)
{
    MemoryBuffer buf(value);

    auto* model = GetComponent<AnimatedModel>();

    // Check which animations we need to remove
    ea::hash_set<StringHash> processedAnimations;

    unsigned numAnimations = buf.ReadVLE();
    while (numAnimations--)
    {
        ea::string animName = buf.ReadString();
        StringHash animHash(animName);
        processedAnimations.insert(animHash);

        // Check if the animation state exists. If not, add new
        AnimationState* state = GetAnimationState(animHash);
        if (!state)
        {
            auto* newAnimation = GetSubsystem<ResourceCache>()->GetResource<Animation>(animName);
            state = AddAnimationState(newAnimation);
            if (!state)
            {
                URHO3D_LOGERROR("Animation update applying aborted due to unknown animation");
                return;
            }
        }
        // Check if the internal control structure exists. If not, add new
        unsigned index;
        for (index = 0; index < animations_.size(); ++index)
        {
            if (animations_[index].hash_ == animHash)
                break;
        }
        if (index == animations_.size())
        {
            AnimationControl newControl;
            newControl.name_ = animName;
            newControl.hash_ = animHash;
            animations_.push_back(newControl);
        }

        unsigned char ctrl = buf.ReadUByte();
        state->SetLayer(buf.ReadUByte());
        state->SetLooped((ctrl & CTRL_LOOPED) != 0);
        state->SetBlendMode((ctrl & CTRL_ADDITIVE) != 0 ? ABM_ADDITIVE : ABM_LERP);
        animations_[index].speed_ = (float)buf.ReadShort() / 2048.0f; // 11 bits of decimal precision, max. 16x playback speed
        animations_[index].targetWeight_ = (float)buf.ReadUByte() / 255.0f; // 8 bits of decimal precision
        animations_[index].fadeTime_ = (float)buf.ReadUByte() / 64.0f; // 6 bits of decimal precision, max. 4 seconds fade
        if (ctrl & CTRL_STARTBONE)
        {
            const ea::string startBoneName = buf.ReadString();
            state->SetStartBone(startBoneName);
        }
        else
            state->SetStartBone("");
        if (ctrl & CTRL_AUTOFADE)
            animations_[index].autoFadeTime_ = (float)buf.ReadUByte() / 64.0f; // 6 bits of decimal precision, max. 4 seconds fade
        else
            animations_[index].autoFadeTime_ = 0.0f;

        animations_[index].removeOnCompletion_ = (ctrl & CTRL_REMOVEONCOMPLETION) != 0;

        if (ctrl & CTRL_SETTIME)
        {
            unsigned char setTimeRev = buf.ReadUByte();
            unsigned short setTime = buf.ReadUShort();
            // Apply set time command only if revision differs
            if (setTimeRev != animations_[index].setTimeRev_)
            {
                state->SetTime(((float)setTime / 65535.0f) * state->GetLength());
                animations_[index].setTimeRev_ = setTimeRev;
            }
        }
        if (ctrl & CTRL_SETWEIGHT)
        {
            unsigned char setWeightRev = buf.ReadUByte();
            unsigned char setWeight = buf.ReadUByte();
            // Apply set weight command only if revision differs
            if (setWeightRev != animations_[index].setWeightRev_)
            {
                state->SetWeight((float)setWeight / 255.0f);
                animations_[index].setWeightRev_ = setWeightRev;
            }
        }
    }

    // Set any extra animations to fade out
    for (auto i = animations_.begin(); i != animations_.end(); ++i)
    {
        if (!processedAnimations.contains(i->hash_))
        {
            i->targetWeight_ = 0.0f;
            i->fadeTime_ = EXTRA_ANIM_FADEOUT_TIME;
        }
    }
}

void AnimationController::SetNodeAnimationStatesAttr(const VariantVector& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    animationStates_.clear();
    unsigned index = 0;
    unsigned numStates = index < value.size() ? value[index++].GetUInt() : 0;
    // Prevent negative or overly large value being assigned from the editor
    if (numStates > M_MAX_INT)
        numStates = 0;
    if (numStates > MAX_NODE_ANIMATION_STATES)
        numStates = MAX_NODE_ANIMATION_STATES;

    animationStates_.reserve(numStates);
    while (numStates--)
    {
        if (index + 2 < value.size())
        {
            // Note: null animation is allowed here for editing
            const ResourceRef& animRef = value[index++].GetResourceRef();
            SharedPtr<AnimationState> newState(new AnimationState(this, GetNode(), cache->GetResource<Animation>(animRef.name_)));
            animationStates_.push_back(newState);

            newState->SetLooped(value[index++].GetBool());
            newState->SetTime(value[index++].GetFloat());
        }
        else
        {
            // If not enough data, just add an empty animation state
            SharedPtr<AnimationState> newState(new AnimationState(this, GetNode(), nullptr));
            animationStates_.push_back(newState);
        }
    }

    MarkAnimationStateOrderDirty();
    MarkAnimationStateTracksDirty();
}

VariantVector AnimationController::GetAnimationsAttr() const
{
    VariantVector ret;
    ret.reserve(animations_.size() * 5);
    for (auto i = animations_.begin(); i != animations_.end(); ++i)
    {
        ret.push_back(i->name_);
        ret.push_back(i->speed_);
        ret.push_back(i->targetWeight_);
        ret.push_back(i->fadeTime_);
        ret.push_back(i->autoFadeTime_);
    }
    return ret;
}

const ea::vector<unsigned char>& AnimationController::GetNetAnimationsAttr() const
{
    attrBuffer_.Clear();

    auto* model = GetComponent<AnimatedModel>();

    unsigned validAnimations = 0;
    for (auto i = animations_.begin(); i != animations_.end(); ++i)
    {
        if (GetAnimationState(i->hash_))
            ++validAnimations;
    }

    attrBuffer_.WriteVLE(validAnimations);
    for (auto i = animations_.begin(); i != animations_.end(); ++i)
    {
        AnimationState* state = GetAnimationState(i->hash_);
        if (!state)
            continue;

        unsigned char ctrl = 0;
        if (state->IsLooped())
            ctrl |= CTRL_LOOPED;
        if (state->GetBlendMode() == ABM_ADDITIVE)
            ctrl |= CTRL_ADDITIVE;
        if (!state->GetStartBone().empty())
            ctrl |= CTRL_STARTBONE;
        if (i->autoFadeTime_ > 0.0f)
            ctrl |= CTRL_AUTOFADE;
        if (i->removeOnCompletion_)
            ctrl |= CTRL_REMOVEONCOMPLETION;
        if (i->setTimeTtl_ > 0.0f)
            ctrl |= CTRL_SETTIME;
        if (i->setWeightTtl_ > 0.0f)
            ctrl |= CTRL_SETWEIGHT;

        attrBuffer_.WriteString(i->name_);
        attrBuffer_.WriteUByte(ctrl);
        attrBuffer_.WriteUByte(state->GetLayer());
        attrBuffer_.WriteShort((short)Clamp(i->speed_ * 2048.0f, -32767.0f, 32767.0f));
        attrBuffer_.WriteUByte((unsigned char)(i->targetWeight_ * 255.0f));
        attrBuffer_.WriteUByte((unsigned char)Clamp(i->fadeTime_ * 64.0f, 0.0f, 255.0f));
        if (ctrl & CTRL_STARTBONE)
            attrBuffer_.WriteString(state->GetStartBone());
        if (ctrl & CTRL_AUTOFADE)
            attrBuffer_.WriteUByte((unsigned char)Clamp(i->autoFadeTime_ * 64.0f, 0.0f, 255.0f));
        if (ctrl & CTRL_SETTIME)
        {
            attrBuffer_.WriteUByte(i->setTimeRev_);
            attrBuffer_.WriteUShort(i->setTime_);
        }
        if (ctrl & CTRL_SETWEIGHT)
        {
            attrBuffer_.WriteUByte(i->setWeightRev_);
            attrBuffer_.WriteUByte(i->setWeight_);
        }
    }

    return attrBuffer_.GetBuffer();
}

VariantVector AnimationController::GetNodeAnimationStatesAttr() const
{
    URHO3D_LOGERROR("AnimationController::GetNodeAnimationStatesAttr is deprecated");
    return Variant::emptyVariantVector;
}

void AnimationController::SetAnimationStatesAttr(const VariantVector& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* model = GetComponent<AnimatedModel>();

    animationStates_.clear();
    animationStates_.reserve(value.size() / 7);
    unsigned index = 0;
    while (index + 6 < value.size())
    {
        const ResourceRef& animRef = value[index++].GetResourceRef();
        const ea::string& startBoneName = value[index++].GetString();
        const bool isLooped = value[index++].GetBool();
        const float weight = value[index++].GetFloat();
        const float time = value[index++].GetFloat();
        const auto layer = static_cast<unsigned char>(value[index++].GetInt());
        const auto blendMode = static_cast<AnimationBlendMode>(value[index++].GetInt());

        // Create new animation state
        const auto animation = cache->GetResource<Animation>(animRef.name_);
        AnimationState* newState = AddAnimationState(animation);

        // Setup new animation state
        newState->SetStartBone(startBoneName);
        newState->SetLooped(isLooped);
        newState->SetWeight(weight);
        newState->SetTime(time);
        newState->SetLayer(layer);
        newState->SetBlendMode(blendMode);
    }

    MarkAnimationStateOrderDirty();
    MarkAnimationStateTracksDirty();
}

VariantVector AnimationController::GetAnimationStatesAttr() const
{
    VariantVector ret;
    ret.reserve(animationStates_.size() * 7);
    for (AnimationState* state : animationStates_)
    {
        Animation* animation = state->GetAnimation();
        ret.push_back(GetResourceRef(animation, Animation::GetTypeStatic()));
        ret.push_back(state->GetStartBone());
        ret.push_back(state->IsLooped());
        ret.push_back(state->GetWeight());
        ret.push_back(state->GetTime());
        ret.push_back((int) state->GetLayer());
        ret.push_back((int) state->GetBlendMode());
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

AnimationState* AnimationController::AddAnimationState(Animation* animation)
{
    if (!animation)
        return nullptr;

    auto* model = GetComponent<AnimatedModel>();
    if (model)
        animationStates_.emplace_back(new AnimationState(this, model, animation));
    else
        animationStates_.emplace_back(new AnimationState(this, node_, animation));

    MarkAnimationStateOrderDirty();
    return animationStates_.back();
}

void AnimationController::RemoveAnimationState(AnimationState* state)
{
    if (!state)
        return;

    const auto iter = ea::find(animationStates_.begin(), animationStates_.end(), state);
    if (iter != animationStates_.end())
        animationStates_.erase(iter);
}

void AnimationController::FindAnimation(const ea::string& name, unsigned& index, AnimationState*& state) const
{
    StringHash nameHash(GetInternalPath(name));

    // Find the AnimationState
    state = GetAnimationState(nameHash);
    if (state)
    {
        // Either a resource name or animation name may be specified. We store resource names, so correct the hash if necessary
        nameHash = state->GetAnimation()->GetNameHash();
    }

    // Find the internal control structure
    index = M_MAX_UNSIGNED;
    for (unsigned i = 0; i < animations_.size(); ++i)
    {
        if (animations_[i].hash_ == nameHash)
        {
            index = i;
            break;
        }
    }
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

bool AnimationController::ParseAnimatablePath(ea::string_view path, Node* startNode,
    WeakPtr<Serializable>& serializable, unsigned& attributeIndex, StringHash& variableName)
{
    if (path.empty())
    {
        URHO3D_LOGWARNING("Variant animation track name must not be empty");
        return false;
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
            return false;
        }

        const ea::string_view nodePath = path.substr(0, sep);
        animatedNode = startNode->FindChild(nodePath);
        if (!animatedNode)
        {
            URHO3D_LOGWARNING("Path to node \"{}\" cannot be resolved", nodePath);
            return false;
        }

        attributePath = path.substr(sep + 1);
    }

    // Special case: if Node variables are referenced, individual variables are supported
    static const ea::string_view variablesPath = "@/Variables/";
    if (attributePath.starts_with(variablesPath))
    {
        variableName = attributePath.substr(variablesPath.size());
        attributePath = attributePath.substr(0, variablesPath.size() - 1);
    }

    // Parse path to component and attribute
    const auto& serializableAndAttribute = animatedNode->FindComponentAttribute(attributePath);
    if (!serializableAndAttribute.first)
    {
        URHO3D_LOGWARNING("Path to attribute \"{}\" cannot be resolved", attributePath);
        return false;
    }

    serializable = serializableAndAttribute.first;
    attributeIndex = serializableAndAttribute.second;
    return true;
}

void AnimationController::UpdateAnimationStateTracks(AnimationState* state)
{
    // TODO(animation): Cache lookups?
    auto model = GetComponent<AnimatedModel>();
    const Animation* animation = state->GetAnimation();

    // Reset internal state. Shouldn't cause any reallocations due to simple vectors inside.
    state->ClearAllTracks();

    // Use root node or start bone node if specified
    const ea::string& startBoneName = state->GetStartBone();
    Node* startNode = !startBoneName.empty() ? node_->GetChild(startBoneName, true) : nullptr;
    if (!startNode)
        startNode = node_;

    // Setup model and node tracks
    const auto& tracks = animation->GetTracks();
    for (const auto& item : tracks)
    {
        const AnimationTrack& track = item.second;
        Node* trackNode = track.nameHash_ == startNode->GetNameHash() ? startNode
            : startNode->GetChild(track.nameHash_, true);
        Bone* trackBone = model ? model->GetSkeleton().GetBone(track.nameHash_) : nullptr;

        // Add model track
        if (trackBone && trackBone->node_)
        {
            ModelAnimationStateTrack stateTrack;
            stateTrack.track_ = &track;
            stateTrack.node_ = trackNode;
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

        if (ParseAnimatablePath(trackName, startNode,
            stateTrack.serializable_, stateTrack.attributeIndex_, stateTrack.variableName_))
        {
            state->AddAttributeTrack(stateTrack);
        }
    }
}

void AnimationController::ConnectToAnimatedModel()
{
    auto model = GetComponent<AnimatedModel>();
    if (model)
        model->ConnectToAnimationStateSource(this);
}

}
