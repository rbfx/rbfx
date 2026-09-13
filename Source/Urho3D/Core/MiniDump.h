// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

namespace Urho3D
{

#if defined(_MSC_VER) && defined(URHO3D_MINIDUMPS)
/// Write a minidump. Needs to be called from within a structured exception handler.
URHO3D_API int WriteMiniDump(const char* applicationName, void* exceptionPointers);
#endif

}

