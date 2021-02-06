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
