//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Thread.h"
#include "../Core/Profiler.h"
#include "../Engine/EngineEvents.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/IOEvents.h"
#include "../IO/Log.h"
#if URHO3D_SYSTEMUI
#   include "../SystemUI/Console.h"
#endif

#ifdef __ANDROID__
#include <SDL/SDL_rwops.h>
#endif

#ifndef MINI_URHO
#include <SDL/SDL_filesystem.h>
#endif

#include <sys/stat.h>
#include <cstdio>

#ifdef _WIN32
#ifndef _MSC_VER
#define _WIN32_IE 0x501
#endif
#include <windows.h>
#include <shellapi.h>
#include <direct.h>
#include <shlobj.h>
#include <sys/types.h>
#include <sys/utime.h>
#else
#include <dirent.h>
#include <cerrno>
#include <unistd.h>
#include <utime.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <spawn.h>
#define MAX_PATH 256
#endif

#if defined(__APPLE__) && !IOS
#include <mach-o/dyld.h>
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#endif

extern "C"
{
#ifdef __ANDROID__
const char* SDL_Android_GetFilesDir();
char** SDL_Android_GetFileList(const char* path, int* count);
void SDL_Android_FreeFileList(char*** array, int* count);
#elif defined(IOS) || defined(TVOS)
const char* SDL_IOS_GetResourceDir();
const char* SDL_IOS_GetDocumentsDir();
#endif
}

#include "../DebugNew.h"

