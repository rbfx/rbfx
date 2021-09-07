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

#include "../Graphics/AnimationTrack.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

Variant InterpolateSpline(VariantType type,
    const Variant& v1, const Variant& v2, const Variant& t1, const Variant& t2, float t)
{
    const float tt = t * t;
    const float ttt = t * tt;

    const float h1 = 2.0f * ttt - 3.0f * tt + 1.0f;
    const float h2 = -2.0f * ttt + 3.0f * tt;
    const float h3 = ttt - 2.0f * tt + t;
    const float h4 = ttt - tt;

    switch (type)
    {
    case VAR_FLOAT:
        return v1.GetFloat() * h1 + v2.GetFloat() * h2 + t1.GetFloat() * h3 + t2.GetFloat() * h4;

    case VAR_VECTOR2:
        return v1.GetVector2() * h1 + v2.GetVector2() * h2 + t1.GetVector2() * h3 + t2.GetVector2() * h4;

    case VAR_VECTOR3:
        return v1.GetVector3() * h1 + v2.GetVector3() * h2 + t1.GetVector3() * h3 + t2.GetVector3() * h4;

    case VAR_VECTOR4:
        return v1.GetVector4() * h1 + v2.GetVector4() * h2 + t1.GetVector4() * h3 + t2.GetVector4() * h4;

    case VAR_QUATERNION:
        return v1.GetQuaternion() * h1 + v2.GetQuaternion() * h2 + t1.GetQuaternion() * h3 + t2.GetQuaternion() * h4;

    case VAR_COLOR:
        return v1.GetColor() * h1 + v2.GetColor() * h2 + t1.GetColor() * h3 + t2.GetColor() * h4;

    case VAR_DOUBLE:
        return v1.GetDouble() * h1 + v2.GetDouble() * h2 + t1.GetDouble() * h3 + t2.GetDouble() * h4;

    default:
        return v1;
    }
}

Variant SubstractAndMultiply(VariantType type, const Variant& v1, const Variant& v2, float t)
{
    switch (type)
    {
    case VAR_FLOAT:
        return (v1.GetFloat() - v2.GetFloat()) * t;

    case VAR_VECTOR2:
        return (v1.GetVector2() - v2.GetVector2()) * t;

    case VAR_VECTOR3:
        return (v1.GetVector3() - v2.GetVector3()) * t;

    case VAR_VECTOR4:
        return (v1.GetVector4() - v2.GetVector4()) * t;

    case VAR_QUATERNION:
        return (v1.GetQuaternion() - v2.GetQuaternion()) * t;

    case VAR_COLOR:
        return (v1.GetColor() - v2.GetColor()) * t;

    case VAR_DOUBLE:
        return (v1.GetDouble() - v2.GetDouble()) * t;

    default:
        return Variant(type);
    }
}

}

void AnimationTrack::Sample(float time, float duration, bool isLooped, unsigned& frameIndex, Transform& value) const
{
    float blendFactor{};
    unsigned nextFrameIndex{};
    GetKeyFrames(time, duration, isLooped, frameIndex, nextFrameIndex, blendFactor);

    const AnimationKeyFrame& keyFrame = keyFrames_[frameIndex];
    const AnimationKeyFrame& nextKeyFrame = keyFrames_[nextFrameIndex];

    if (blendFactor >= M_EPSILON)
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_.Lerp(nextKeyFrame.position_, blendFactor);
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_.Slerp(nextKeyFrame.rotation_, blendFactor);
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_.Lerp(nextKeyFrame.scale_, blendFactor);
    }
    else
    {
        if (channelMask_ & CHANNEL_POSITION)
            value.position_ = keyFrame.position_;
        if (channelMask_ & CHANNEL_ROTATION)
            value.rotation_ = keyFrame.rotation_;
        if (channelMask_ & CHANNEL_SCALE)
            value.scale_ = keyFrame.scale_;
    }
}

void VariantAnimationTrack::Commit()
{
    type_ = GetType();

    switch (type_)
    {
    case VAR_FLOAT:
    case VAR_VECTOR2:
    case VAR_VECTOR3:
    case VAR_VECTOR4:
    case VAR_QUATERNION:
    case VAR_COLOR:
    case VAR_DOUBLE:
        // Floating point compounds may have any interpolation type.
        // Calculate tangents if spline interpolation is used.
        if (interpolation_ == KeyFrameInterpolation::Spline)
        {
            const unsigned numKeyFrames = keyFrames_.size();
            splineTangents_.resize(numKeyFrames);

            for (unsigned i = 1; i + 1 < numKeyFrames; ++i)
            {
                splineTangents_[i] = SubstractAndMultiply(
                    type_, keyFrames_[i + 1].value_, keyFrames_[i - 1].value_, splineTension_);
            }

            // If spline is not closed, make end point's tangent zero
            if (numKeyFrames <= 2 || keyFrames_[0].value_ != keyFrames_[numKeyFrames - 1].value_)
            {
                splineTangents_[0] = Variant(type_);
                if (numKeyFrames >= 2)
                    splineTangents_[numKeyFrames - 1] = Variant(type_);
            }
            else
            {
                const Variant tangent = SubstractAndMultiply(
                    type_, keyFrames_[1].value_, keyFrames_[numKeyFrames - 2].value_, splineTension_);
                splineTangents_[0] = tangent;
                splineTangents_[numKeyFrames - 1] = tangent;
            }
        }
        break;

    case VAR_INT:
    case VAR_INT64:
    case VAR_INTRECT:
    case VAR_INTVECTOR2:
    case VAR_INTVECTOR3:
        // Integer compounds cannot have spline interpolation, fallback to linear.
        if (interpolation_ == KeyFrameInterpolation::Spline)
            interpolation_ = KeyFrameInterpolation::Linear;
        break;

    default:
        // Other types don't support interpolation at all, fallback to none.
        interpolation_ = KeyFrameInterpolation::None;
        break;
    }
}

Variant VariantAnimationTrack::Sample(float time, float duration, bool isLooped, unsigned& frameIndex) const
{
    float blendFactor{};
    unsigned nextFrameIndex{};
    GetKeyFrames(time, duration, isLooped, frameIndex, nextFrameIndex, blendFactor);

    const VariantAnimationKeyFrame& keyFrame = keyFrames_[frameIndex];
    const VariantAnimationKeyFrame& nextKeyFrame = keyFrames_[nextFrameIndex];

    if (blendFactor >= M_EPSILON)
    {
        if (interpolation_ == KeyFrameInterpolation::Spline && splineTangents_.size() == keyFrames_.size())
        {
            return InterpolateSpline(type_, keyFrame.value_, nextKeyFrame.value_,
                splineTangents_[frameIndex], splineTangents_[nextFrameIndex], blendFactor);
        }
        else if (interpolation_ == KeyFrameInterpolation::Linear)
            return keyFrame.value_.Lerp(nextKeyFrame.value_, blendFactor);
    }

    return keyFrame.value_;
}

VariantType VariantAnimationTrack::GetType() const
{
    return keyFrames_.empty() ? VAR_NONE : keyFrames_[0].value_.GetType();
}

}
