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

#include "../Graphics/AnimatedModel.h"
#include "../Graphics/Animation.h"
#include "../Graphics/AnimationController.h"
#include "../Graphics/AnimationState.h"
#include "../Graphics/DrawableEvents.h"
#include "../IO/Log.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

Variant BlendAdditive(const Variant& oldValue, const Variant& newValue, const Variant& baseValue, float weight)
{
    switch (newValue.GetType())
    {
    case VAR_FLOAT:
        return oldValue.GetFloat() + (newValue.GetFloat() - baseValue.GetFloat()) * weight;

    case VAR_DOUBLE:
        return oldValue.GetDouble() + (newValue.GetDouble() - baseValue.GetDouble()) * weight;

    case VAR_INT:
        return static_cast<int>(oldValue.GetInt() + (newValue.GetInt() - baseValue.GetInt()) * weight);

    case VAR_INT64:
        return static_cast<long long>(oldValue.GetInt64() + (newValue.GetInt64() - baseValue.GetInt64()) * weight);

    case VAR_VECTOR2:
        return oldValue.GetVector2() + (newValue.GetVector2() - baseValue.GetVector2()) * weight;

    case VAR_VECTOR3:
        return oldValue.GetVector3() + (newValue.GetVector3() - baseValue.GetVector3()) * weight;

    case VAR_VECTOR4:
        return oldValue.GetVector4() + (newValue.GetVector4() - baseValue.GetVector4()) * weight;

    case VAR_QUATERNION:
        return oldValue.GetQuaternion() * Quaternion::IDENTITY.Slerp(newValue.GetQuaternion() * baseValue.GetQuaternion().Inverse(), weight);

    case VAR_COLOR:
        return oldValue.GetColor() + (newValue.GetColor() - baseValue.GetColor()) * weight;

    case VAR_INTVECTOR2:
        return oldValue.GetIntVector2() + VectorRoundToInt((newValue.GetIntVector2() - baseValue.GetIntVector2()).ToVector2() * weight);

    case VAR_INTVECTOR3:
        return oldValue.GetIntVector3() + VectorRoundToInt((newValue.GetIntVector3() - baseValue.GetIntVector3()).ToVector3() * weight);

    default:
        return oldValue;
    }
}

}

void AnimatedAttributeReference::SetValue(const Variant& value) const
{
    switch (attributeType_)
    {
    case AnimatedAttributeType::Default:
        serializable_->SetAttribute(attributeIndex_, value);
        break;

    case AnimatedAttributeType::NodeVariables:
    {
        URHO3D_ASSERT(dynamic_cast<Node*>(serializable_.Get()));
        auto& node = static_cast<Node&>(*serializable_);
        node.SetVarByHash(StringHash(subAttributeKey_), value);
        break;
    }

    case AnimatedAttributeType::AnimatedModelMorphs:
    {
        URHO3D_ASSERT(dynamic_cast<AnimatedModel*>(serializable_.Get()));
        auto& animatedModel = static_cast<AnimatedModel&>(*serializable_);
        animatedModel.SetMorphWeight(subAttributeKey_, value.GetFloat());
        break;
    }
    }
}

AnimationState::AnimationState(AnimationController* controller, AnimatedModel* model) :
    controller_(controller),
    model_(model)
{
}

AnimationState::AnimationState(AnimationController* controller, Node* node) :
    controller_(controller),
    node_(node)
{
}

AnimationState::~AnimationState() = default;

void AnimationState::Initialize(Animation* animation, const ea::string& startBone, AnimationBlendMode blendMode)
{
    if (animation_ != animation || startBone_ != startBone || blendMode != blendingMode_)
    {
        animation_ = animation;
        startBone_ = startBone;
        blendingMode_ = blendMode;
        MarkTracksDirty();
    }
}

void AnimationState::Update(bool looped, float time, float weight)
{
    SetLooped(looped);
    SetTime(time);
    SetWeight(weight);
}

bool AnimationState::AreTracksDirty() const
{
    return tracksDirty_ || (animation_ && animation_->GetRevision() != animationRevision_);
}

void AnimationState::MarkTracksDirty()
{
    tracksDirty_ = true;
}

void AnimationState::ClearAllTracks()
{
    modelTracks_.clear();
    nodeTracks_.clear();
    attributeTracks_.clear();
}

void AnimationState::AddModelTrack(const ModelAnimationStateTrack& track)
{
    modelTracks_.push_back(track);
}

void AnimationState::AddNodeTrack(const NodeAnimationStateTrack& track)
{
    nodeTracks_.push_back(track);
}

void AnimationState::AddAttributeTrack(const AttributeAnimationStateTrack& track)
{
    attributeTracks_.push_back(track);
}

void AnimationState::OnTracksReady()
{
    tracksDirty_ = false;
    animationRevision_ = animation_ ? animation_->GetRevision() : ObjectRevisionTracker::InvalidRevision;
    if (model_)
        model_->MarkAnimationDirty();
}

void AnimationState::SetLooped(bool looped)
{
    if (looped_ != looped)
    {
        looped_ = looped;
        if (model_)
            model_->MarkAnimationDirty();
    }
}

void AnimationState::SetWeight(float weight)
{
    if (!animation_)
        return;

    weight = Clamp(weight, 0.0f, 1.0f);
    if (weight != weight_)
    {
        weight_ = weight;
        if (model_)
            model_->MarkAnimationDirty();
    }
}

void AnimationState::SetTime(float time)
{
    if (!animation_)
        return;

    time = Clamp(time, 0.0f, animation_->GetLength());
    if (time != time_)
    {
        time_ = time;
        if (model_)
            model_->MarkAnimationDirty();
    }
}

AnimatedModel* AnimationState::GetModel() const
{
    return model_;
}

