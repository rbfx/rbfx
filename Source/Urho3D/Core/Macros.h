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


#if __GNUC__ || __clang__
#   define URHO3D_LIKELY(cond)   __builtin_expect((cond), 1)
#   define URHO3D_UNLIKELY(cond) __builtin_expect((cond), 0)
#else
#   define URHO3D_LIKELY(cond)   (cond)
#   define URHO3D_UNLIKELY(cond) (cond)
#endif

#define URHO3D_ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))

#if __cplusplus >= 201703L
#   define URHO3D_FALLTHROUGH [[fallthrough]]
#else
#   define URHO3D_FALLTHROUGH
#endif

#if _WIN32
#  define URHO3D_STDCALL __stdcall
#else
#  define URHO3D_STDCALL
#endif

#define CONCATENATE(a, b) CONCATENATE_IMPL(a, b)
#define CONCATENATE_IMPL(a, b) a##b

#define TO_STRING(x) TO_STRING_IMPL(x)
#define TO_STRING_IMPL(x) #x
