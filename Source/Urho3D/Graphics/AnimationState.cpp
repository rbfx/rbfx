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
        return oldValue.GetIntVector2() + VectorRoundToInt(static_cast<Vector2>(newValue.GetIntVector2() - baseValue.GetIntVector2()) * weight);

    case VAR_INTVECTOR3:
        return oldValue.GetIntVector3() + VectorRoundToInt(static_cast<Vector3>(newValue.GetIntVector3() - baseValue.GetIntVector3()) * weight);

    default:
        return oldValue;
    }
}

}

AnimationState::AnimationState(AnimationController* controller, AnimatedModel* model, Animation* animation) :
    controller_(controller),
    model_(model),
    animation_(animation)
{
}

AnimationState::AnimationState(AnimationController* controller, Node* node, Animation* animation) :
    controller_(controller),
    node_(node),
    animation_(animation)
{
}

AnimationState::~AnimationState() = default;

bool AnimationState::AreTracksDirty() const
{
    return tracksDirty_;
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
    if (model_)
        model_->MarkAnimationDirty();
}

void AnimationState::SetLooped(bool looped)
{
    looped_ = looped;
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

void AnimationState::SetBlendMode(AnimationBlendMode mode)
{
    if (blendingMode_ != mode)
    {
        blendingMode_ = mode;
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

void AnimationState::SetStartBone(const ea::string& name)
{
    if (!animation_)
        return;

    if (name != startBone_)
    {
        startBone_ = name;
        MarkTracksDirty();
        if (model_)
            model_->MarkAnimationDirty();
    }
}

void AnimationState::AddWeight(float delta)
{
    if (delta == 0.0f)
        return;

    SetWeight(GetWeight() + delta);
}

void AnimationState::AddTime(float delta)
{
    if (!animation_ || (!model_ && !node_))
        return;

    float length = animation_->GetLength();
    if (delta == 0.0f || length == 0.0f)
        return;

    bool sendFinishEvent = false;

    float oldTime = GetTime();
    float time = oldTime + delta;
    if (looped_)
    {
        while (time >= length)
        {
            time -= length;
            sendFinishEvent = true;
        }
        while (time < 0.0f)
        {
            time += length;
            sendFinishEvent = true;
        }
    }

    SetTime(time);

    if (!looped_)
    {
        if (delta > 0.0f && oldTime < length && GetTime() == length)
            sendFinishEvent = true;
        else if (delta < 0.0f && oldTime > 0.0f && GetTime() == 0.0f)
            sendFinishEvent = true;
    }

    // Process finish event
    if (sendFinishEvent)
    {
        using namespace AnimationFinished;

        WeakPtr<AnimationState> self(this);
        WeakPtr<Node> senderNode(model_ ? model_->GetNode() : node_.Get());

        VariantMap& eventData = senderNode->GetEventDataMap();
        eventData[P_NODE] = senderNode;
        eventData[P_ANIMATION] = animation_;
        eventData[P_NAME] = animation_->GetAnimationName();
        eventData[P_LOOPED] = looped_;

        // Note: this may cause arbitrary deletion of animation states, including the one we are currently processing
        senderNode->SendEvent(E_ANIMATIONFINISHED, eventData);
        if (senderNode.Expired() || self.Expired())
            return;
    }

    // Process animation triggers
    if (animation_->GetNumTriggers())
    {
        bool wrap = false;

        if (delta > 0.0f)
        {
            if (oldTime > time)
            {
                oldTime -= length;
                wrap = true;
            }
        }
        if (delta < 0.0f)
        {
            if (time > oldTime)
            {
                time -= length;
                wrap = true;
            }
        }
        if (oldTime > time)
            ea::swap(oldTime, time);

        const ea::vector<AnimationTriggerPoint>& triggers = animation_->GetTriggers();
        for (auto i = triggers.begin(); i != triggers.end(); ++i)
        {
            float frameTime = i->time_;
            if (looped_ && wrap)
                frameTime = fmodf(frameTime, length);

            if (oldTime <= frameTime && time > frameTime)
            {
                using namespace AnimationTrigger;

                WeakPtr<AnimationState> self(this);
                WeakPtr<Node> senderNode(model_ ? model_->GetNode() : node_.Get());

                VariantMap& eventData = senderNode->GetEventDataMap();
                eventData[P_NODE] = senderNode;
                eventData[P_ANIMATION] = animation_;
                eventData[P_NAME] = animation_->GetAnimationName();
                eventData[P_TIME] = i->time_;
                eventData[P_DATA] = i->data_;

                // Note: this may cause arbitrary deletion of animation states, including the one we are currently processing
                senderNode->SendEvent(E_ANIMATIONTRIGGER, eventData);
                if (senderNode.Expired() || self.Expired())
                    return;
            }
        }
    }
}

void AnimationState::SetLayer(unsigned char layer)
{
    if (layer != layer_)
    {
        layer_ = layer;
        controller_->MarkAnimationStateOrderDirty();
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

void AnimationState::ApplyModelTracks()
{
    if (!animation_ || !IsEnabled())
        return;

    for (ModelAnimationStateTrack& stateTrack : modelTracks_)
    {
        // Do not apply if the bone has animation disabled
        if (!stateTrack.bone_->animated_)
            continue;

        ApplyTransformTrack(*stateTrack.track_, stateTrack.node_, stateTrack.bone_, stateTrack.keyFrame_, weight_, true);
    }
}

void AnimationState::ApplyNodeTracks()
{
    if (!animation_ || !IsEnabled())
        return;

    for (NodeAnimationStateTrack& stateTrack : nodeTracks_)
    {
        ApplyTransformTrack(*stateTrack.track_, stateTrack.node_, nullptr, stateTrack.keyFrame_, weight_, false);
    }
}

void AnimationState::ApplyAttributeTracks()
{
    if (!animation_ || !IsEnabled())
        return;

    for (AttributeAnimationStateTrack& stateTrack : attributeTracks_)
    {
        ApplyAttributeTrack(stateTrack, weight_);
    }
}

void AnimationState::ApplyTransformTrack(const AnimationTrack& track,
    Node* node, Bone* bone, unsigned& frame, float weight, bool silent)
{
    if (track.keyFrames_.empty() || !node)
        return;

    const AnimationChannelFlags channelMask = track.channelMask_;

    Transform newTransform;
    track.Sample(time_, animation_->GetLength(), looped_, frame, newTransform);

    if (blendingMode_ == ABM_ADDITIVE) // not ABM_LERP
    {
        if (channelMask & CHANNEL_POSITION)
        {
            const Vector3& base = bone ? bone->initialPosition_ : track.baseValue_.position_;
            const Vector3 delta = newTransform.position_ - base;
            newTransform.position_ = node->GetPosition() + delta * weight;
        }
        if (channelMask & CHANNEL_ROTATION)
        {
            const Quaternion& base = bone ? bone->initialRotation_ : track.baseValue_.rotation_;
            const Quaternion delta = newTransform.rotation_ * base.Inverse();
            newTransform.rotation_ = (delta * node->GetRotation()).Normalized();
            if (!Equals(weight, 1.0f))
                newTransform.rotation_ = node->GetRotation().Slerp(newTransform.rotation_, weight);
        }
        if (channelMask & CHANNEL_SCALE)
        {
            const Vector3& base = bone ? bone->initialScale_ : track.baseValue_.scale_;
            const Vector3 delta = newTransform.scale_ - base;
            newTransform.scale_ = node->GetScale() + delta * weight;
        }
    }
    else
    {
        if (!Equals(weight, 1.0f)) // not full weight
        {
            if (channelMask & CHANNEL_POSITION)
                newTransform.position_ = node->GetPosition().Lerp(newTransform.position_, weight);
            if (channelMask & CHANNEL_ROTATION)
                newTransform.rotation_ = node->GetRotation().Slerp(newTransform.rotation_, weight);
            if (channelMask & CHANNEL_SCALE)
                newTransform.scale_ = node->GetScale().Lerp(newTransform.scale_, weight);
        }
    }

    if (silent)
    {
        if (channelMask & CHANNEL_POSITION)
            node->SetPositionSilent(newTransform.position_);
        if (channelMask & CHANNEL_ROTATION)
            node->SetRotationSilent(newTransform.rotation_);
        if (channelMask & CHANNEL_SCALE)
            node->SetScaleSilent(newTransform.scale_);
    }
    else
    {
        if (channelMask & CHANNEL_POSITION)
            node->SetPosition(newTransform.position_);
        if (channelMask & CHANNEL_ROTATION)
            node->SetRotation(newTransform.rotation_);
        if (channelMask & CHANNEL_SCALE)
            node->SetScale(newTransform.scale_);
    }
}

void AnimationState::ApplyAttributeTrack(AttributeAnimationStateTrack& stateTrack, float weight)
{
    const VariantAnimationTrack& track = *stateTrack.track_;
    Serializable* serializable = stateTrack.serializable_;
    if (track.keyFrames_.empty() || !serializable)
        return;

    Variant newValue = track.Sample(time_, animation_->GetLength(), looped_, stateTrack.keyFrame_);

    // Apply blending
    if (blendingMode_ == ABM_ADDITIVE || !Equals(weight, 1.0f))
    {
        Variant oldValue;
        if (stateTrack.variableName_ == StringHash{})
            oldValue = serializable->GetAttribute(stateTrack.attributeIndex_);
        else
        {
            assert(dynamic_cast<Node*>(serializable));
            auto node = static_cast<Node*>(serializable);
            oldValue = node->GetVar(stateTrack.variableName_);
        }

        if (blendingMode_ == ABM_ADDITIVE)
            newValue = BlendAdditive(oldValue, newValue, track.baseValue_, weight);
        else
            newValue = oldValue.Lerp(newValue, weight);
    }

    // Apply final value
    if (stateTrack.variableName_ == StringHash{})
        serializable->SetAttribute(stateTrack.attributeIndex_, newValue);
    else
    {
        assert(dynamic_cast<Node*>(serializable));
        auto node = static_cast<Node*>(serializable);
        node->SetVar(stateTrack.variableName_, newValue);
    }
}

}
