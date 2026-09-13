// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <type_traits>

namespace Urho3D
{

#ifndef URHO3D_TYPE_TRAIT
/// Helper macro that creates type trait. Expression should be any well-formed C++ expression over template type U.
#define URHO3D_TYPE_TRAIT(name, expr) \
    template <typename U> struct name \
    { \
        template<typename T> static decltype((expr), std::true_type{}) func(std::remove_reference_t<T>*); \
        template<typename T> static std::false_type func(...); \
        using type = decltype(func<U>(nullptr)); \
        static constexpr bool value{ type::value }; \
    }
#endif

namespace Detail
{

template <class> struct MemberFunctionTraits;

template <class Return, class Object, class... Args>
struct MemberFunctionTraits<Return (Object::*)(Args...)>
{
    using ReturnType = Return;
    using ObjectType = Object;
};

template <class Return, class Object, class... Args>
struct MemberFunctionTraits<Return (Object::*)(Args...) const>
{
    using ReturnType = Return;
    using ObjectType = Object;
};

}

/// Helper type trait that extracts object type from member function pointer.
template <class T> using MemberFunctionObject = typename Detail::MemberFunctionTraits<T>::ObjectType;

}
