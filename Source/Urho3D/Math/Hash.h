// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Urho3D.h"

#include <EASTL/functional.h>
#include <EASTL/type_traits.h>

#include <cstring>

namespace Urho3D
{

/// Combine hash into result value.
template <class T> //
inline void CombineHash(T& result, unsigned hash, ea::enable_if_t<sizeof(T) == 4, int>* = 0)
{
    result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
}

template <class T> //
inline void CombineHash(T& result, unsigned long long hash, ea::enable_if_t<sizeof(T) == 8, int>* = 0)
{
    result ^= hash + 0x9e3779b97f4a7c15ull + (result << 6) + (result >> 2);
}

/// Finalize hash to make hash distribution more uniform and destroy linearity.
/// https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
inline void FinalizeHash(unsigned& value)
{
    value ^= value >> 16;
    value *= 0x85ebca6b;
    value ^= value >> 13;
    value *= 0xc2b2ae35;
    value ^= value >> 16;
}

inline void FinalizeHash(unsigned long long& value)
{
    value ^= value >> 33;
    value *= 0xff51afd7ed558ccdull;
    value ^= value >> 33;
    value *= 0xc4ceb9fe1a85ec53ull;
    value ^= value >> 33;
}

/// Fold 64-bit hash to 32-bit.
inline unsigned FoldHash(unsigned long long value)
{
    const auto lowValue = static_cast<unsigned>(value);
    const auto highValue = static_cast<unsigned>(value >> 32ull);
    if (highValue == 0)
        return lowValue;

    auto result = lowValue;
    CombineHash(result, highValue);
    return result;
}

/// Make hash for `float` with zero error tolerance.
inline unsigned MakeHash(float value)
{
    unsigned uintValue{};
    std::memcpy(&uintValue, &value, sizeof(float));
    return uintValue;
}

/// Make hash for `double` with zero error tolerance.
inline unsigned MakeHash(double value)
{
    unsigned long long ulongValue{};
    std::memcpy(&ulongValue, &value, sizeof(double));
    return FoldHash(ulongValue);
}

/// Make hash template helper.
template <class T> inline unsigned MakeHash(const T& value)
{
    const auto hash = ea::hash<T>{}(value);
    if constexpr (sizeof(hash) > 4)
        return FoldHash(hash);
    else
        return hash;
}

} // namespace Urho3D