namespace Urho3D
{

ea::string specifiedExecutableFile;

int DoSystemCommand(const ea::string& commandLine, bool redirectToLog, Context* context)
{
#if defined(TVOS) || defined(IOS) || defined(UWP)
    return -1;
#else
#if !defined(__EMSCRIPTEN__) && !defined(MINI_URHO)
    if (!redirectToLog)
#endif
        return system(commandLine.c_str());

#if !defined(__EMSCRIPTEN__) && !defined(MINI_URHO)
    // Get a platform-agnostic temporary file name for stderr redirection
    ea::string stderrFilename;
    ea::string adjustedCommandLine(commandLine);
    char* prefPath = SDL_GetPrefPath("urho3d", "temp");
    if (prefPath)
    {
        stderrFilename = ea::string(prefPath) + "command-stderr";
        adjustedCommandLine += " 2>" + stderrFilename;
        SDL_free(prefPath);
    }

#ifdef _MSC_VER
    #define popen _popen
    #define pclose _pclose
#endif

    // Use popen/pclose to capture the stdout and stderr of the command
    FILE* file = popen(adjustedCommandLine.c_str(), "r");
    if (!file)
        return -1;

    // Capture the standard output stream
    char buffer[0x2000];
    while (!feof(file))
    {
        if (fgets(buffer, sizeof(buffer), file))
        {
            ea::string text(buffer);
            const char array[] = { ' ', '\t', '\r', '\n', 0 };
            text.erase(text.find_last_not_of(array) + 1);
            URHO3D_LOGINFO(text);
        }
    }
    int exitCode = pclose(file);

    // Capture the standard error stream
    if (!stderrFilename.empty())
    {
        SharedPtr<File> errFile(new File(context, stderrFilename, FILE_READ));
        while (!errFile->IsEof())
        {
            unsigned numRead = errFile->Read(buffer, sizeof(buffer));
            if (numRead)
                URHO3D_LOGERROR(ea::string(buffer, numRead));
        }
    }

    return exitCode;
#endif
#endif
}

enum SystemRunFlag : unsigned
{
    SR_DEFAULT,
    SR_WAIT_FOR_EXIT,
    SR_READ_OUTPUT = 1u << 1u | SR_WAIT_FOR_EXIT,
};
URHO3D_FLAGSET(SystemRunFlag, SystemRunFlags);

int DoSystemRun(const ea::string& fileName, const ea::vector<ea::string>& arguments, SystemRunFlags flags, ea::string& output)
{
#if defined(TVOS) || defined(IOS) || (defined(__ANDROID__) && __ANDROID_API__ < 28) || defined(UWP)
    return -1;
#else
    ea::string fixedFileName = GetNativePath(fileName);

#ifdef _WIN32
    // Add .exe extension if no extension defined
    if (GetExtension(fixedFileName).empty())
        fixedFileName += ".exe";

    ea::string commandLine = "\"" + fixedFileName + "\"";
    for (unsigned i = 0; i < arguments.size(); ++i)
        commandLine += " \"" + arguments[i] + "\"";

    STARTUPINFOW startupInfo{};
    PROCESS_INFORMATION processInfo{};
    startupInfo.cb = sizeof(startupInfo);

    ea::wstring commandLineW = MultiByteToWide(commandLine);
    DWORD processFlags = 0;
    if (flags & SR_WAIT_FOR_EXIT)
        // If we are waiting for process result we are likely reading stdout, in that case we probably do not want to see a console window.
        processFlags = CREATE_NO_WINDOW;

    HANDLE pipeRead = 0, pipeWrite = 0;
    if (flags & SR_READ_OUTPUT)
    {
        SECURITY_ATTRIBUTES attr;
        attr.nLength = sizeof(SECURITY_ATTRIBUTES);
        attr.bInheritHandle = FALSE;
        attr.lpSecurityDescriptor = NULL;
        if (!CreatePipe(&pipeRead, &pipeWrite, &attr, 0))
            return -1;

        if (!SetHandleInformation(pipeRead, HANDLE_FLAG_INHERIT, 0))
            return -1;

        if (!SetHandleInformation(pipeWrite, HANDLE_FLAG_INHERIT, 1))
            return -1;

        DWORD mode = PIPE_NOWAIT;
        if (!SetNamedPipeHandleState(pipeRead, &mode, nullptr, nullptr))
            return -1;

        startupInfo.hStdOutput = pipeWrite;
        startupInfo.hStdError = pipeWrite;
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
    }

    if (!CreateProcessW(nullptr, (wchar_t*)commandLineW.c_str(), nullptr, nullptr, TRUE, processFlags, nullptr, nullptr, &startupInfo, &processInfo))
        return -1;

    DWORD exitCode = 0;
    if (flags & SR_WAIT_FOR_EXIT)
    {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        GetExitCodeProcess(processInfo.hProcess, &exitCode);
    }

    if (flags & SR_READ_OUTPUT)
    {
        char buf[1024];
        for (;;)
        {
            DWORD bytesRead = 0;
            if (!ReadFile(pipeRead, buf, sizeof(buf), &bytesRead, nullptr) || !bytesRead)
                break;
            auto err = GetLastError();

            unsigned start = output.length();
            output.resize(start + bytesRead);
            memcpy(&output[start], buf, bytesRead);
        }

        CloseHandle(pipeWrite);
        CloseHandle(pipeRead);
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return exitCode;
#else

    int desc[2];
    if (flags & SR_READ_OUTPUT)
    {
        if (pipe(desc) == -1)
            return -1;
        fcntl(desc[0], F_SETFL, O_NONBLOCK);
        fcntl(desc[1], F_SETFL, O_NONBLOCK);
    }

    ea::vector<const char*> argPtrs;
    argPtrs.push_back(fixedFileName.c_str());
    for (unsigned i = 0; i < arguments.size(); ++i)
        argPtrs.push_back(arguments[i].c_str());
    argPtrs.push_back(nullptr);

    pid_t pid = 0;
    posix_spawn_file_actions_t actions{};
    posix_spawn_file_actions_init(&actions);
    if (flags & SR_READ_OUTPUT)
    {
        posix_spawn_file_actions_addclose(&actions, STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, desc[1], STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&actions, STDERR_FILENO);
        posix_spawn_file_actions_adddup2(&actions, desc[1], STDERR_FILENO);
    }
    posix_spawnp(&pid, fixedFileName.c_str(), &actions, nullptr, (char**)&argPtrs[0], environ);
    posix_spawn_file_actions_destroy(&actions);

    if (pid > 0)
    {
        int exitCode = 0;
        if (flags & SR_WAIT_FOR_EXIT)
            waitpid(pid, &exitCode, 0);

        if (flags & SR_READ_OUTPUT)
        {
            char buf[1024];
            for (;;)
            {
                ssize_t bytesRead = read(desc[0], buf, sizeof(buf));
                if (bytesRead <= 0)
                    break;

                unsigned start = output.length();
                output.resize(start + bytesRead);
                memcpy(&output[start], buf, bytesRead);
            }
            close(desc[0]);
            close(desc[1]);
        }
        return exitCode;
    }
    else
        return -1;
#endif
#endif
}

/// Base class for async execution requests.
class AsyncExecRequest : public Thread
{
public:
    /// Construct.
    explicit AsyncExecRequest(unsigned& requestID) :
        requestID_(requestID)
    {
        // Increment ID for next request
        ++requestID;
        if (requestID == M_MAX_UNSIGNED)
            requestID = 1;
    }

    /// Return request ID.
    unsigned GetRequestID() const { return requestID_; }

    /// Return exit code. Valid when IsCompleted() is true.
    int GetExitCode() const { return exitCode_; }

    /// Return completion status.
    bool IsCompleted() const { return completed_; }

protected:
    /// Request ID.
    unsigned requestID_{};
    /// Exit code.
    int exitCode_{};
    /// Completed flag.
    volatile bool completed_{};
};

/// Async system command operation.
class AsyncSystemCommand : public AsyncExecRequest
{
public:
    /// Construct and run.
    AsyncSystemCommand(unsigned requestID, const ea::string& commandLine) :
        AsyncExecRequest(requestID),
        commandLine_(commandLine)
    {
        Run();
    }

    /// The function to run in the thread.
    void ThreadFunction() override
    {
        URHO3D_PROFILE_THREAD("AsyncSystemCommand Thread");

        exitCode_ = DoSystemCommand(commandLine_, false, nullptr);
        completed_ = true;
    }

private:
    /// Command line.
    ea::string commandLine_;
};

/// Async system run operation.
class AsyncSystemRun : public AsyncExecRequest
{
public:
    /// Construct and run.
    AsyncSystemRun(unsigned requestID, const ea::string& fileName, const ea::vector<ea::string>& arguments) :
        AsyncExecRequest(requestID),
        fileName_(fileName),
        arguments_(arguments)
    {
        Run();
    }

    /// The function to run in the thread.
    void ThreadFunction() override
    {
        URHO3D_PROFILE_THREAD("AsyncSystemRun Thread");
        ea::string output;
        exitCode_ = DoSystemRun(fileName_, arguments_, SR_WAIT_FOR_EXIT, output);
        completed_ = true;
    }

private:
    /// File to run.
    ea::string fileName_;
    /// Command line split in arguments.
    const ea::vector<ea::string>& arguments_;
};

FileSystem::FileSystem(Context* context) :
    Object(context)
{
    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(FileSystem, HandleBeginFrame));
}

FileSystem::~FileSystem()
{
    // If any async exec items pending, delete them
    if (asyncExecQueue_.size())
    {
        for (auto i = asyncExecQueue_.begin(); i != asyncExecQueue_.end(); ++i)
            delete(*i);

        asyncExecQueue_.clear();
    }
}

bool FileSystem::SetCurrentDir(const ea::string& pathName)
{
    if (!CheckAccess(pathName))
    {
        URHO3D_LOGERROR("Access denied to " + pathName);
        return false;
    }
#ifdef _WIN32
    if (SetCurrentDirectoryW(GetWideNativePath(pathName).c_str()) == FALSE)
    {
        URHO3D_LOGERROR("Failed to change directory to " + pathName);
        return false;
    }
#else
    if (chdir(GetNativePath(pathName).c_str()) != 0)
    {
        URHO3D_LOGERROR("Failed to change directory to " + pathName);
        return false;
    }
#endif

    return true;
}

bool FileSystem::CreateDir(const ea::string& pathName)
{
    if (!CheckAccess(pathName))
    {
        URHO3D_LOGERROR("Access denied to " + pathName);
        return false;
    }

    // Create each of the parents if necessary
    ea::string parentPath = GetParentPath(pathName);
    if (parentPath.length() > 1 && !DirExists(parentPath))
    {
        if (!CreateDir(parentPath))
            return false;
    }

#ifdef _WIN32
    bool success = (CreateDirectoryW(GetWideNativePath(RemoveTrailingSlash(pathName)).c_str(), nullptr) == TRUE) ||
        (GetLastError() == ERROR_ALREADY_EXISTS);
#else
    bool success = mkdir(GetNativePath(RemoveTrailingSlash(pathName)).c_str(), S_IRWXU) == 0 || errno == EEXIST;
#endif

    if (success)
        URHO3D_LOGDEBUG("Created directory " + pathName);
    else
        URHO3D_LOGERROR("Failed to create directory " + pathName);

    return success;
}

void FileSystem::SetExecuteConsoleCommands(bool enable)
{
#if URHO3D_SYSTEMUI
    if (enable == executeConsoleCommands_)
        return;

    executeConsoleCommands_ = enable;
    if (enable)
        SubscribeToEvent(E_CONSOLECOMMAND, URHO3D_HANDLER(FileSystem, HandleConsoleCommand));
    else
        UnsubscribeFromEvent(E_CONSOLECOMMAND);

    Console* console = GetSubsystem<Console>();
    console->RefreshInterpreters();
#else
    URHO3D_LOGWARNING("Engine was built without console support.");
#endif
}

int FileSystem::SystemCommand(const ea::string& commandLine, bool redirectStdOutToLog)
{
    if (allowedPaths_.empty())
        return DoSystemCommand(commandLine, redirectStdOutToLog, context_);
    else
    {
        URHO3D_LOGERROR("Executing an external command is not allowed");
        return -1;
    }
}

int FileSystem::SystemRun(const ea::string& fileName, const ea::vector<ea::string>& arguments, ea::string& output)
{
    if (allowedPaths_.empty())
        return DoSystemRun(fileName, arguments, SR_READ_OUTPUT, output);
    else
    {
        URHO3D_LOGERROR("Executing an external command is not allowed");
        return -1;
    }
}

int FileSystem::SystemRun(const ea::string& fileName, const ea::vector<ea::string>& arguments)
{
    ea::string output;
    if (allowedPaths_.empty())
        return DoSystemRun(fileName, arguments, SR_WAIT_FOR_EXIT, output);
    else
    {
        URHO3D_LOGERROR("Executing an external command is not allowed");
        return -1;
    }
}

int FileSystem::SystemSpawn(const ea::string& fileName, const ea::vector<ea::string>& arguments)
{
    ea::string output;
    if (allowedPaths_.empty())
        return DoSystemRun(fileName, arguments, SR_DEFAULT, output);
    else
    {
        URHO3D_LOGERROR("Executing an external command is not allowed");
        return -1;
    }
}

unsigned FileSystem::SystemCommandAsync(const ea::string& commandLine)
{
#ifdef URHO3D_THREADING
    if (allowedPaths_.empty())
    {
        unsigned requestID = nextAsyncExecID_;
        auto* cmd = new AsyncSystemCommand(nextAsyncExecID_, commandLine);
        asyncExecQueue_.push_back(cmd);
        return requestID;
    }
    else
    {
        URHO3D_LOGERROR("Executing an external command is not allowed");
        return M_MAX_UNSIGNED;
    }
#else
    URHO3D_LOGERROR("Can not execute an asynchronous command as threading is disabled");
    return M_MAX_UNSIGNED;
#endif
}

unsigned FileSystem::SystemRunAsync(const ea::string& fileName, const ea::vector<ea::string>& arguments)
{
#ifdef URHO3D_THREADING
    if (allowedPaths_.empty())
    {
        unsigned requestID = nextAsyncExecID_;
        auto* cmd = new AsyncSystemRun(nextAsyncExecID_, fileName, arguments);
        asyncExecQueue_.push_back(cmd);
        return requestID;
    }
    else
    {
        URHO3D_LOGERROR("Executing an external command is not allowed");
        return M_MAX_UNSIGNED;
    }
#else
    URHO3D_LOGERROR("Can not run asynchronously as threading is disabled");
    return M_MAX_UNSIGNED;
#endif
}

bool FileSystem::SystemOpen(const ea::string& fileName, const ea::string& mode)
{
    if (allowedPaths_.empty())
    {
        // allow opening of http and file urls
        if (!fileName.starts_with("http://") && !fileName.starts_with("https://") && !fileName.starts_with("file://"))
        {
            if (!FileExists(fileName) && !DirExists(fileName))
            {
                URHO3D_LOGERROR("File or directory " + fileName + " not found");
                return false;
            }
        }
#ifdef UWP
        bool success = false;
#elif defined(_WIN32)
        bool success = (size_t)ShellExecuteW(nullptr, !mode.empty() ? MultiByteToWide(mode).c_str() : nullptr,
            GetWideNativePath(fileName).c_str(), nullptr, nullptr, SW_SHOW) > 32;
#else
        ea::vector<ea::string> arguments;
        arguments.push_back(fileName);
        bool success = SystemRun(
#if defined(__APPLE__)
            "/usr/bin/open",
#else
            "/usr/bin/xdg-open",
#endif
            arguments) == 0;
#endif
        if (!success)
            URHO3D_LOGERROR("Failed to open " + fileName + " externally");
        return success;
    }
    else
    {
        URHO3D_LOGERROR("Opening a file externally is not allowed");
        return false;
    }
}

bool FileSystem::Copy(const ea::string& srcFileName, const ea::string& destFileName)
{
    if (!CheckAccess(GetPath(srcFileName)))
    {
        URHO3D_LOGERROR("Access denied to " + srcFileName);
        return false;
    }
    if (!CheckAccess(GetPath(destFileName)))
    {
        URHO3D_LOGERROR("Access denied to " + destFileName);
        return false;
    }

    SharedPtr<File> srcFile(new File(context_, srcFileName, FILE_READ));
    if (!srcFile->IsOpen())
        return false;
    SharedPtr<File> destFile(new File(context_, destFileName, FILE_WRITE));
    if (!destFile->IsOpen())
        return false;

    unsigned fileSize = srcFile->GetSize();
    ea::shared_array<unsigned char> buffer(new unsigned char[fileSize]);

    unsigned bytesRead = srcFile->Read(buffer.get(), fileSize);
    unsigned bytesWritten = destFile->Write(buffer.get(), fileSize);
    return bytesRead == fileSize && bytesWritten == fileSize;
}

bool FileSystem::Rename(const ea::string& srcFileName, const ea::string& destFileName)
{
    if (!CheckAccess(GetPath(srcFileName)))
    {
        URHO3D_LOGERROR("Access denied to " + srcFileName);
        return false;
    }
    if (!CheckAccess(GetPath(destFileName)))
    {
        URHO3D_LOGERROR("Access denied to " + destFileName);
        return false;
    }

#ifdef UWP
    return false;
#elif defined(_WIN32)
    return MoveFileW(GetWideNativePath(srcFileName).c_str(), GetWideNativePath(destFileName).c_str()) != 0;
#else
    return rename(GetNativePath(srcFileName).c_str(), GetNativePath(destFileName).c_str()) == 0;
#endif
}

bool FileSystem::Delete(const ea::string& fileName)
{
    if (!CheckAccess(GetPath(fileName)))
    {
        URHO3D_LOGERROR("Access denied to " + fileName);
        return false;
    }

#ifdef _WIN32
    return DeleteFileW(GetWideNativePath(fileName).c_str()) != 0;
#else
    return remove(GetNativePath(fileName).c_str()) == 0;
#endif
}

ea::string FileSystem::GetCurrentDir() const
{
#ifdef _WIN32
    wchar_t path[MAX_PATH];
    path[0] = 0;
    GetCurrentDirectoryW(MAX_PATH, path);
    return AddTrailingSlash(WideToMultiByte(path));
#else
    char path[MAX_PATH];
    path[0] = 0;
    getcwd(path, MAX_PATH);
    return AddTrailingSlash(ea::string(path));
#endif
}

bool FileSystem::CheckAccess(const ea::string& pathName) const
{
    ea::string fixedPath = AddTrailingSlash(pathName);

    // If no allowed directories defined, succeed always
    if (allowedPaths_.empty())
        return true;

    // If there is any attempt to go to a parent directory, disallow
    if (fixedPath.contains(".."))
        return false;

    // Check if the path is a partial match of any of the allowed directories
    for (auto i = allowedPaths_.begin(); i != allowedPaths_.end(); ++i)
    {
        if (fixedPath.find(*i) == 0)
            return true;
    }

    // Not found, so disallow
    return false;
}

unsigned FileSystem::GetLastModifiedTime(const ea::string& fileName) const
{
    if (fileName.empty() || !CheckAccess(fileName))
        return 0;

#ifdef _WIN32
    struct _stat st;
    if (!_stat(fileName.c_str(), &st))
        return (unsigned)st.st_mtime;
    else
        return 0;
#else
    struct stat st{};
    if (!stat(fileName.c_str(), &st))
        return (unsigned)st.st_mtime;
    else
        return 0;
#endif
}

bool FileSystem::FileExists(const ea::string& fileName) const
{
    if (!CheckAccess(GetPath(fileName)))
        return false;

#ifdef __ANDROID__
    if (URHO3D_IS_ASSET(fileName))
    {
        SDL_RWops* rwOps = SDL_RWFromFile(URHO3D_ASSET(fileName), "rb");
        if (rwOps)
        {
            SDL_RWclose(rwOps);
            return true;
        }
        else
            return false;
    }
#endif

    ea::string fixedName = GetNativePath(RemoveTrailingSlash(fileName));

#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(MultiByteToWide(fixedName).c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || attributes & FILE_ATTRIBUTE_DIRECTORY)
        return false;
#else
    struct stat st{};
    if (stat(fixedName.c_str(), &st) || st.st_mode & S_IFDIR)
        return false;
#endif

    return true;
}

bool FileSystem::DirExists(const ea::string& pathName) const
{
    if (!CheckAccess(pathName))
        return false;

#ifndef _WIN32
    // Always return true for the root directory
    if (pathName == "/")
        return true;
#endif

    ea::string fixedName = GetNativePath(RemoveTrailingSlash(pathName));

#ifdef __ANDROID__
    if (URHO3D_IS_ASSET(fixedName))
    {
        // Split the pathname into two components: the longest parent directory path and the last name component
        ea::string assetPath(URHO3D_ASSET((fixedName + "/")));
        ea::string parentPath;
        unsigned pos = assetPath.find_last_of('/', assetPath.length() - 2);
        if (pos != ea::string::npos)
        {
            parentPath = assetPath.substr(0, pos);
            assetPath = assetPath.substr(pos + 1);
        }
        assetPath.resize(assetPath.length() - 1);

        bool exist = false;
        int count;
        char** list = SDL_Android_GetFileList(parentPath.c_str(), &count);
        for (int i = 0; i < count; ++i)
        {
            exist = assetPath == list[i];
            if (exist)
                break;
        }
        SDL_Android_FreeFileList(&list, &count);
        return exist;
    }
#endif

#if defined(_WIN32) && !defined(UWP)
    DWORD attributes = GetFileAttributesW(MultiByteToWide(fixedName).c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
        return false;
#else
    struct stat st{};
    if (stat(fixedName.c_str(), &st) || !(st.st_mode & S_IFDIR))
        return false;
#endif

    return true;
}

void FileSystem::ScanDir(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, unsigned flags, bool recursive) const
{
    result.clear();

    if (CheckAccess(pathName))
    {
        ea::string initialPath = AddTrailingSlash(pathName);
        ScanDirInternal(result, initialPath, initialPath, filter, flags, recursive);
    }
}

void FileSystem::ScanDirAdd(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, unsigned flags, bool recursive) const
{
    if (CheckAccess(pathName))
    {
        ea::string initialPath = AddTrailingSlash(pathName);
        ScanDirInternal(result, initialPath, initialPath, filter, flags, recursive);
    }
}

ea::string FileSystem::GetProgramDir() const
{
#ifdef UWP
    return AddTrailingSlash(WideToMultiByte(Windows::ApplicationModel::Package::Current->InstalledLocation->Path->Data()));
#elif defined(__ANDROID__)
    // This is an internal directory specifier pointing to the assets in the .apk
    // Files from this directory will be opened using special handling
    return APK;
#elif defined(IOS) || defined(TVOS)
    return AddTrailingSlash(SDL_IOS_GetResourceDir());
#elif DESKTOP
    return GetPath(GetProgramFileName());
#else
    return GetCurrentDir();
#endif
}

ea::string FileSystem::GetProgramFileName() const
{
#if DESKTOP
    if (!specifiedExecutableFile.empty())
        return specifiedExecutableFile;

    return GetInterpreterFileName();
#else
    return "";
#endif
}

ea::string FileSystem::GetInterpreterFileName() const
{
#ifndef DESKTOP
#elif defined(_WIN32)
    wchar_t exeName[MAX_PATH];
    exeName[0] = 0;
    GetModuleFileNameW(nullptr, exeName, MAX_PATH);
    return WideToMultiByte(exeName);
#elif defined(__APPLE__)
    char exeName[MAX_PATH];
    memset(exeName, 0, MAX_PATH);
    unsigned size = MAX_PATH;
    _NSGetExecutablePath(exeName, &size);
    return ea::string(exeName);
#elif defined(__linux__)
    char exeName[MAX_PATH];
    memset(exeName, 0, MAX_PATH);
    pid_t pid = getpid();
    ea::string link(ea::string::CtorSprintf{}, "/proc/%d/exe", pid);
    readlink(link.c_str(), exeName, MAX_PATH);
    return ea::string(exeName);
#endif
    return "";
}

ea::string FileSystem::GetUserDocumentsDir() const
{
#if defined(UWP)
    return AddTrailingSlash(WideToMultiByte(Windows::Storage::ApplicationData::Current->LocalFolder->Path->Data()));
#elif defined(__ANDROID__)
    return AddTrailingSlash(SDL_Android_GetFilesDir());
#elif defined(IOS) || defined(TVOS)
    return AddTrailingSlash(SDL_IOS_GetDocumentsDir());
#elif defined(_WIN32)
    wchar_t pathName[MAX_PATH];
    pathName[0] = 0;
    SHGetSpecialFolderPathW(nullptr, pathName, CSIDL_PERSONAL, 0);
    return AddTrailingSlash(WideToMultiByte(pathName));
#else
    char pathName[MAX_PATH];
    pathName[0] = 0;
    strcpy(pathName, getenv("HOME"));
    return AddTrailingSlash(ea::string(pathName));
#endif
}

ea::string FileSystem::GetAppPreferencesDir(const ea::string& org, const ea::string& app) const
{
    ea::string dir;
#ifndef MINI_URHO
    char* prefPath = SDL_GetPrefPath(org.c_str(), app.c_str());
    if (prefPath)
    {
        dir = GetInternalPath(ea::string(prefPath));
        SDL_free(prefPath);
    }
    else
#endif
        URHO3D_LOGWARNING("Could not get application preferences directory");

    return dir;
}

void FileSystem::RegisterPath(const ea::string& pathName)
{
    if (pathName.empty())
        return;

    allowedPaths_.insert(AddTrailingSlash(pathName));
}

bool FileSystem::SetLastModifiedTime(const ea::string& fileName, unsigned newTime)
{
    if (fileName.empty() || !CheckAccess(fileName))
        return false;

#ifdef _WIN32
    struct _stat oldTime;
    struct _utimbuf newTimes;
    if (_stat(fileName.c_str(), &oldTime) != 0)
        return false;
    newTimes.actime = oldTime.st_atime;
    newTimes.modtime = newTime;
    return _utime(fileName.c_str(), &newTimes) == 0;
#else
    struct stat oldTime{};
    struct utimbuf newTimes{};
    if (stat(fileName.c_str(), &oldTime) != 0)
        return false;
    newTimes.actime = oldTime.st_atime;
    newTimes.modtime = newTime;
    return utime(fileName.c_str(), &newTimes) == 0;
#endif
}

void FileSystem::ScanDirInternal(ea::vector<ea::string>& result, ea::string path, const ea::string& startPath,
    const ea::string& filter, unsigned flags, bool recursive) const
{
    path = AddTrailingSlash(path);
    ea::string deltaPath;
    if (path.length() > startPath.length())
        deltaPath = path.substr(startPath.length());

    ea::string filterExtension;
    unsigned dotPos = filter.find_last_of('.');
    if (dotPos != ea::string::npos)
        filterExtension = filter.substr(dotPos);
    if (filterExtension.contains('*'))
        filterExtension.clear();

#ifdef __ANDROID__
    if (URHO3D_IS_ASSET(path))
    {
        ea::string assetPath(URHO3D_ASSET(path));
        assetPath = RemoveTrailingSlash(assetPath);       // AssetManager.list() does not like trailing slash
        int count;
        char** list = SDL_Android_GetFileList(assetPath.c_str(), &count);
        for (int i = 0; i < count; ++i)
        {
            ea::string fileName(list[i]);
            if (!(flags & SCAN_HIDDEN) && fileName.starts_with("."))
                continue;

#ifdef ASSET_DIR_INDICATOR
            // Patch the directory name back after retrieving the directory flag
            bool isDirectory = fileName.ends_with(ASSET_DIR_INDICATOR);
            if (isDirectory)
            {
                fileName.resize(fileName.length() - sizeof(ASSET_DIR_INDICATOR) / sizeof(char) + 1);
                if (flags & SCAN_DIRS)
                    result.push_back(deltaPath + fileName);
                if (recursive)
                    ScanDirInternal(result, path + fileName, startPath, filter, flags, recursive);
            }
            else if (flags & SCAN_FILES)
#endif
            {
                if (filterExtension.empty() || fileName.ends_with(filterExtension))
                    result.push_back(deltaPath + fileName);
            }
        }
        SDL_Android_FreeFileList(&list, &count);
        return;
    }
#endif
#ifdef _WIN32
    WIN32_FIND_DATAW info;
    HANDLE handle = FindFirstFileW(MultiByteToWide((path + "*")).c_str(), &info);
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            ea::string fileName = WideToMultiByte(info.cFileName);
            if (!fileName.empty())
            {
                if (info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN && !(flags & SCAN_HIDDEN))
                    continue;
                if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (flags & SCAN_DIRS)
                        result.emplace_back(deltaPath + fileName);
                    if (recursive && fileName != "." && fileName != "..")
                        ScanDirInternal(result, path + fileName, startPath, filter, flags, recursive);
                }
                else if (flags & SCAN_FILES)
                {
                    if (filterExtension.empty() || fileName.ends_with(filterExtension))
                        result.emplace_back(deltaPath + fileName);
                }
            }
        }
        while (FindNextFileW(handle, &info));

