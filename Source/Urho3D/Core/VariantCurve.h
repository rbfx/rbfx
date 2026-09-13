// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    /// Sample value at given time without looping and keyframe hint.
    Variant Sample(float time) const;
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
