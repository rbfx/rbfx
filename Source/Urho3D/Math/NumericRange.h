// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/algorithm.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/utility.h>

namespace Urho3D
{

/// Numerical range (pair of min and max value). Invalid if flipped.
template <class T> struct NumericRange : ea::pair<T, T>
{
    /// Construct invalid.
    NumericRange()
        : ea::pair<T, T>(ea::numeric_limits<T>::max(), ea::numeric_limits<T>::lowest()){};

    /// Construct valid.
    NumericRange(const T& minValue, const T& maxValue)
        : ea::pair<T, T>(minValue, maxValue)
    {
    }

    /// Return whether the range is valid.
    bool IsValid() const { return this->first <= this->second; }

    /// Return whether the range intersects another.
    bool Intersect(const NumericRange& rhs) const { return this->first <= rhs.second && rhs.first <= this->second; }

    /// Return whether the range contains a value (including borders).
    bool ContainsInclusive(const T& value) const { return this->first <= value && value <= this->second; }

    /// Return whether the range contains a value (excluding borders).
    bool ContainsExclusive(const T& value) const { return this->first < value && value < this->second; }

    /// Accumulate range.
    NumericRange<T>& operator|=(const NumericRange& rhs)
    {
        this->first = ea::min(this->first, rhs.first);
        this->second = ea::max(this->second, rhs.second);
        return *this;
    }

    /// Accumulate range.
    NumericRange<T> operator|(const NumericRange& rhs) const
    {
        auto lhs = *this;
        lhs |= rhs;
        return lhs;
    }

    /// Trim range.
    NumericRange<T>& operator&=(const NumericRange& rhs)
    {
        this->first = ea::max(this->first, rhs.first);
        this->second = ea::min(this->second, rhs.second);
        return *this;
    }

    /// Trim range.
    NumericRange<T> operator&(const NumericRange& rhs) const
    {
        auto lhs = *this;
        lhs &= rhs;
        return lhs;
    }
};

/// Floating-point numerical range.
using FloatRange = NumericRange<float>;

/// Unsigned integer numerical range.
using UintRange = NumericRange<unsigned>;

} // namespace Urho3D
