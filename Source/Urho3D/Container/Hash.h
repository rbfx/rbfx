//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "Urho3D/Container/Ptr.h"

#include <EASTL/span.h>
#include <EASTL/unordered_map.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>
#include <EASTL/weak_ptr.h>

#include <cstddef>
#include <type_traits>

namespace Urho3D
{

/// Combine hash into result value.
template <class T>
inline void CombineHash(T& result, unsigned hash, ea::enable_if_t<sizeof(T) == 4, int>* = 0)
{
    result ^= hash + 0x9e3779b9 + (result << 6) + (result >> 2);
}

template <class T>
inline void CombineHash(T& result, unsigned long long hash, ea::enable_if_t<sizeof(T) == 8, int>* = 0)
{
    result ^= hash + 0x9e3779b97f4a7c15ull + (result << 6) + (result >> 2);
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

/// Make hash for floating-point variable with zero error tolerance.
inline unsigned MakeHash(float value)
{
    unsigned uintValue{};
    memcpy(&uintValue, &value, sizeof(float));
    CombineHash(uintValue, 0u); // shuffle it a bit
    return uintValue;
}

/// Make hash template helper.
template <class T>
inline unsigned MakeHash(const T& value)
{
    const auto hash = ea::hash<T>{}(value);
    if constexpr (sizeof(hash) > 4)
        return FoldHash(hash);
    else
        return hash;
}

}

namespace eastl
{

template <class T, class Enabled> struct hash;

namespace detail
{
template<typename T, typename = void> struct is_hashable : std::false_type { };
template<typename T> struct is_hashable<T, decltype(void(&T::ToHash))> : std::true_type { };
}

template <typename T>
struct hash<T, typename enable_if<detail::is_hashable<T>::value>::type>
{
    size_t operator()(const T& value) const
    {
        return value.ToHash();
    }
};

template <class T, class U>
struct hash<pair<T, U>>
{
    size_t operator()(const pair<T, U>& value) const
    {
        size_t result = 0;
        Urho3D::CombineHash(result, hash<T>{}(value.first));
        Urho3D::CombineHash(result, hash<U>{}(value.second));
        return result;
    }
};

template <class T, class Allocator>
struct hash<vector<T, Allocator>>
{
    size_t operator()(const vector<T, Allocator>& value) const
    {
        size_t result = 0;
        for (const auto& elem : value)
            Urho3D::CombineHash(result, hash<T>{}(elem));
        return result;
    }
};

template <class Key, class Value, class Hash, class Predicate, class Allocator, bool bCacheHashCode>
struct hash<unordered_map<Key, Value, Hash, Predicate, Allocator, bCacheHashCode>>
{
    size_t operator()(const unordered_map<Key, Value, Hash, Predicate, Allocator, bCacheHashCode>& value) const
    {
        size_t result = 0;
        for (const auto& [key, elem] : value)
        {
            Urho3D::CombineHash(result, hash<Key>{}(key));
            Urho3D::CombineHash(result, hash<Value>{}(elem));
        }
        return result;
    }
};

template <class... T> struct hash<tuple<T...>>
{
    size_t operator()(const tuple<T...>& value) const
    {
        size_t result = 0;
        const auto calculate = [&result](const auto&... args)
        {
            const unsigned hashes[] = {Urho3D::MakeHash(args)...};
            for (const unsigned hash : hashes)
                Urho3D::CombineHash(result, hash);
        };
        ea::apply(calculate, value);
        return result;
    }
};

template <class T>
struct hash<span<T>>
{
    size_t operator()(const span<T>& value) const
    {
        size_t result = 0;
        Urho3D::CombineHash(result, value.size());
        for (const auto& elem : value)
            Urho3D::CombineHash(result, hash<T>{}(elem));
        return result;
    }
};

} // namespace eastl
