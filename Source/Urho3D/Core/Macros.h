// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
