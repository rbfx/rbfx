// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"
#include "Urho3D/Container/KeyFrameSet.h"
#include "Urho3D/Core/VariantCurve.h"
#include "Urho3D/Graphics/Skeleton.h"
#include "Urho3D/Math/Transform.h"

#include <EASTL/span.h>

namespace Urho3D
{

/// Skeletal animation keyframe.
/// TODO: Replace inheritance with composition?
struct AnimationKeyFrame : public Transform
{
    /// Keyframe time.
    float time_{};

    AnimationKeyFrame() = default;
    AnimationKeyFrame(float time, const Vector3& position, const Quaternion& rotation = Quaternion::IDENTITY,
        const Vector3& scale = Vector3::ONE)
        : Transform{position, rotation, scale}
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
    /// Weight of the position channel.
    float positionWeight_{1.0f};
    /// Weight of the rotation channel.
    float rotationWeight_{1.0f};
    /// Weight of the scale channel.
    float scaleWeight_{1.0f};

    /// Sample value at given time.
    void Sample(float time, float duration, bool isLooped, unsigned& frameIndex, Transform& transform) const;
    /// Return whether the track is looped, i.e. the first and the last keyframes have the same value.
    bool IsLooped(
        float positionThreshold = 0.001f, float rotationThreshold = 0.001f, float scaleThreshold = 0.001f) const;

    /// Create track from separate position, rotation and scale tracks. Keyframe times are merged within epsilon.
    void CreateMerged(AnimationChannelFlags channels, ea::span<const ea::pair<float, Vector3>> positionTrack,
        ea::span<const ea::pair<float, Quaternion>> rotationTrack, ea::span<const ea::pair<float, Vector3>> scaleTrack,
        float epsilon);
};

/// Generic variant animation keyframe.
using VariantAnimationKeyFrame = VariantCurvePoint;

/// Generic animation track, stores keyframes of single animatable entity.
struct URHO3D_API VariantAnimationTrack : public VariantCurve
{
    /// Weight of the track.
    float weight_{1.0f};

    /// Return whether the track is looped, i.e. the first and the last keyframes have the same value.
    bool IsLooped() const;
};

} // namespace Urho3D
