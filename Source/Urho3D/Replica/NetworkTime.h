// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Assert.h"
#include "../Replica/NetworkId.h"

#include <EASTL/numeric_limits.h>
#include <EASTL/string.h>

namespace Urho3D
{

/// Represents network-synchronized time of client and server.
/// Consists of deterministic frame index and approximate sub-frame factor that
/// indicates relative time between the beginning of the frame and the next frame.
/// Sub-frame factor is always in range `[0, 1)`.
/// Overflow of integer frame index is supported.
/// Deltas should be relatively small in order to work as expected: `|delta| < 2kkk`.
class URHO3D_API NetworkTime
{
public:
    NetworkTime() = default;

    explicit NetworkTime(NetworkFrame frame, float subFrame = 0.0f)
        : frame_(frame)
        , fraction_(subFrame)
    {
        Normalize();
    }

    static NetworkTime FromDouble(double frame)
    {
        NetworkTime result;
        result += frame;
        return result;
    }

    NetworkFrame Frame() const { return frame_; }

    float Fraction() const { return fraction_; }

    ea::string ToString() const;

    NetworkTime& operator+=(double rhs)
    {
        AddDelta(rhs);
        return *this;
    }

    NetworkTime& operator-=(double rhs)
    {
        AddDelta(-rhs);
        return *this;
    }

    NetworkTime operator+(double rhs) const
    {
        NetworkTime result = *this;
        result += rhs;
        return result;
    }

    NetworkTime operator-(double rhs) const
    {
        NetworkTime result = *this;
        result -= rhs;
        return result;
    }

    double operator-(const NetworkTime& rhs) const { return GetDelta(rhs); }

    bool operator==(const NetworkTime& rhs) const { return frame_ == rhs.frame_ && fraction_ == rhs.fraction_; }

    bool operator!=(const NetworkTime& rhs) const { return !(*this == rhs); }

private:
    void Normalize();
    void AddDelta(double delta);
    double GetDelta(const NetworkTime& origin) const;

    NetworkFrame frame_{};
    float fraction_{};
};

/// Helper class to smoothly adjust NetworkTime on client.
class SoftNetworkTime
{
public:
    SoftNetworkTime(
        unsigned updateFrequency, float snapThreshold, float tolerance, float minTimeScale, float maxTimeScale);

    /// Reset time unconditionally.
    void Reset(const NetworkTime& targetTime);
    /// Update time, result should be as close to target as possible.
    float Update(float timeStep, const NetworkTime& targetTime);

    const NetworkTime& GetTime() const { return smoothTime_; }

private:
    const unsigned updateFrequency_{};
    const float snapThreshold_{};
    const float tolerance_{};
    const float minTimeScale_{};
    const float maxTimeScale_{};

    NetworkTime smoothTime_;
};


}
