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

#include "../Container/FlagSet.h"
#include "../Container/KeyFrameSet.h"
#include "../Container/Ptr.h"
#include "../Math/Transform.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

enum AnimationChannel : unsigned char
{
    CHANNEL_NONE = 0x0,
    CHANNEL_POSITION = 0x1,
    CHANNEL_ROTATION = 0x2,
    CHANNEL_SCALE = 0x4,
};
URHO3D_FLAGSET(AnimationChannel, AnimationChannelFlags);

/// Method of interpolation between keyframes.
enum class KeyFrameInterpolation
{
    None,
    Linear,
    Spline,
};

/// Skeletal animation keyframe.
/// TODO: Replace inheritance with composition?
struct AnimationKeyFrame : public Transform
{
    /// Keyframe time.
    float time_{};

    AnimationKeyFrame() = default;
    AnimationKeyFrame(float time, const Vector3& position,
        const Quaternion& rotation = Quaternion::IDENTITY, const Vector3& scale = Vector3::ONE)
        : Transform{ position, rotation, scale }
        , time_(time)
    {
    }
};

/// Skeletal animation track, stores keyframes of a single bone.
/// @fakeref
struct URHO3D_API AnimationTrack : public KeyFrameSet<AnimationKeyFrame>
{
    /// Bone or scene node name.
    ea::string name_;
    /// Name hash.
    StringHash nameHash_;
    /// Bitmask of included data (position, rotation, scale).
    AnimationChannelFlags channelMask_{};
    /// Base transform for additive animations.
    /// If animation is applied to bones, bone initial transform is used instead.
    Transform baseValue_;

    /// Sample value at given time.
    void Sample(float time, float duration, bool isLooped, unsigned& frameIndex, Transform& transform) const;
};

/// Generic variant animation keyframe.
struct VariantAnimationKeyFrame
{
    /// Keyframe time.
    float time_{};
    /// Attribute value.
    Variant value_;
};

/// Generic animation track, stores keyframes of single animatable entity.
struct URHO3D_API VariantAnimationTrack : public KeyFrameSet<VariantAnimationKeyFrame>
{
    /// Annotated recursive name of animatable entity.
    ea::string name_;
    /// Name hash.
    StringHash nameHash_;
    /// Base transform for additive animations.
    Variant baseValue_;
    /// Interpolation mode.
    KeyFrameInterpolation interpolation_{ KeyFrameInterpolation::Linear };
    /// Spline tension for spline interpolation.
    float splineTension_{ 0.5f };

    /// Cached data (never serialized, recalculated on commit).
    /// @{
    VariantType type_{};
    ea::vector<Variant> splineTangents_;
    /// @}

    /// Commit changes and recalculate derived members. May change interpolation mode.
    void Commit();
    /// Sample value at given time.
    Variant Sample(float time, float duration, bool isLooped, unsigned& frameIndex) const;
    /// Return type of animation track. Defined by the type of the first keyframe.
    VariantType GetType() const;
};

/// %Animation trigger point.
struct AnimationTriggerPoint
{
    /// Trigger time.
    float time_{};
    /// Trigger data.
    Variant data_;
};

/// Skeletal animation resource.
/// Don't use bone tracks and generic variant tracks with the same names.
class URHO3D_API Animation : public ResourceWithMetadata
{
    URHO3D_OBJECT(Animation, ResourceWithMetadata);

public:
    /// Construct.
    explicit Animation(Context* context);
    /// Destruct.
    ~Animation() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;
    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;

    /// Set animation name.
    /// @property
    void SetAnimationName(const ea::string& name);
    /// Set animation length.
    /// @property
    void SetLength(float length);
    /// Create and return a track by name. If track by same name already exists, returns the existing.
    AnimationTrack* CreateTrack(const ea::string& name);
    /// Create and return generic variant track by name. If variant track by same name already exists, returns the existing.
    VariantAnimationTrack* CreateVariantTrack(const ea::string& name);
    /// Remove a track by name. Return true if was found and removed successfully. This is unsafe if the animation is currently used in playback.
    bool RemoveTrack(const ea::string& name);
    /// Remove all tracks. This is unsafe if the animation is currently used in playback.
    void RemoveAllTracks();
    /// Set a trigger point at index.
    /// @property{set_triggers}
    void SetTrigger(unsigned index, const AnimationTriggerPoint& trigger);
    /// Add a trigger point.
    void AddTrigger(const AnimationTriggerPoint& trigger);
    /// Add a trigger point.
    void AddTrigger(float time, bool timeIsNormalized, const Variant& data);
    /// Remove a trigger point by index.
    void RemoveTrigger(unsigned index);
    /// Remove all trigger points.
    void RemoveAllTriggers();
    /// Resize trigger point vector.
    /// @property
    void SetNumTriggers(unsigned num);
    /// Clone the animation.
    SharedPtr<Animation> Clone(const ea::string& cloneName = EMPTY_STRING) const;

    /// Return animation name.
    /// @property
    const ea::string& GetAnimationName() const { return animationName_; }

    /// Return animation name hash.
    StringHash GetAnimationNameHash() const { return animationNameHash_; }

    /// Return animation length.
    /// @property
    float GetLength() const { return length_; }

    /// Return all animation tracks.
    const ea::unordered_map<StringHash, AnimationTrack>& GetTracks() const { return tracks_; }

    /// Return number of animation tracks.
    /// @property
    unsigned GetNumTracks() const { return tracks_.size(); }

    /// Return animation track by index.
    AnimationTrack *GetTrack(unsigned index);

    /// Return animation track by name.
    /// @property{get_tracks}
    AnimationTrack* GetTrack(const ea::string& name);
    /// Return animation track by name hash.
    AnimationTrack* GetTrack(StringHash nameHash);

    /// Return generic variant animation tracks.
    /// @{
    const ea::unordered_map<StringHash, VariantAnimationTrack>& GetVariantTracks() const { return variantTracks_; }
    unsigned GetNumVariantTracks() const { return variantTracks_.size(); }
    VariantAnimationTrack* GetVariantTrack(unsigned index);
    VariantAnimationTrack* GetVariantTrack(const ea::string& name);
    VariantAnimationTrack* GetVariantTrack(StringHash nameHash);
    /// @}

    /// Return animation trigger points.
    const ea::vector<AnimationTriggerPoint>& GetTriggers() const { return triggers_; }

    /// Return number of animation trigger points.
    /// @property
    unsigned GetNumTriggers() const { return triggers_.size(); }

    /// Return a trigger point by index.
    AnimationTriggerPoint* GetTrigger(unsigned index);

    /// Set all animation tracks.
    void SetTracks(const ea::vector<AnimationTrack>& tracks);

private:
    /// Class versions (used for serialization)
    /// @{
    static const unsigned legacyVersion = 1; // Fake version for legacy unversioned UANI file
    static const unsigned variantTrackVersion = 2; // VariantAnimationTrack support added here

    static const unsigned currentVersion = variantTrackVersion;
    /// @}

    /// Animation name.
    ea::string animationName_;
    /// Animation name hash.
    StringHash animationNameHash_;
    /// Animation length.
    float length_;
    /// Animation tracks.
    ea::unordered_map<StringHash, AnimationTrack> tracks_;
    /// Generic variant animation tracks.
    ea::unordered_map<StringHash, VariantAnimationTrack> variantTracks_;
    /// Animation trigger points.
    ea::vector<AnimationTriggerPoint> triggers_;
};

}
