// Copyright (c) 2008-2017 the Urho3D project.
// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#ifdef _WIN32

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#endif

#define URHO3D_EXPORT_API __declspec(dllexport)
#define URHO3D_IMPORT_API __declspec(dllimport)

#ifdef URHO3D_STATIC
#  define URHO3D_API
#  define URHO3D_NO_EXPORT
#else
#  ifndef URHO3D_API
#    ifdef URHO3D_EXPORTS
        /* We are building this library */
#      define URHO3D_API URHO3D_EXPORT_API
#    else
        /* We are using this library */
#      define URHO3D_API URHO3D_IMPORT_API
#    endif
#  endif

#  ifndef URHO3D_NO_EXPORT
#    define URHO3D_NO_EXPORT
#  endif
#endif

#ifndef URHO3D_DEPRECATED
#  define URHO3D_DEPRECATED __declspec(deprecated)
#endif

#ifndef URHO3D_DEPRECATED_EXPORT
#  define URHO3D_DEPRECATED_EXPORT URHO3D_API URHO3D_DEPRECATED
#endif

#ifndef URHO3D_DEPRECATED_NO_EXPORT
#  define URHO3D_DEPRECATED_NO_EXPORT URHO3D_NO_EXPORT URHO3D_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define URHO3D_NO_DEPRECATED
#endif

#else

#define URHO3D_EXPORT_API __attribute__((visibility("default")))
#define URHO3D_IMPORT_API __attribute__((visibility("default")))

#ifdef URHO3D_STATIC
#ifndef URHO3D_API
#  define URHO3D_API
#endif
#  define URHO3D_NO_EXPORT
#else
#  define URHO3D_API URHO3D_EXPORT_API
#  ifndef URHO3D_NO_EXPORT
#    define URHO3D_NO_EXPORT __attribute__((visibility("hidden")))
#  endif
#endif

#ifndef URHO3D_DEPRECATED
#  define URHO3D_DEPRECATED __attribute__ ((__deprecated__))
#endif

#ifndef URHO3D_DEPRECATED_EXPORT
#  define URHO3D_DEPRECATED_EXPORT URHO3D_API URHO3D_DEPRECATED
#endif

#ifndef URHO3D_DEPRECATED_NO_EXPORT
#  define URHO3D_DEPRECATED_NO_EXPORT URHO3D_NO_EXPORT URHO3D_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define URHO3D_NO_DEPRECATED
#endif

#endif

// Disable SSE if compiler does not support it.
#if defined(URHO3D_SSE) && !defined(__SSE2__) && (!defined(_M_IX86_FP) || _M_IX86_FP < 2)
#   undef URHO3D_SSE
#endif

// Platform identification macros.
#if defined(__ANDROID__)
    #define URHO3D_PLATFORM_ANDROID 1
#elif defined(IOS)
    #define URHO3D_PLATFORM_IOS 1
#elif defined(TVOS)
    #define URHO3D_PLATFORM_TVOS 1
#elif defined(__APPLE__)
    #define URHO3D_PLATFORM_MACOS 1
#elif UWP
    #define URHO3D_PLATFORM_UNIVERSAL_WINDOWS 1
#elif defined(_WIN32)
    #define URHO3D_PLATFORM_WINDOWS 1
#elif defined(RPI)
    #define URHO3D_PLATFORM_RASPBERRY_PI 1
#elif defined(__EMSCRIPTEN__)
    #define URHO3D_PLATFORM_WEB 1
#elif defined(__linux__)
    #define URHO3D_PLATFORM_LINUX 1
#else
    #error Unsupported platform
#endif

namespace Urho3D
{
}
