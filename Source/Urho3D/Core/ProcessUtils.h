// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Container/Str.h"

#include <cstdlib>

namespace Urho3D
{

class Mutex;

#if _WIN32
static const char* DYN_LIB_SUFFIX = ".dll";
#elif __APPLE__
static const char* DYN_LIB_SUFFIX = ".dylib";
#else
static const char* DYN_LIB_SUFFIX = ".so";
#endif

enum class PlatformId
{
    Windows,
    UniversalWindowsPlatform,

    Linux,
    Android,
    RaspberryPi,

    MacOS,
    iOS,
    tvOS,

    Web,

    Count,
    Unknown = Count
};

/// Initialize the FPU to round-to-nearest, single precision mode.
URHO3D_API void InitFPU();
/// Display an error dialog with the specified title and message.
URHO3D_API void ErrorDialog(const ea::string& title, const ea::string& message);
/// Exit the application with an error message to the console.
URHO3D_API void ErrorExit(const ea::string& message = EMPTY_STRING, int exitCode = EXIT_FAILURE);
/// Open a console window.
URHO3D_API void OpenConsoleWindow();
/// Print Unicode text to the console. Will not be printed to the MSVC output window.
URHO3D_API void PrintUnicode(const ea::string& str, bool error = false);
/// Print Unicode text to the console with a newline appended. Will not be printed to the MSVC output window.
URHO3D_API void PrintUnicodeLine(const ea::string& str, bool error = false);
/// Print ASCII text to the console with a newline appended. Uses printf() to allow printing into the MSVC output window.
URHO3D_API void PrintLine(const ea::string& str, bool error = false);
/// Print ASCII text to the console with a newline appended. Uses printf() to allow printing into the MSVC output window.
URHO3D_API void PrintLine(const char* str, bool error = false);
/// Parse arguments from the command line. First argument is by default assumed to be the executable name and is skipped.
URHO3D_API const ea::vector<ea::string>& ParseArguments(const ea::string& cmdLine, bool skipFirstArgument = true);
/// Parse arguments from the command line.
URHO3D_API const ea::vector<ea::string>& ParseArguments(const char* cmdLine);
/// Parse arguments from a wide char command line.
URHO3D_API const ea::vector<ea::string>& ParseArguments(const ea::wstring& cmdLine);
/// Parse arguments from a wide char command line.
URHO3D_API const ea::vector<ea::string>& ParseArguments(const wchar_t* cmdLine);
/// Parse arguments from argc & argv.
URHO3D_API const ea::vector<ea::string>& ParseArguments(int argc, char** argv);
/// Return previously parsed arguments.
URHO3D_API const ea::vector<ea::string>& GetArguments();
/// Read input from the console window. Return empty if no input.
URHO3D_API ea::string GetConsoleInput();
/// Return the runtime platform.
URHO3D_API PlatformId GetPlatform();
/// Return the runtime platform as string, or (?) if not identified.
URHO3D_API ea::string GetPlatformName();
/// Return the number of physical CPU cores.
URHO3D_API unsigned GetNumPhysicalCPUs();
/// Return the number of logical CPUs (different from physical if hyperthreading is used).
URHO3D_API unsigned GetNumLogicalCPUs();
/// Set minidump write location as an absolute path. If empty, uses default (UserProfile/AppData/Roaming/urho3D/crashdumps) Minidumps are only supported on MSVC compiler.
URHO3D_API void SetMiniDumpDir(const ea::string& pathName);
/// Return minidump write location.
URHO3D_API ea::string GetMiniDumpDir();
/// Return the total amount of usable memory in bytes.
URHO3D_API unsigned long long GetTotalMemory();
/// Return the name of the currently logged in user, or (?) if not identified.
URHO3D_API ea::string GetLoginName();
/// Return the name of the running machine.
URHO3D_API ea::string GetHostName();
/// Return the version of the currently running OS, or (?) if not identified.
URHO3D_API ea::string GetOSVersion();
/// Return a random UUID.
URHO3D_API ea::string GenerateUUID();
/// Return current process ID.
URHO3D_API unsigned GetCurrentProcessID();
/// Open a URL/URI in the browser or other appropriate external application.
URHO3D_API bool OpenURL(const ea::string& url);

}
