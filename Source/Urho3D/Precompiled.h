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

#ifdef __cplusplus

#if URHO3D_PCH

#include <fmt/format.h>

#include <tracy/Tracy.hpp>
#if URHO3D_PROFILING
#include <tracy/client/TracyLock.hpp>
#endif

#include <EASTL/algorithm.h>
#include <EASTL/any.h>
#include <EASTL/array.h>
//#include <EASTL/atomic.h>
#include <EASTL/bitset.h>
#include <EASTL/bitvector.h>
#include <EASTL/chrono.h>
#include <EASTL/deque.h>
#include <EASTL/finally.h>
#include <EASTL/fixed_function.h>
#include <EASTL/fixed_hash_map.h>
#include <EASTL/fixed_hash_set.h>
#include <EASTL/fixed_list.h>
#include <EASTL/fixed_map.h>
#include <EASTL/fixed_set.h>
#include <EASTL/fixed_slist.h>
#include <EASTL/fixed_string.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/functional.h>
#include <EASTL/hash_map.h>
#include <EASTL/hash_set.h>
#include <EASTL/intrusive_hash_map.h>
#include <EASTL/intrusive_hash_set.h>
#include <EASTL/intrusive_list.h>
#include <EASTL/intrusive_ptr.h>
#include <EASTL/iterator.h>
#include <EASTL/linked_array.h>
#include <EASTL/linked_ptr.h>
#include <EASTL/list.h>
#include <EASTL/map.h>
#include <EASTL/numeric.h>
#include <EASTL/numeric_limits.h>
#include <EASTL/optional.h>
#include <EASTL/priority_queue.h>
#include <EASTL/set.h>
#include <EASTL/shared_array.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/slist.h>
#include <EASTL/sort.h>
#include <EASTL/span.h>
#include <EASTL/stack.h>
#include <EASTL/string.h>
#include <EASTL/tuple.h>
#include <EASTL/type_traits.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>
#include <EASTL/utility.h>
#include <EASTL/variant.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector_multimap.h>
#include <EASTL/vector_multiset.h>
#include <EASTL/vector_set.h>
#include <EASTL/weak_ptr.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <exception>
#include <future>
#include <iterator>
#include <thread>
#include <tuple>
#include <type_traits>

#endif

#endif