Node* AnimationState::GetNode() const
{
    return node_;
}

float AnimationState::GetLength() const
{
    return animation_ ? animation_->GetLength() : 0.0f;
}

void AnimationState::CalculateModelTracks(ea::vector<ModelAnimationOutput>& output) const
{
    if (!animation_ || !IsEnabled())
        return;

    for (const ModelAnimationStateTrack& stateTrack : modelTracks_)
    {
        // Do not apply if the bone has animation disabled
        if (!stateTrack.bone_->animated_)
            continue;

        URHO3D_ASSERT(output.size() > stateTrack.boneIndex_);
        ModelAnimationOutput& trackOutput = output[stateTrack.boneIndex_];

        unsigned keyFrame = stateTrack.keyFrame_;
        CalculateTransformTrack(trackOutput, *stateTrack.track_, keyFrame, weight_);
        stateTrack.keyFrame_ = keyFrame;
    }
}

void AnimationState::CalculateNodeTracks(ea::unordered_map<Node*, NodeAnimationOutput>& output) const
{
    if (!animation_ || !IsEnabled())
        return;

    for (const NodeAnimationStateTrack& stateTrack : nodeTracks_)
    {
        NodeAnimationOutput& trackOutput = output[stateTrack.node_.Get()];

        unsigned keyFrame = stateTrack.keyFrame_;
        CalculateTransformTrack(trackOutput, *stateTrack.track_, keyFrame, weight_);
        stateTrack.keyFrame_ = keyFrame;
    }
}

void AnimationState::CalculateAttributeTracks(ea::unordered_map<AnimatedAttributeReference, Variant>& output) const
{
    if (!animation_ || !IsEnabled())
        return;

    for (const AttributeAnimationStateTrack& stateTrack : attributeTracks_)
    {
        Variant& trackOutput = output[stateTrack.attribute_];

        unsigned keyFrame = stateTrack.keyFrame_;
        CalculateAttributeTrack(trackOutput, *stateTrack.track_, keyFrame, weight_);
        stateTrack.keyFrame_ = keyFrame;
    }
}

void AnimationState::CalculateTransformTrack(
    NodeAnimationOutput& output, const AnimationTrack& track, unsigned& frame, float baseWeight) const
{
    if (track.keyFrames_.empty())
        return;

    const float weight = baseWeight * track.weight_;
    const bool isFullWeight = Equals(weight, 1.0f);
    const AnimationKeyFrame& baseValue = track.keyFrames_.front();

    Transform sampledValue;
    track.Sample(time_, animation_->GetLength(), looped_, frame, sampledValue);

    if (blendingMode_ == ABM_ADDITIVE)
    {
        // In additive mode, check for output being already initialzed
        if ((track.channelMask_ & output.dirty_).Test(CHANNEL_POSITION))
        {
            const Vector3 delta = sampledValue.position_ - baseValue.position_;
            output.localToParent_.position_ += delta * weight;
        }

        if ((track.channelMask_ & output.dirty_).Test(CHANNEL_ROTATION))
        {
            const Quaternion delta = sampledValue.rotation_ * baseValue.rotation_.Inverse();
            if (isFullWeight)
                output.localToParent_.rotation_ = delta * output.localToParent_.rotation_;
            else
                output.localToParent_.rotation_ = Quaternion::IDENTITY.Slerp(delta, weight) * output.localToParent_.rotation_;
        }

        if ((track.channelMask_ & output.dirty_).Test(CHANNEL_SCALE))
        {
            const Vector3 delta = sampledValue.scale_ - baseValue.scale_;
            output.localToParent_.scale_ += delta * weight;
        }
    }
    else
    {
        // In interpolation mode, disable interpolation if output is not initialzed yet
        if (track.channelMask_.Test(CHANNEL_POSITION))
        {
            if (!isFullWeight && output.dirty_.Test(CHANNEL_POSITION))
                output.localToParent_.position_ = output.localToParent_.position_.Lerp(sampledValue.position_, weight);
            else
            {
                output.dirty_ |= CHANNEL_POSITION;
                output.localToParent_.position_ = sampledValue.position_;
            }
        }

        if (track.channelMask_.Test(CHANNEL_ROTATION))
        {
            if (!isFullWeight && output.dirty_.Test(CHANNEL_ROTATION))
                output.localToParent_.rotation_ = output.localToParent_.rotation_.Slerp(sampledValue.rotation_, weight);
            else
            {
                output.dirty_ |= CHANNEL_ROTATION;
                output.localToParent_.rotation_ = sampledValue.rotation_;
            }
        }

        if (track.channelMask_.Test(CHANNEL_SCALE))
        {
            if (!isFullWeight && output.dirty_.Test(CHANNEL_SCALE))
                output.localToParent_.scale_ = output.localToParent_.scale_.Lerp(sampledValue.scale_, weight);
            else
            {
                output.dirty_ |= CHANNEL_SCALE;
                output.localToParent_.scale_ = sampledValue.scale_;
            }
        }
    }
}

void AnimationState::CalculateAttributeTrack(
    Variant& output, const VariantAnimationTrack& track, unsigned& frame, float baseWeight) const
{
    if (track.keyFrames_.empty())
        return;

    const float weight = baseWeight * track.weight_;
    const bool isFullWeight = Equals(weight, 1.0f);
    const Variant& baseValue = track.keyFrames_.front().value_;

    const Variant sampledValue = track.Sample(time_, animation_->GetLength(), looped_, frame);

    if (blendingMode_ == ABM_ADDITIVE)
    {
        if (!output.IsEmpty())
            output = BlendAdditive(output, sampledValue, baseValue, weight);
    }
    else
    {
        if (!output.IsEmpty() && !isFullWeight)
            output = output.Lerp(sampledValue, weight);
        else
            output = sampledValue;
    }
}

}
