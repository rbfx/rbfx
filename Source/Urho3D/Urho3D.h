//
// Copyright (c) 2008-2017 the Urho3D project.
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

// @URHO3D_ENGINE_CONFIG@

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

// Diligent platform defines.
// This is needed so the user can include Diligent-using headers without linking to Diligent-PublicBuildSettings.
// TODO: Can we resolve this at CMake level?
#if URHO3D_PLATFORM_WINDOWS
    #define PLATFORM_WIN32 1
#endif
#if URHO3D_PLATFORM_UNIVERSAL_WINDOWS
    #define PLATFORM_UNIVERSAL_WINDOWS 1
#endif

#if URHO3D_PLATFORM_LINUX
    #define PLATFORM_LINUX 1
#endif
#if URHO3D_PLATFORM_ANDROID
    #define PLATFORM_ANDROID 1
#endif
#if URHO3D_PLATFORM_RASPBERRY_PI
    // As workaround, we use PLATFORM_LINUX for raspberry pi
    #define PLATFORM_LINUX 1
#endif

#if URHO3D_PLATFORM_MACOS
    #define PLATFORM_MACOS 1
#endif
#if URHO3D_PLATFORM_IOS
    #define PLATFORM_IOS 1
#endif
#if URHO3D_PLATFORM_TVOS
    #define PLATFORM_TVOS 1
#endif

#if URHO3D_PLATFORM_WEB
    #define PLATFORM_EMSCRIPTEN 1
#endif
