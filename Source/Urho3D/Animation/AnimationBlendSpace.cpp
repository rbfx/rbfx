// SPDX-License-Identifier: MIT

#include "AnimationBlendSpace.h"

#include <algorithm>
#include <cmath>

namespace Urho3D
{

AnimationBlendSpace::AnimationBlendSpace(AnimationBlendSpaceDimension dimension)
    : dimension_(dimension)
{
}

void AnimationBlendSpace::SetAxisRange(const Vector2& minimum, const Vector2& maximum)
{
    axisMinimum_ = Vector2(Min(minimum.x_, maximum.x_), Min(minimum.y_, maximum.y_));
    axisMaximum_ = Vector2(Max(minimum.x_, maximum.x_), Max(minimum.y_, maximum.y_));
}

bool AnimationBlendSpace::AddSample(const AnimationBlendSample& sample, ea::string* error)
{
    if (sample.clip.empty())
    {
        if (error)
            *error = "Blend sample clip cannot be empty.";
        return false;
    }
    for (const AnimationBlendSample& existing : samples_)
    {
        if (existing.clip == sample.clip)
        {
            if (error)
                *error = "Blend sample clip already exists.";
            return false;
        }
    }
    samples_.push_back(sample);
    return true;
}

bool AnimationBlendSpace::RemoveSample(const ea::string& clip)
{
    const auto iter = std::find_if(samples_.begin(), samples_.end(),
        [&](const AnimationBlendSample& sample) { return sample.clip == clip; });
    if (iter == samples_.end())
        return false;
    samples_.erase(iter);
    return true;
}

ea::vector<AnimationBlendWeight> AnimationBlendSpace::Evaluate(const Vector2& input) const
{
    ea::vector<AnimationBlendWeight> result;
    if (samples_.empty())
        return result;

    const Vector2 clamped = ClampInput(input);
    float exactDistance = M_INFINITY;
    const AnimationBlendSample* exactSample = nullptr;
    ea::vector<float> inverseDistances;
    inverseDistances.reserve(samples_.size());
    float inverseSum = 0.0f;

    for (const AnimationBlendSample& sample : samples_)
    {
        const float dx = dimension_ == AnimationBlendSpaceDimension::OneD
            ? clamped.x_ - sample.position.x_ : clamped.x_ - sample.position.x_;
        const float dy = dimension_ == AnimationBlendSpaceDimension::OneD
            ? 0.0f : clamped.y_ - sample.position.y_;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < exactDistance)
        {
            exactDistance = distanceSquared;
            exactSample = &sample;
        }
        if (distanceSquared <= M_EPSILON)
        {
            result.push_back({sample.clip, 1.0f});
            return result;
        }

        const float inverse = 1.0f / std::sqrt(distanceSquared);
        inverseDistances.push_back(inverse);
        inverseSum += inverse;
    }

    if (!exactSample || inverseSum <= M_EPSILON)
        return result;
    result.reserve(samples_.size());
    for (unsigned i = 0; i < samples_.size(); ++i)
        result.push_back({samples_[i].clip, inverseDistances[i] / inverseSum});
    return result;
}

Vector2 AnimationBlendSpace::ClampInput(const Vector2& input) const
{
    return Vector2(Clamp(input.x_, axisMinimum_.x_, axisMaximum_.x_),
        Clamp(input.y_, axisMinimum_.y_, axisMaximum_.y_));
}

} // namespace Urho3D