        FindClose(handle);
    }
#else
    DIR* dir;
    struct dirent* de;
    struct stat st{};
    dir = opendir(GetNativePath(path).c_str());
    if (dir)
    {
        while ((de = readdir(dir)))
        {
            /// \todo Filename may be unnormalized Unicode on Mac OS X. Re-normalize as necessary
            ea::string fileName(de->d_name);
            bool normalEntry = fileName != "." && fileName != "..";
            if (normalEntry && !(flags & SCAN_HIDDEN) && fileName.starts_with("."))
                continue;
            ea::string pathAndName = path + fileName;
            if (!stat(pathAndName.c_str(), &st))
            {
                if (st.st_mode & S_IFDIR)
                {
                    if (flags & SCAN_DIRS)
                        result.push_back(deltaPath + fileName);
                    if (recursive && normalEntry)
                        ScanDirInternal(result, path + fileName, startPath, filter, flags, recursive);
                }
                else if (flags & SCAN_FILES)
                {
                    if (filterExtension.empty() || fileName.ends_with(filterExtension))
                        result.push_back(deltaPath + fileName);
                }
            }
        }
        closedir(dir);
    }
#endif
}

void FileSystem::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    // Go through the execution queue and post + remove completed requests
    for (auto i = asyncExecQueue_.begin(); i != asyncExecQueue_.end();)
    {
        AsyncExecRequest* request = *i;
        if (request->IsCompleted())
        {
            using namespace AsyncExecFinished;

            VariantMap& newEventData = GetEventDataMap();
            newEventData[P_REQUESTID] = request->GetRequestID();
            newEventData[P_EXITCODE] = request->GetExitCode();
            SendEvent(E_ASYNCEXECFINISHED, newEventData);

            delete request;
            i = asyncExecQueue_.erase(i);
        }
        else
            ++i;
    }
}

