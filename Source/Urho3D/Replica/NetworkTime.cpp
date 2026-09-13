// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Format.h"
#include "Urho3D/Math/MathDefs.h"
#include "Urho3D/Replica/NetworkTime.h"

namespace Urho3D
{

ea::string NetworkTime::ToString() const
{
    return Format("#{}:{:.2f}", frame_, fraction_);
}

void NetworkTime::Normalize()
{
    while (fraction_ < 0.0)
    {
        --frame_;
        fraction_ += 1.0;
    }

    while (fraction_ >= 1.0)
    {
        ++frame_;
        fraction_ -= 1.0;
    }
}

void NetworkTime::AddDelta(double delta)
{
    const auto deltaInt = static_cast<long long>(delta);
    const double deltaFract = delta - deltaInt;
    frame_ = frame_ + deltaInt;
    fraction_ += deltaFract;
    Normalize();
}

double NetworkTime::GetDelta(const NetworkTime& origin) const
{
    const auto deltaInt = frame_ - origin.frame_;
    const double deltaFract = fraction_ - origin.fraction_;
    return deltaInt + deltaFract;
}

SoftNetworkTime::SoftNetworkTime(
    unsigned updateFrequency, float snapThreshold, float tolerance, float minTimeScale, float maxTimeScale)
    : updateFrequency_{updateFrequency}
    , snapThreshold_{snapThreshold}
    , tolerance_{tolerance}
    , minTimeScale_{minTimeScale}
    , maxTimeScale_{maxTimeScale}
{
}

void SoftNetworkTime::Reset(const NetworkTime& targetTime)
{
    smoothTime_ = targetTime;
}

float SoftNetworkTime::Update(float timeStep, const NetworkTime& targetTime)
{
    const float timeError = (targetTime - smoothTime_) / updateFrequency_ - timeStep;

    if (std::abs(timeError) < tolerance_)
    {
        smoothTime_ += timeStep * updateFrequency_;
        return timeStep;
    }

    if (std::abs(timeError) >= snapThreshold_)
    {
        smoothTime_ = targetTime;
        return timeStep;
    }

    const float dilatedTimeStep = Clamp(timeStep + timeError, timeStep * minTimeScale_, timeStep * maxTimeScale_);
    smoothTime_ += dilatedTimeStep * updateFrequency_;
    return dilatedTimeStep;
}

}
