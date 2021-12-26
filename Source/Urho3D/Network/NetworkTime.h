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

/// \file

#pragma once

#include <Urho3D/Urho3D.h>

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

    explicit NetworkTime(unsigned frame, float subFrame = 0.0f)
        : frame_(frame)
        , subFrame_(subFrame)
    {
        Normalize();
    }

    unsigned GetFrame() const { return frame_; }

    float GetSubFrame() const { return subFrame_; }

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

    bool operator==(const NetworkTime& rhs) const { return frame_ == rhs.frame_ && subFrame_ == rhs.subFrame_; }

    bool operator!=(const NetworkTime& rhs) const { return !(*this == rhs); }

private:
    void Normalize();
    void AddDelta(double delta);
    double GetDelta(const NetworkTime& origin) const;

    unsigned frame_{};
    float subFrame_{};
};


}