void FileSystem::HandleConsoleCommand(StringHash eventType, VariantMap& eventData)
{
    using namespace ConsoleCommand;
    if (eventData[P_ID].GetString() == GetTypeName())
        SystemCommand(eventData[P_COMMAND].GetString(), true);
}

void SplitPath(const ea::string& fullPath, ea::string& pathName, ea::string& fileName, ea::string& extension, bool lowercaseExtension)
{
    ea::string fullPathCopy = GetInternalPath(fullPath);

    unsigned extPos = fullPathCopy.find_last_of('.');
    unsigned pathPos = fullPathCopy.find_last_of('/');

    if (extPos != ea::string::npos && (pathPos == ea::string::npos || extPos > pathPos))
    {
        extension = fullPathCopy.substr(extPos);
        if (lowercaseExtension)
            extension = extension.to_lower();
        fullPathCopy = fullPathCopy.substr(0, extPos);
    }
    else
        extension.clear();

    pathPos = fullPathCopy.find_last_of('/');
    if (pathPos != ea::string::npos)
    {
        fileName = fullPathCopy.substr(pathPos + 1);
        pathName = fullPathCopy.substr(0, pathPos + 1);
    }
    else
    {
        fileName = fullPathCopy;
        pathName.clear();
    }
}

ea::string GetPath(const ea::string& fullPath)
{
    ea::string path, file, extension;
    SplitPath(fullPath, path, file, extension);
    return path;
}

