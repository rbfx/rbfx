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

/// \file

#pragma once

#include <EASTL/unordered_map.h>

#include "../Container/Ptr.h"
#include "../Math/StringHash.h"

namespace Urho3D
{

class Animation;
class AnimationController;
class AnimatedModel;
class Deserializer;
class Node;
class Serializer;
class Serializable;
class Skeleton;
struct AnimationTrack;
struct VariantAnimationTrack;
struct Bone;

/// %Animation blending mode.
enum AnimationBlendMode
{
    // Lerp blending (default)
    ABM_LERP = 0,
    // Additive blending based on difference from bind pose
    ABM_ADDITIVE
};

/// Per-track data of skinned model animation.
/// TODO(animation): Do we want per-bone weights?
struct URHO3D_API ModelAnimationStateTrack
{
    const AnimationTrack* track_{};
    Bone* bone_{};
    WeakPtr<Node> node_;
    unsigned keyFrame_{};
};

/// Per-track data of node model animation.
struct URHO3D_API NodeAnimationStateTrack
{
    const AnimationTrack* track_{};
    WeakPtr<Node> node_;
    unsigned keyFrame_{};
};

/// Per-track data of attribute animation.
struct URHO3D_API AttributeAnimationStateTrack
{
    const VariantAnimationTrack* track_{};
    WeakPtr<Serializable> serializable_;
    unsigned attributeIndex_{};
    StringHash variableName_;
    unsigned keyFrame_{};
};

/// %Animation instance.
class URHO3D_API AnimationState : public RefCounted
{
public:
    /// Construct with animated model and animation pointers.
    AnimationState(AnimationController* controller, AnimatedModel* model, Animation* animation);
    /// Construct with root scene node and animation pointers.
    AnimationState(AnimationController* controller, Node* node, Animation* animation);
    /// Destruct.
    ~AnimationState() override;

    /// Modify tracks. For internal use only.
    /// @{
    bool AreTracksDirty() const;
    void MarkTracksDirty();
    void ClearAllTracks();
    void AddModelTrack(const ModelAnimationStateTrack& track);
    void AddNodeTrack(const NodeAnimationStateTrack& track);
    void AddAttributeTrack(const AttributeAnimationStateTrack& track);
    void OnTracksReady();
    /// @}

    /// Set looping enabled/disabled.
    /// @property
    void SetLooped(bool looped);
    /// Set blending weight.
    /// @property
    void SetWeight(float weight);
    /// Set blending mode.
    /// @property
    void SetBlendMode(AnimationBlendMode mode);
    /// Set time position. Does not fire animation triggers.
    /// @property
    void SetTime(float time);
    /// Set start bone name.
    void SetStartBone(const ea::string& name);
    /// Modify blending weight.
    void AddWeight(float delta);
    /// Modify time position. %Animation triggers will be fired.
    void AddTime(float delta);
    /// Set blending layer.
    /// @property
    void SetLayer(unsigned char layer);

    /// Return animation.
    /// @property
    Animation* GetAnimation() const { return animation_; }

    /// Return animated model this state belongs to (model mode).
    /// @property
    AnimatedModel* GetModel() const;
    /// Return root scene node this state controls (node hierarchy mode).
    /// @property
    Node* GetNode() const;

    /// Return name of start bone.
    const ea::string& GetStartBone() const { return startBone_; }

    /// Return whether weight is nonzero.
    /// @property
    bool IsEnabled() const { return weight_ > 0.0f; }

    /// Return whether looped.
    /// @property
    bool IsLooped() const { return looped_; }

    /// Return blending weight.
    /// @property
    float GetWeight() const { return weight_; }

    /// Return blending mode.
    /// @property
    AnimationBlendMode GetBlendMode() const { return blendingMode_; }

    /// Return time position.
    /// @property
    float GetTime() const { return time_; }

    /// Return animation length.
    /// @property
    float GetLength() const;

    /// Return blending layer.
    /// @property
    unsigned char GetLayer() const { return layer_; }

    /// Apply animation to a skeleton. Transform changes are applied silently, so the model needs to dirty its root model afterward.
    void ApplyModelTracks();
    /// Apply animation to a scene node hierarchy.
    void ApplyNodeTracks();
    /// Apply animation to attributes.
    void ApplyAttributeTracks();

private:
    /// Apply single transformation track to target object. Key frame hint is updated on call.
    void ApplyTransformTrack(const AnimationTrack& track,
        Node* node, Bone* bone, unsigned& frame, float weight, bool silent);
    /// Apply single attribute track to target object. Key frame hint is updated on call.
    void ApplyAttributeTrack(AttributeAnimationStateTrack& stateTrack, float weight);

    /// Owner controller.
    WeakPtr<AnimationController> controller_;
    /// Animated model (model mode).
    WeakPtr<AnimatedModel> model_;
    /// Root scene node (node hierarchy mode).
    WeakPtr<Node> node_;
    /// Animation.
    SharedPtr<Animation> animation_;

    /// Whether the animation state tracks are dirty and should be updated.
    bool tracksDirty_{ true };

    /// Dynamic properties of AnimationState.
    /// @{
    bool looped_{};
    float weight_{};
    float time_{};
    unsigned char layer_{};
    AnimationBlendMode blendingMode_{};
    ea::string startBone_;
    /// @}

    /// Tracks that are actually applied to the objects.
    /// @{
    ea::vector<ModelAnimationStateTrack> modelTracks_;
    ea::vector<NodeAnimationStateTrack> nodeTracks_;
    ea::vector<AttributeAnimationStateTrack> attributeTracks_;
    /// @}
};

using AnimationStateVector = ea::vector<SharedPtr<AnimationState>>;

}
