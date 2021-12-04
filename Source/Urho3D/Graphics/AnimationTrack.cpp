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
#include "../IO/ArchiveSerialization.h"

namespace Urho3D
{

namespace
{
const char* interpMethodNames[] = {"None", "Linear", "TensionSpline", "TangentSpline", nullptr};

struct SerializedKeyframe
{
    /// Is spline.
    bool isSplineInterpolation_;

    /// Keyframe time.
    float time_{};
    /// Attribute value.
    Variant value_;

    /// Tangents for cubic spline.
    Variant inTangent_;
    /// Attribute value.
    Variant outTangent_;

    bool hasTangents_ {false};

    bool Serialize(Archive& archive)
    {
        if (!SerializeValue(archive, "time", time_))
            return false;
        if (!SerializeValue(archive, "value", value_))
            return false;
        if (isSplineInterpolation_)
        {
            hasTangents_ = SerializeValue(archive, "in", inTangent_) && SerializeValue(archive, "out", outTangent_);
        }
        return true;
    };
};
bool SerializeValue(Archive& archive, const char* name, SerializedKeyframe& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
        return value.Serialize(archive);
    return false;
}

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
        return (v1.GetQuaternion() * h1 + v2.GetQuaternion() * h2 + t1.GetQuaternion() * h3 + t2.GetQuaternion() * h4).Normalized();

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
        if (interpolation_ == KeyFrameInterpolation::TensionSpline)
        {
            const unsigned numKeyFrames = keyFrames_.size();
            inTangents_.resize(numKeyFrames);

            for (unsigned i = 1; i + 1 < numKeyFrames; ++i)
            {
                inTangents_[i] = SubstractAndMultiply(
                    type_, keyFrames_[i + 1].value_, keyFrames_[i - 1].value_, splineTension_);
            }

            // If spline is not closed, make end point's tangent zero
            if (numKeyFrames <= 2 || keyFrames_[0].value_ != keyFrames_[numKeyFrames - 1].value_)
            {
                inTangents_[0] = Variant(type_);
                if (numKeyFrames >= 2)
                    inTangents_[numKeyFrames - 1] = Variant(type_);
            }
            else
            {
                const Variant tangent = SubstractAndMultiply(
                    type_, keyFrames_[1].value_, keyFrames_[numKeyFrames - 2].value_, splineTension_);
                inTangents_[0] = tangent;
                inTangents_[numKeyFrames - 1] = tangent;
            }

            outTangents_ = inTangents_;
        }
        break;

    case VAR_INT:
    case VAR_INT64:
    case VAR_INTRECT:
    case VAR_INTVECTOR2:
    case VAR_INTVECTOR3:
        // Integer compounds cannot have spline interpolation, fallback to linear.
        if (interpolation_ == KeyFrameInterpolation::TensionSpline || interpolation_ == KeyFrameInterpolation::TangentSpline)
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
        const bool isSplineInterpolation = interpolation_ == KeyFrameInterpolation::TensionSpline || interpolation_ == KeyFrameInterpolation::TangentSpline;
        if (isSplineInterpolation && inTangents_.size() == keyFrames_.size() && outTangents_.size() == keyFrames_.size())
        {
            return InterpolateSpline(type_, keyFrame.value_, nextKeyFrame.value_,
                outTangents_[frameIndex], inTangents_[nextFrameIndex], blendFactor);
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

bool VariantAnimationTrack::Serialize(Archive& archive)
{
    SerializeValue(archive, "name", name_);
    if (archive.IsInput())
        nameHash_ = name_;
    SerializeValue(archive, "baseValue", baseValue_);
    SerializeEnum(archive, "interpolation", interpMethodNames, interpolation_);
    SerializeValue(archive, "splineTension", splineTension_);

    if (auto block = archive.OpenArrayBlock("keyframes", keyFrames_.size()))
    {
        if (archive.IsInput())
        {
            keyFrames_.clear();
            inTangents_.clear();
            outTangents_.clear();
            keyFrames_.reserve(block.GetSizeHint());
            const bool isSplineInterpolation = interpolation_ == KeyFrameInterpolation::TensionSpline ||
                                               interpolation_ == KeyFrameInterpolation::TangentSpline;
            if (isSplineInterpolation)
            {
                inTangents_.reserve(block.GetSizeHint());
                outTangents_.reserve(block.GetSizeHint());
            }
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                SerializedKeyframe kf;
                kf.isSplineInterpolation_ = isSplineInterpolation;
                if (!SerializeValue(archive, "keyframe", kf))
                    return false;
                keyFrames_.push_back(VariantAnimationKeyFrame { kf.time_, kf.value_ });
                if (isSplineInterpolation)
                {
                    inTangents_.push_back(kf.inTangent_);
                    outTangents_.push_back(kf.outTangent_);
                }
            }
            return true;
        }
        else
        {
            const bool isSplineInterpolation = (interpolation_ == KeyFrameInterpolation::TensionSpline ||
                                                interpolation_ == KeyFrameInterpolation::TangentSpline) &&
                                               keyFrames_.size() == inTangents_.size() &&
                                               keyFrames_.size() == outTangents_.size();

            for (unsigned i=0; i<keyFrames_.size(); ++i)
            {
                SerializedKeyframe kf;
                kf.isSplineInterpolation_ = isSplineInterpolation;
                kf.time_ = keyFrames_[i].time_;
                kf.value_ = keyFrames_[i].value_;
                if (isSplineInterpolation)
                {
                    kf.inTangent_ = inTangents_[i];
                    kf.outTangent_ = outTangents_[i];
                }
                if (!SerializeValue(archive, "keyframe", kf))
                    return false;
            }
            return true;
        }
    }
    if (archive.IsInput())
    {
        nameHash_ = name_;



    }
    else
    {
        
    }
    return true;
}

bool SerializeValue(Archive& archive, const char* name, VariantAnimationTrack& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
        return value.Serialize(archive);
    return false;
}

}
