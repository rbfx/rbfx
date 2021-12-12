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
#include "../Core/VariantCurve.h"
#include "../Math/Transform.h"

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

    /// Sample value at given time.
    void Sample(float time, float duration, bool isLooped, unsigned& frameIndex, Transform& transform) const;
};

/// Generic variant animation keyframe.
using VariantAnimationKeyFrame = VariantCurvePoint;

/// Generic animation track, stores keyframes of single animatable entity.
struct URHO3D_API VariantAnimationTrack : public VariantCurve {};

}
