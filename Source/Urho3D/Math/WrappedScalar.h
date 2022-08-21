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

#include "../Math/MathDefs.h"

namespace Urho3D
{

/// Range between two WrappedScalar values.
/// Preserves direction of change.
template <class T>
class WrappedScalarRange
{
public:
    WrappedScalarRange(T value, T minValue, T maxValue)
        : min_(minValue)
        , max_(maxValue)
        , begin_(value)
        , end_(value)
        , numWraps_(0)
    {
    }

    WrappedScalarRange(T beginValue, T endValue, T minValue, T maxValue, int numWraps)
        : min_(minValue)
        , max_(maxValue)
        , begin_(beginValue)
        , end_(endValue)
        , numWraps_(numWraps)
    {
    }

    T Begin() const { return begin_; }
    T End() const { return end_; }
    T Min() const { return min_; }
    T Max() const { return max_; }
    bool IsEmpty() const { return begin_ == end_; }

    bool ContainsInclusive(T value) const { return ContainsInternal(value, true, true); }
    bool ContainsExclusive(T value) const { return ContainsInternal(value, false, false); }
    bool ContainsExcludingBegin(T value) const { return ContainsInternal(value, false, true); }
    bool ContainsExcludingEnd(T value) const { return ContainsInternal(value, true, false); }

    bool operator==(const WrappedScalarRange& rhs) const
    {
        return min_ == rhs.min_
            && max_ == rhs.max_
            && begin_ == rhs.begin_
            && end_ == rhs.end_
            && numWraps_ == rhs.numWraps_;
    }

    bool operator!=(const WrappedScalarRange& rhs) const { return !(*this == rhs); }

private:
    bool ContainsInternal(T value, bool includeBegin, bool includeEnd) const
    {
        if (value < min_ || value > max_)
            return false;

        const T lowest = ea::min(begin_, end_);
        const T highest = ea::max(begin_, end_);

        if (numWraps_ == 0)
        {
            // min  begin    end   max
            // |    |--------|     |
            //   or
            // min  end      begin max
            // |    |--------|     |
            return (value >= lowest && value <= highest)
                && (includeBegin || value != begin_)
                && (includeEnd || value != end_);
        }
        else if ((numWraps_ == 1 && end_ < begin_) || (numWraps_ == -1 && end_ > begin_))
        {
            // min  end    begin   max
            // |----|      |-------|
            //   or
            // min  begin  end     max
            // |----|      |-------|
            return (value <= lowest || value >= highest)
                && (includeBegin || value != begin_)
                && (includeEnd || value != end_);
        }
        else
        {
            // min  end    begin   max
            // |----|======|-------|
            return true;
        }
    }

    T min_{};
    T max_{};
    T begin_{};
    T end_{};
    int numWraps_{};
};

/// Wrapped value between min and max boundaries.
template <class T>
class WrappedScalar
{
public:
    WrappedScalar() = default;

    WrappedScalar(T value, T minValue, T maxValue)
        : min_(ea::min(minValue, maxValue))
        , max_(ea::max(minValue, maxValue))
        , value_(Clamp(value, min_, max_))
    {
    }

    /// Reset value.
    void Set(T value)
    {
        value_ = Clamp(value, min_, max_);
    }

    /// Add delta to the scalar, wrapping value at the boundaries. Return the range of the change.
    WrappedScalarRange<T> UpdateWrapped(T delta)
    {
        if (delta == 0 || static_cast<double>(max_ - min_) < M_LARGE_EPSILON)
            return {value_, min_, max_};

        int numWraps = 0;

        const auto oldValue = value_;
        while (delta > 0)
        {
            value_ += delta;
            if (value_ < max_)
                delta = 0;
            else
            {
                delta = value_ - max_;
                value_ = min_;
                ++numWraps;
            }
        }

        while (delta < 0)
        {
            value_ += delta;
            if (value_ > min_)
                delta = 0;
            else
            {
                delta = value_ - min_;
                value_ = max_;
                --numWraps;
            }
        }

        return {oldValue, value_, min_, max_, numWraps};
    }

    /// Add delta to the scalar, clamping value at the boundaries. Return the range of the change. Optionally returns out-of-bounds range instead of clamped one.
    WrappedScalarRange<T> UpdateClamped(T delta, bool returnOutOfBounds = false)
    {
        const auto oldValue = value_;
        value_ = Clamp(value_ + delta, min_, max_);
        return {oldValue, returnOutOfBounds ? oldValue + delta : value_, min_, max_, 0};
    }

    /// Clamp boundaries.
    WrappedScalar<T> MinMaxClamped(T minValue, T maxValue) const
    {
        return {value_, ea::max(minValue, min_), ea::min(maxValue, max_)};
    }

    T Value() const { return value_; }
    T Min() const { return min_; }
    T Max() const { return max_; }

    bool operator==(const WrappedScalar<T>& rhs) const
    {
        return min_ == rhs.min_
            && max_ == rhs.max_
            && value_ == rhs.value_;
    }

    bool operator!=(const WrappedScalar<T>& rhs) const { return !(*this == rhs); }

private:
    T min_{-M_LARGE_VALUE};
    T max_{M_LARGE_VALUE};
    T value_{};
};

}
