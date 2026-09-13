// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <tuple>
#include <EASTL/tuple.h>
#include <EASTL/type_traits.h>

namespace Urho3D
{

/// Helper class to get index of type in tuple.
template <class T, class Tuple>
struct IndexInTuple;

template <class T, class... Types>
struct IndexInTuple<T, ea::tuple<T, Types...>> : ea::integral_constant<unsigned, 0> {};

template <class T, class U, class... Types>
struct IndexInTuple<T, ea::tuple<U, Types...>> : ea::integral_constant<unsigned, 1 + IndexInTuple<T, ea::tuple<Types...>>::value> {};

template <class T, class... Types>
struct IndexInTuple<T, std::tuple<Types...>> : IndexInTuple<T, ea::tuple<Types...>> {};

/// Helper class to check if tuple contains type.
template <class T, class Tuple>
struct TupleHasType;

template <class T>
struct TupleHasType<T, ea::tuple<>> : ea::false_type {};

template <class T, class U, class... Types>
struct TupleHasType<T, ea::tuple<U, Types...>> : TupleHasType<T, ea::tuple<Types...>> {};

template <class T, class... Types>
struct TupleHasType<T, ea::tuple<T, Types...>> : ea::true_type {};

template <class T, class... Types>
struct TupleHasType<T, std::tuple<Types...>> : TupleHasType<T, ea::tuple<Types...>> {};

/// Return index of element in tuple and error if not found.
template <class T, class Tuple>
constexpr unsigned IndexInTupleV = IndexInTuple<T, Tuple>::value;

/// Return if tuple contains type.
template <class T, class Tuple>
constexpr bool TupleHasTypeV = TupleHasType<T, Tuple>::value;

}