ea::string GetFileName(const ea::string& fullPath)
{
    ea::string path, file, extension;
    SplitPath(fullPath, path, file, extension);
    return file;
}

ea::string GetExtension(const ea::string& fullPath, bool lowercaseExtension)
{
    ea::string path, file, extension;
    SplitPath(fullPath, path, file, extension, lowercaseExtension);
    return extension;
}

ea::string GetFileNameAndExtension(const ea::string& fileName, bool lowercaseExtension)
{
    ea::string path, file, extension;
    SplitPath(fileName, path, file, extension, lowercaseExtension);
    return file + extension;
}

ea::string ReplaceExtension(const ea::string& fullPath, const ea::string& newExtension)
{
    ea::string path, file, extension;
    SplitPath(fullPath, path, file, extension);
    return path + file + newExtension;
}

ea::string AddTrailingSlash(const ea::string& pathName)
{
    ea::string ret = pathName.trimmed();
    ret.replace('\\', '/');
    if (!ret.empty() && ret.back() != '/')
        ret += '/';
    return ret;
}

ea::string RemoveTrailingSlash(const ea::string& pathName)
{
    ea::string ret = pathName.trimmed();
    ret.replace('\\', '/');
    if (!ret.empty() && ret.back() == '/')
        ret.resize(ret.length() - 1);
    return ret;
}

