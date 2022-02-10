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

#include "../Core/Format.h"
#include "../Math/MathDefs.h"
#include "../Replica/NetworkTime.h"

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
