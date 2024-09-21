//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Container/Hash.h"
#include "../Core/VariantCurve.h"
#include "../IO/Archive.h"
#include "../IO/ArchiveSerialization.h"

namespace Urho3D
{

namespace
{

const char* const keyFrameInterpolationNames[] =
{
    "None",
    "Linear",
    "Spline",
    "CubicSpline",
    nullptr
};

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

unsigned VariantCurvePoint::ToHash() const
{
    unsigned hash = 0;
    CombineHash(hash, MakeHash(time_));
    CombineHash(hash, MakeHash(value_));
    return hash;
}

const VariantCurve VariantCurve::EMPTY;

void VariantCurve::Commit()
{
    nameHash_ = name_;
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

Variant VariantCurve::Sample(float time, float duration, bool isLooped, unsigned& frameIndex) const
{
    float blendFactor{};
    unsigned nextFrameIndex{};
    GetKeyFrames(time, duration, isLooped, frameIndex, nextFrameIndex, blendFactor);

    const VariantCurvePoint& keyFrame = keyFrames_[frameIndex];
    const VariantCurvePoint& nextKeyFrame = keyFrames_[nextFrameIndex];

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

Variant VariantCurve::Sample(float time) const
{
    unsigned frameIndex{};
    return Sample(time, 0.0f, false, frameIndex);
}

VariantType VariantCurve::GetType() const
{
    return keyFrames_.empty() ? VAR_NONE : keyFrames_[0].value_.GetType();
}

void VariantCurve::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "name", name_);
    SerializeValue(archive, "type", type_);
    SerializeEnum(archive, "interpolation", interpolation_, keyFrameInterpolationNames);
    SerializeValue(archive, "splineTension", splineTension_);

    if (interpolation_ == KeyFrameInterpolation::TangentSpline)
    {
        SerializeVectorTieAsObjects(archive, "keyframes", ea::tie(keyFrames_, inTangents_, outTangents_), "keyframe",
            [&](Archive& archive, const char* name, ea::tuple<VariantCurvePoint&, Variant&, Variant&> value)
        {
            auto block = archive.OpenUnorderedBlock(name);

            auto& keyFrame = ea::get<0>(value);
            auto& inTangent = ea::get<1>(value);
            auto& outTangent = ea::get<2>(value);
            SerializeValue(archive, "time", keyFrame.time_);
            SerializeVariantAsType(archive, "value", keyFrame.value_, type_);
            SerializeVariantAsType(archive, "in", inTangent, type_);
            SerializeVariantAsType(archive, "out", outTangent, type_);
        });
    }
    else
    {
        SerializeVectorAsObjects(archive, "keyframes", keyFrames_, "keyframe",
            [&](Archive& archive, const char* name, VariantCurvePoint& value)
        {
            auto block = archive.OpenUnorderedBlock(name);
            SerializeValue(archive, "time", value.time_);
            SerializeVariantAsType(archive, "value", value.value_, type_);
        });
    }

    if (archive.IsInput())
        Commit();
}

unsigned VariantCurve::ToHash() const
{
    unsigned hash = 0;
    CombineHash(hash, MakeHash(name_));
    CombineHash(hash, MakeHash(interpolation_));
    CombineHash(hash, MakeHash(splineTension_));
    CombineHash(hash, MakeHash(inTangents_));
    CombineHash(hash, MakeHash(outTangents_));
    CombineHash(hash, MakeHash(keyFrames_));
    return hash;
}

bool VariantCurve::operator==(const VariantCurve& rhs) const
{
    return name_ == rhs.name_
        && interpolation_ == rhs.interpolation_
        && splineTension_ == rhs.splineTension_
        && inTangents_ == rhs.inTangents_
        && outTangents_ == rhs.outTangents_
        && keyFrames_ == rhs.keyFrames_;
}

}