ea::string GetParentPath(const ea::string& path)
{
    unsigned pos = RemoveTrailingSlash(path).find_last_of('/');
    if (pos != ea::string::npos)
        return path.substr(0, pos + 1);
    else
        return ea::string();
}

ea::string GetInternalPath(const ea::string& pathName)
{
    return pathName.replaced('\\', '/');
}

ea::string GetNativePath(const ea::string& pathName)
{
#ifdef _WIN32
    return pathName.replaced('/', '\\');
#else
    return pathName;
#endif
}

ea::wstring GetWideNativePath(const ea::string& pathName)
{
    ea::wstring result = MultiByteToWide(pathName);
#ifdef _WIN32
    result.replace(L'/', L'\\');
#endif
    return result;
}

bool IsAbsolutePath(const ea::string& pathName)
{
    if (pathName.empty())
        return false;

    ea::string path = GetInternalPath(pathName);

    if (path[0] == '/')
        return true;

#ifdef _WIN32
    if (path.length() > 1 && IsAlpha(path[0]) && path[1] == ':')
        return true;
#endif

    return false;
}

bool FileSystem::CreateDirs(const ea::string& root, const ea::string& subdirectory)
{
    ea::string folder = AddTrailingSlash(GetInternalPath(root));
    ea::string sub = GetInternalPath(subdirectory);
    ea::vector<ea::string> subs = sub.split('/');

    for (unsigned i = 0; i < subs.size(); i++)
    {
        folder += subs[i];
        folder += "/";

        if (DirExists(folder))
            continue;

        CreateDir(folder);

        if (!DirExists(folder))
            return false;
    }

    return true;

}

