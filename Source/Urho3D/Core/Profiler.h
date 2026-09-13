// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <tracy/Tracy.hpp>
#if URHO3D_PROFILING
#include <client/TracyLock.hpp>
#endif

namespace Urho3D
{

static const unsigned PROFILER_COLOR_EVENTS = 0xb26d19;
static const unsigned PROFILER_COLOR_RESOURCES = 0x006b82;

void SetProfilerThreadName(const char* name);

}

#define URHO3D_PROFILE_FUNCTION()                   ZoneScopedN(__FUNCTION__)
#define URHO3D_PROFILE_C(name, color)               ZoneScopedNC(name, color)
#define URHO3D_PROFILE(name)                        ZoneScopedN(name)
#define URHO3D_PROFILE_THREAD(name)                 Urho3D::SetProfilerThreadName(name)
#define URHO3D_PROFILE_VALUE(name, value)           TracyPlot(name, value)
#define URHO3D_PROFILE_FRAME()                      FrameMark
#define URHO3D_PROFILE_MESSAGE(txt, len)            TracyMessage(txt, len)
#define URHO3D_PROFILE_ZONENAME(txt, len)           ZoneName(txt, len)
#if URHO3D_PROFILING
#   define URHO3D_PROFILE_SRC_LOCATION(title)       [] () -> const tracy::SourceLocationData* { static const tracy::SourceLocationData srcloc { nullptr, title, __FILE__, __LINE__, 0 }; return &srcloc; }()
#   define URHO3D_PROFILE_MUTEX(name)               ProfiledMutex name{URHO3D_PROFILE_SRC_LOCATION_DATA(#name)}
#else
#   define URHO3D_PROFILE_SRC_LOCATION_DATA(title)
#   define URHO3D_PROFILE_MUTEX(name)               Mutex name{}
#endif
