// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Math/Vector2.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// A named clip sample in an animation blend space.
struct URHO3D_API AnimationBlendSample
{
    ea::string clip;
    Vector2 position;
};

/// One weighted clip produced by BlendSpace evaluation.
struct URHO3D_API AnimationBlendWeight
{
    ea::string clip;
    float weight{};
};

/// Dimension of a blend space used by locomotion and aim systems.
enum class AnimationBlendSpaceDimension
{
    OneD,
    TwoD
};

/// Deterministic inverse-distance blend space for 1D and 2D animation sampling.
class URHO3D_API AnimationBlendSpace
{
public:
    explicit AnimationBlendSpace(AnimationBlendSpaceDimension dimension = AnimationBlendSpaceDimension::TwoD);

    void SetDimension(AnimationBlendSpaceDimension dimension) { dimension_ = dimension; }
    AnimationBlendSpaceDimension GetDimension() const { return dimension_; }
    void SetAxisRange(const Vector2& minimum, const Vector2& maximum);
    const Vector2& GetAxisMinimum() const { return axisMinimum_; }
    const Vector2& GetAxisMaximum() const { return axisMaximum_; }

    bool AddSample(const AnimationBlendSample& sample, ea::string* error = nullptr);
    bool RemoveSample(const ea::string& clip);
    void ClearSamples() { samples_.clear(); }
    const ea::vector<AnimationBlendSample>& GetSamples() const { return samples_; }

    ea::vector<AnimationBlendWeight> Evaluate(const Vector2& input) const;
    ea::vector<AnimationBlendWeight> Evaluate(float input) const { return Evaluate(Vector2(input, 0.0f)); }

private:
    Vector2 ClampInput(const Vector2& input) const;

    AnimationBlendSpaceDimension dimension_;
    Vector2 axisMinimum_{-1.0f, -1.0f};
    Vector2 axisMaximum_{1.0f, 1.0f};
    ea::vector<AnimationBlendSample> samples_;
};

} // namespace Urho3D