bool FileSystem::CreateDirsRecursive(const ea::string& directoryIn)
{
    ea::string directory = AddTrailingSlash(GetInternalPath(directoryIn));

    if (DirExists(directory))
        return true;

    if (FileExists(directory))
        return false;

    ea::string parentPath = directory;

    ea::vector<ea::string> paths;

    paths.push_back(directory);

    while (true)
    {
        parentPath = GetParentPath(parentPath);

        if (!parentPath.length())
            break;

        paths.push_back(parentPath);
    }

    if (!paths.size())
        return false;

    for (auto i = (int) (paths.size() - 1); i >= 0; i--)
    {
        const ea::string& pathName = paths[i];

        if (FileExists(pathName))
            return false;

        if (DirExists(pathName))
            continue;

        if (!CreateDir(pathName))
            return false;

        // double check
        if (!DirExists(pathName))
            return false;

    }

    return true;

}

bool FileSystem::RemoveDir(const ea::string& directoryIn, bool recursive)
{
    ea::string directory = AddTrailingSlash(directoryIn);

    if (!DirExists(directory))
        return false;

    ea::vector<ea::string> results;

    // ensure empty if not recursive
    if (!recursive)
    {
        ScanDir(results, directory, "*", SCAN_DIRS | SCAN_FILES | SCAN_HIDDEN, true );
        while (results.erase_first(".") != results.end()) {}
        while (results.erase_first("..") != results.end()) {}

        if (results.size())
            return false;

#ifdef WIN32
        return RemoveDirectoryW(GetWideNativePath(directory).c_str()) != 0;
#else
        return remove(GetNativePath(directory).c_str()) == 0;
#endif
    }

    // delete all files at this level
    ScanDir(results, directory, "*", SCAN_FILES | SCAN_HIDDEN, false );
    for (unsigned i = 0; i < results.size(); i++)
    {
        if (!Delete(directory + results[i]))
            return false;
    }
    results.clear();

    // recurse into subfolders
    ScanDir(results, directory, "*", SCAN_DIRS, false );
    for (unsigned i = 0; i < results.size(); i++)
    {
        if (results[i] == "." || results[i] == "..")
            continue;

        if (!RemoveDir(directory + results[i], true))
            return false;
    }

    return RemoveDir(directory, false);

}

