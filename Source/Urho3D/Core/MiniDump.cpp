// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#if defined(_MSC_VER) && defined(URHO3D_MINIDUMPS)

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/ProcessUtils.h"

#include <cstdio>
#include <io.h>
#include <fcntl.h>
#include <time.h>
#include <windows.h>
#include <dbghelp.h>

namespace Urho3D
{

URHO3D_API int WriteMiniDump(const char* applicationName, void* exceptionPointers)
{
    static bool miniDumpWritten = false;

    // In case of recursive or repeating exceptions, only write the dump once
    /// \todo This function should not allocate any dynamic memory
    if (miniDumpWritten)
        return EXCEPTION_EXECUTE_HANDLER;

    miniDumpWritten = true;

    MINIDUMP_EXCEPTION_INFORMATION info;
    info.ThreadId = GetCurrentThreadId();
    info.ExceptionPointers = (EXCEPTION_POINTERS*)exceptionPointers;
    info.ClientPointers = TRUE;

    static time_t sysTime;
    time(&sysTime);
    const char* dateTime = ctime(&sysTime);
    ea::string dateTimeStr = ea::string(dateTime);
    dateTimeStr.replace("\n", "");
    dateTimeStr.replace(":", "");
    dateTimeStr.replace("/", "");
    dateTimeStr.replace(' ', '_');

    ea::string miniDumpDir = GetMiniDumpDir();
    ea::string miniDumpName = miniDumpDir + ea::string(applicationName) + "_" + dateTimeStr + ".dmp";

    CreateDirectoryW(MultiByteToWide(miniDumpDir).c_str(), nullptr);
    HANDLE file = CreateFileW(MultiByteToWide(miniDumpName).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, 0, nullptr);

    BOOL success = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpWithDataSegs, &info, nullptr, nullptr);
    CloseHandle(file);

    if (success)
        ErrorDialog(applicationName, "An unexpected error occurred. A minidump was generated to " + miniDumpName);
    else
        ErrorDialog(applicationName, "An unexpected error occurred. Could not write minidump.");

    return EXCEPTION_EXECUTE_HANDLER;
}

}

#endif
