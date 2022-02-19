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

#include "../Core/ProcessUtils.h"

#if UWP
#include <wrl.h>
#endif

#if defined(_WIN32) && !defined(URHO3D_WIN32_CONSOLE)
#include "../Core/MiniDump.h"
#include "../WindowsSupport.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#endif

// Define a platform-specific main function, which in turn executes the user-defined function

#if defined(UWP)
#define URHO3D_DEFINE_MAIN(function) \
extern "C" int SDL_main(int argc, char** argv) \
{ \
    Urho3D::ParseArguments(argc, argv); \
    return function; \
} \
extern "C" URHO3D_API int SDL_WinRTRunApp(int(*)(int, char**), void*); \
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) \
{ \
    return SDL_WinRTRunApp(&SDL_main, nullptr); \
}
// MSVC debug mode: use memory leak reporting
#elif defined(_MSC_VER) && defined(_DEBUG) && !defined(URHO3D_WIN32_CONSOLE)
#define URHO3D_DEFINE_MAIN(function) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) \
{ \
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF); \
    Urho3D::ParseArguments(GetCommandLineW()); \
    return function; \
}
// MSVC release mode: write minidump on crash
#elif defined(_MSC_VER) && defined(URHO3D_MINIDUMPS) && !defined(URHO3D_WIN32_CONSOLE)
#define URHO3D_DEFINE_MAIN(function) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) \
{ \
    Urho3D::ParseArguments(GetCommandLineW()); \
    int exitCode; \
    __try \
    { \
        exitCode = function; \
    } \
    __except(Urho3D::WriteMiniDump("Urho3D", GetExceptionInformation())) \
    { \
    } \
    return exitCode; \
}
// Other Win32 or minidumps disabled: just execute the function
#elif defined(_WIN32) && !defined(URHO3D_WIN32_CONSOLE)
#define URHO3D_DEFINE_MAIN(function) \
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) \
{ \
    Urho3D::ParseArguments(GetCommandLineW()); \
    return function; \
}
// Android or iOS or tvOS: use SDL_main
#elif defined(__ANDROID__) || defined(IOS) || defined(TVOS)
#define URHO3D_DEFINE_MAIN(function) \
extern "C" __attribute__((visibility("default"))) int SDL_main(int argc, char** argv); \
int SDL_main(int argc, char** argv) \
{ \
    Urho3D::ParseArguments(argc, argv); \
    return function; \
}
// Linux or OS X: use main
#else
#define URHO3D_DEFINE_MAIN(function) \
int main(int argc, char** argv) \
{ \
    Urho3D::ParseArguments(argc, argv); \
    return function; \
}
#endif
