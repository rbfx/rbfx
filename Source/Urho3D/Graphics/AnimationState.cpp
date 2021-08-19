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
}

void AnimationState::AddModelTrack(const ModelAnimationStateTrack& track)
{
    modelTracks_.push_back(track);
}

void AnimationState::AddNodeTrack(const NodeAnimationStateTrack& track)
{
    nodeTracks_.push_back(track);
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

void AnimationState::ApplyTransformTrack(const AnimationTrack& track,
    Node* node, Bone* bone, unsigned& frame, float weight, bool silent)
{
    if (track.keyFrames_.empty() || !node)
        return;

    track.GetKeyFrameIndex(time_, frame);

    // Check if next frame to interpolate to is valid, or if wrapping is needed (looping animation only)
    unsigned nextFrame = frame + 1;
    bool interpolate = true;
    if (nextFrame >= track.keyFrames_.size())
    {
        if (!looped_)
        {
            nextFrame = frame;
            interpolate = false;
        }
        else
            nextFrame = 0;
    }

    const AnimationKeyFrame* keyFrame = &track.keyFrames_[frame];
    const AnimationChannelFlags channelMask = track.channelMask_;

    Vector3 newPosition;
    Quaternion newRotation;
    Vector3 newScale;

    if (interpolate)
    {
        const AnimationKeyFrame* nextKeyFrame = &track.keyFrames_[nextFrame];
        float timeInterval = nextKeyFrame->time_ - keyFrame->time_;
        if (timeInterval < 0.0f)
            timeInterval += animation_->GetLength();
        float t = timeInterval > 0.0f ? (time_ - keyFrame->time_) / timeInterval : 1.0f;

        if (channelMask & CHANNEL_POSITION)
            newPosition = keyFrame->position_.Lerp(nextKeyFrame->position_, t);
        if (channelMask & CHANNEL_ROTATION)
            newRotation = keyFrame->rotation_.Slerp(nextKeyFrame->rotation_, t);
        if (channelMask & CHANNEL_SCALE)
            newScale = keyFrame->scale_.Lerp(nextKeyFrame->scale_, t);
    }
    else
    {
        if (channelMask & CHANNEL_POSITION)
            newPosition = keyFrame->position_;
        if (channelMask & CHANNEL_ROTATION)
            newRotation = keyFrame->rotation_;
        if (channelMask & CHANNEL_SCALE)
            newScale = keyFrame->scale_;
    }

    if (blendingMode_ == ABM_ADDITIVE) // not ABM_LERP
    {
        if (channelMask & CHANNEL_POSITION)
        {
            const Vector3 delta = bone ? (newPosition - bone->initialPosition_) : newPosition;
            newPosition = node->GetPosition() + delta * weight;
        }
        if (channelMask & CHANNEL_ROTATION)
        {
            const Quaternion delta = bone ? (newRotation * bone->initialRotation_.Inverse()) : newRotation;
            newRotation = (delta * node->GetRotation()).Normalized();
            if (!Equals(weight, 1.0f))
                newRotation = node->GetRotation().Slerp(newRotation, weight);
        }
        if (channelMask & CHANNEL_SCALE)
        {
            const Vector3 delta = bone ? (newScale - bone->initialScale_) : newScale;
            newScale = node->GetScale() + delta * weight;
        }
    }
    else
    {
        if (!Equals(weight, 1.0f)) // not full weight
        {
            if (channelMask & CHANNEL_POSITION)
                newPosition = node->GetPosition().Lerp(newPosition, weight);
            if (channelMask & CHANNEL_ROTATION)
                newRotation = node->GetRotation().Slerp(newRotation, weight);
            if (channelMask & CHANNEL_SCALE)
                newScale = node->GetScale().Lerp(newScale, weight);
        }
    }

    if (silent)
    {
        if (channelMask & CHANNEL_POSITION)
            node->SetPositionSilent(newPosition);
        if (channelMask & CHANNEL_ROTATION)
            node->SetRotationSilent(newRotation);
        if (channelMask & CHANNEL_SCALE)
            node->SetScaleSilent(newScale);
    }
    else
    {
        if (channelMask & CHANNEL_POSITION)
            node->SetPosition(newPosition);
        if (channelMask & CHANNEL_ROTATION)
            node->SetRotation(newRotation);
        if (channelMask & CHANNEL_SCALE)
            node->SetScale(newScale);
    }
}

}
