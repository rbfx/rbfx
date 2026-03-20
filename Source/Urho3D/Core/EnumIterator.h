// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/IteratorRange.h"

#include <EASTL/type_traits.h>

namespace Urho3D
{

/// Iterator that iterates consecutive enum values.
template <class T> class EnumIterator
{
public:
    explicit EnumIterator(T value)
        : value_(value)
    {
    }

    EnumIterator& operator++()
    {
        value_ = static_cast<T>(static_cast<ea::underlying_type_t<T>>(value_) + 1);
        return *this;
    }

    EnumIterator operator++(int)
    {
        EnumIterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(const EnumIterator& rhs) const { return value_ == rhs.value_; }
    bool operator!=(const EnumIterator& rhs) const { return !(*this == rhs); }
    const T& operator*() const { return value_; }
    const T* operator->() const { return &value_; }

private:
    T value_{};
};

/// Returns iterable range of enum values. End value is excluded from iteration.
/// @{
template <class T> IteratorRange<EnumIterator<T>> MakeEnumRange(T begin, T end)
{
    return {EnumIterator<T>{begin}, EnumIterator<T>{end}};
}

template <class T> IteratorRange<EnumIterator<T>> MakeEnumRange(T end = T::Count)
{
    return {EnumIterator<T>{T{}}, EnumIterator<T>{end}};
}
/// @}

} // namespace App
