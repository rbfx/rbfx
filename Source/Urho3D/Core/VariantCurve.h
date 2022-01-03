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

#pragma once

#include "../Container/KeyFrameSet.h"
#include "../Core/Variant.h"

namespace Urho3D
{

class Archive;
class ArchiveBlock;

/// Method of interpolation between keyframes or curve points.
enum class KeyFrameInterpolation
{
    /// No interpolation, value is snapped to next one.
    None,
    /// Linear interpolation between values. Spherical interpolation for quaternions.
    Linear,
    /// Cubic spline with constant tension.
    TensionSpline,
    /// Cubic spline with explicit in and out tangents.
    TangentSpline
};

/// Generic variant animation keyframe or curve point.
/// @note Tangents (if present) are stored separately to save memory.
struct VariantCurvePoint
{
    /// Input scalar. Time for animation, may be something else.
    float time_{};
    /// Output value. Should have the same type for all points in curve.
    Variant value_;

    unsigned ToHash() const;
    bool operator==(const VariantCurvePoint& rhs) const { return time_ == rhs.time_ && value_ == rhs.value_; }
    bool operator!=(const VariantCurvePoint& rhs) const { return !(*this == rhs); }
};

/// Curve of Variant values sampled by scalar values.
class URHO3D_API VariantCurve : public KeyFrameSet<VariantCurvePoint>
{
public:
    /// Empty curve. Returns empty Variant on sampling.
    static const VariantCurve EMPTY;

    /// Commit changes and recalculate derived members. May change interpolation mode.
    /// Sample should be called only on committed curve!
    void Commit();
    /// Sample value at given time.
    Variant Sample(float time, float duration, bool isLooped, unsigned& frameIndex) const;
    /// Return type of animation track. Defined by the type of the first keyframe.
    VariantType GetType() const;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    /// @name Utility functions
    /// @{
    unsigned ToHash() const;
    bool operator==(const VariantCurve& rhs) const;
    bool operator!=(const VariantCurve& rhs) const { return !(*this == rhs); }
    /// @}

    // TODO: Make private
public:
    /// Annotated name of the curve. May have special meaning for the user.
    ea::string name_;
    StringHash nameHash_;

    /// Interpolation mode.
    KeyFrameInterpolation interpolation_{ KeyFrameInterpolation::Linear };

    /// Spline tension for spline interpolation.
    float splineTension_{ 0.5f };
    /// Tangents for cubic spline. Recalculated on commit for tension spline.
    ea::vector<Variant> inTangents_;
    ea::vector<Variant> outTangents_;

    /// Type of values, deduced from key frames.
    VariantType type_{};
};

}
