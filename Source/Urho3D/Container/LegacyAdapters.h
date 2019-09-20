//
// Copyright (c) 2017-2019 the Rokas Kupstys project.
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

#ifdef URHO3D_CONTAINER_ADAPTERS

#include <EASTL/sort.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/utility.h>
#include <EASTL/vector.h>

namespace Urho3D
{

template <class T>
using Vector = ea::vector<T>;

template <class T>
using PODVector = ea::vector<T>;

template <class T>
using RandomAccessIterator = typename ea::vector<T>::iterator;

template <class T>
using RandomAccessConstIterator = typename ea::vector<T>::const_iterator;

using String = ea::string;

template <class T, class U>
using HashMap = ea::unordered_map<T, U>;

template <class T>
using HashSet = ea::unordered_set<T>;

template <class T, class U>
using Pair = ea::pair<T, U>;

template <class T, class U>
Pair<T, U> MakePair(T&& first, U&& second) { return ea::make_pair(ea::forward<T>(first), ea::forward<U>(second)); }

template <class Iterator>
void Sort(Iterator first, Iterator last) { ea::sort(first, last); }

}

#endif
