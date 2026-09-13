// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include <stdint.h>

#if URHO3D_PROFILING
#if _WIN32
#   include <windows.h>
#else
#   include "pthread.h"
#endif
#endif
#include "Profiler.h"

namespace Urho3D
{

void SetProfilerThreadName(const char* name)
{
#if URHO3D_PROFILING
    tracy::SetThreadName(name);
#endif
}

}
