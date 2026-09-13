// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/utility.h>

namespace Urho3D
{

/// Helper class to iterate over the range.
template <class T>
struct IteratorRange : public ea::pair<T, T>
{
    /// Construct empty.
    IteratorRange() = default;
    /// Construct valid.
    IteratorRange(const T& begin, const T& end) : ea::pair<T, T>(begin, end) {}
};

/// Make iterator range.
template <class T> IteratorRange<T> MakeIteratorRange(const T& begin, const T& end) { return { begin, end }; }

template <class T> T begin(const IteratorRange<T>& range) { return range.first; }
template <class T> T end(const IteratorRange<T>& range) { return range.second; }

}