bool FileSystem::CopyDir(const ea::string& directoryIn, const ea::string& directoryOut)
{
    if (FileExists(directoryOut))
        return false;

    ea::vector<ea::string> results;
    ScanDir(results, directoryIn, "*", SCAN_FILES, true );

    for (unsigned i = 0; i < results.size(); i++)
    {
        ea::string srcFile = directoryIn + "/" + results[i];
        ea::string dstFile = directoryOut + "/" + results[i];

        ea::string dstPath = GetPath(dstFile);

        if (!CreateDirsRecursive(dstPath))
            return false;

        //LOGINFOF("SRC: %s DST: %s", srcFile.c_str(), dstFile.c_str());
        if (!Copy(srcFile, dstFile))
            return false;
    }

    return true;

}

bool IsAbsoluteParentPath(const ea::string& absParentPath, const ea::string& fullPath)
{
    if (!IsAbsolutePath(absParentPath) || !IsAbsolutePath(fullPath))
        return false;

    ea::string path1 = AddTrailingSlash(GetSanitizedPath(absParentPath));
    ea::string path2 = AddTrailingSlash(GetSanitizedPath(GetPath(fullPath)));

    if (path2.starts_with(path1))
        return true;

    return false;
}

ea::string GetSanitizedPath(const ea::string& path)
{
    ea::string sanitized = GetInternalPath(path);
    StringVector parts = sanitized.split('/');

    bool hasTrailingSlash = path.ends_with("/") || path.ends_with("\\");

#ifndef _WIN32

    bool absolute = IsAbsolutePath(path);
    sanitized = ea::string::joined(parts, "/");
    if (absolute)
        sanitized = "/" + sanitized;

#else

    sanitized = ea::string::joined(parts, "/");

#endif

    if (hasTrailingSlash)
        sanitized += "/";

    return sanitized;

}

bool GetRelativePath(const ea::string& fromPath, const ea::string& toPath, ea::string& output)
{
    output = EMPTY_STRING;

    ea::string from = GetSanitizedPath(fromPath);
    ea::string to = GetSanitizedPath(toPath);

    StringVector fromParts = from.split('/');
    StringVector toParts = to.split('/');

    if (!fromParts.size() || !toParts.size())
        return false;

    if (fromParts == toParts)
    {
        return true;
    }

    // no common base?
    if (fromParts[0] != toParts[0])
        return false;

    int startIdx;

    for (startIdx = 0; startIdx < toParts.size(); startIdx++)
    {
        if (startIdx >= fromParts.size() || fromParts[startIdx] != toParts[startIdx])
            break;
    }

    if (startIdx == toParts.size())
    {
        if (from.ends_with("/") && to.ends_with("/"))
        {
            for (unsigned i = 0; i < fromParts.size() - startIdx; i++)
            {
                output += "../";
            }

            return true;
        }
        return false;
    }

    for (int i = 0; i < (int) fromParts.size() - startIdx; i++)
    {
        output += "../";
    }

    for (int i = startIdx; i < (int) toParts.size(); i++)
    {
        output += toParts[i] + "/";
    }

    return true;

}

ea::string FileSystem::GetTemporaryDir() const
{
#if defined(_WIN32)
#if defined(MINI_URHO)
    return getenv("TMP");
#else
    wchar_t pathName[MAX_PATH];
    pathName[0] = 0;
    GetTempPathW(SDL_arraysize(pathName), pathName);
    return AddTrailingSlash(WideToMultiByte(pathName));
#endif
#else
    if (char* pathName = getenv("TMPDIR"))
        return AddTrailingSlash(pathName);
#ifdef P_tmpdir
    return AddTrailingSlash(P_tmpdir);
#else
    return "/tmp/";
#endif
#endif
}

ea::string GetAbsolutePath(const ea::string& path)
{
    ea::vector<ea::string> parts;
#if !_WIN32
    parts.push_back("");
#endif
    auto split = path.split('/');
    parts.insert(parts.end(), split.begin(), split.end());

    int index = 0;
    while (index < parts.size() - 1)
    {
        if (parts[index] != ".." && parts[index + 1] == "..")
        {
            parts.erase_at(index, 2);
            index = Max(0, --index);
        }
        else
            ++index;
    }

    return ea::string::joined(parts, "/");
}

}
