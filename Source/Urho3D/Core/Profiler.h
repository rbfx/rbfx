//
// Copyright (c) 2018 Rokas Kupstys
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

#include "../Container/Str.h"
#include "../Core/Thread.h"
#include "../Core/Timer.h"

#if URHO3D_PROFILING
#   include <tracy/Tracy.hpp>
#endif

namespace Urho3D
{

static const unsigned PROFILER_COLOR_EVENTS = 0xb26d19;
static const unsigned PROFILER_COLOR_RESOURCES = 0x006b82;

}

#if URHO3D_PROFILING
#   define URHO3D_PROFILE_C(name, color)          ZoneScopedNC(name, color)
#   define URHO3D_PROFILE(name)                   ZoneScopedN(name)
#if _WIN32
#   define URHO3D_PROFILE_THREAD(name)            tracy::SetThreadName(GetCurrentThread(), name);
#else
#   define URHO3D_PROFILE_THREAD(name)            tracy::SetThreadName(pthread_self(), name);
#endif
#   define URHO3D_PROFILE_VALUE(name, value)      TracyPlot(name, value)
#   define URHO3D_PROFILE_FRAME()                 FrameMark
#   define URHO3D_PROFILE_MESSAGE(txt, len)       TracyMessage(txt, len)
#   define URHO3D_PROFILE_ZONENAME(txt, len)      ZoneName(txt, len)
#else
#   define URHO3D_PROFILE(...)
#   define URHO3D_PROFILE_THREAD(...)
#   define URHO3D_PROFILE_VALUE(...)
#   define URHO3D_PROFILE_FUNCTION(...)
#   define URHO3D_PROFILE_FRAME()
#   define URHO3D_PROFILE_MESSAGE(txt, len)
#   define URHO3D_PROFILE_ZONENAME(txt, len)
#endif

