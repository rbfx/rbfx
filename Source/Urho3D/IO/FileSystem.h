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

#pragma once

#include <EASTL/list.h>
#include <EASTL/hash_set.h>

#include "../Core/Object.h"

namespace Urho3D
{

class AsyncExecRequest;

/// Return files.
static const unsigned SCAN_FILES = 0x1;
/// Return directories.
static const unsigned SCAN_DIRS = 0x2;
/// Return also hidden files.
static const unsigned SCAN_HIDDEN = 0x4;

/// Subsystem for file and directory operations and access control.
class URHO3D_API FileSystem : public Object
{
    URHO3D_OBJECT(FileSystem, Object);

public:
    /// Construct.
    explicit FileSystem(Context* context);
    /// Destruct.
    ~FileSystem() override;

    /// Set the current working directory.
    /// @property
    bool SetCurrentDir(const ea::string& pathName);
    /// Create a directory.
    bool CreateDir(const ea::string& pathName);
    /// Set whether to execute engine console commands as OS-specific system command.
    /// @property
    void SetExecuteConsoleCommands(bool enable);
    /// Run a program using the command interpreter, block until it exits and return the exit code. Will fail if any allowed paths are defined.
    int SystemCommand(const ea::string& commandLine, bool redirectStdOutToLog = false);
    /// Run a specific program, block until it exits and return the exit code. Will fail if any allowed paths are defined. Returns STDOUT output of subprocess.
    int SystemRun(const ea::string& fileName, const ea::vector<ea::string>& arguments, ea::string& output);
    /// Run a specific program, block until it exits and return the exit code. Will fail if any allowed paths are defined.
    int SystemRun(const ea::string& fileName, const ea::vector<ea::string>& arguments);
    /// Run a specific program, do not block until it exits. Will fail if any allowed paths are defined.
    int SystemSpawn(const ea::string& fileName, const ea::vector<ea::string>& arguments);
    /// Run a program using the command interpreter asynchronously. Return a request ID or M_MAX_UNSIGNED if failed. The exit code will be posted together with the request ID in an AsyncExecFinished event. Will fail if any allowed paths are defined.
    unsigned SystemCommandAsync(const ea::string& commandLine);
    /// Run a specific program asynchronously. Return a request ID or M_MAX_UNSIGNED if failed. The exit code will be posted together with the request ID in an AsyncExecFinished event. Will fail if any allowed paths are defined.
    unsigned SystemRunAsync(const ea::string& fileName, const ea::vector<ea::string>& arguments);
    /// Open a file in an external program, with mode such as "edit" optionally specified. Will fail if any allowed paths are defined.
    bool SystemOpen(const ea::string& fileName, const ea::string& mode = EMPTY_STRING);
    /// Copy a file. Return true if successful.
    bool Copy(const ea::string& srcFileName, const ea::string& destFileName);
    /// Rename a file. Return true if successful.
    bool Rename(const ea::string& srcFileName, const ea::string& destFileName);
    /// Delete a file. Return true if successful.
    bool Delete(const ea::string& fileName);
    /// Register a path as allowed to access. If no paths are registered, all are allowed. Registering allowed paths is considered securing the Urho3D execution environment: running programs and opening files externally through the system will fail afterward.
    void RegisterPath(const ea::string& pathName);
    /// Set a file's last modified time as seconds since 1.1.1970. Return true on success.
    bool SetLastModifiedTime(const ea::string& fileName, unsigned newTime);

    /// Return the absolute current working directory.
    /// @property
    ea::string GetCurrentDir() const;

    /// Return whether is executing engine console commands as OS-specific system command.
    /// @property
    bool GetExecuteConsoleCommands() const { return executeConsoleCommands_; }

    /// Return whether paths have been registered.
    bool HasRegisteredPaths() const { return allowedPaths_.size() > 0; }

    /// Check if a path is allowed to be accessed. If no paths are registered, all are allowed.
    bool CheckAccess(const ea::string& pathName) const;
    /// Returns the file's last modified time as seconds since 1.1.1970, or 0 if can not be accessed.
    unsigned GetLastModifiedTime(const ea::string& fileName) const;
    /// Check if a file exists.
    bool FileExists(const ea::string& fileName) const;
    /// Check if a directory exists.
    bool DirExists(const ea::string& pathName) const;
    /// Scan a directory for specified files.
    void ScanDir(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, unsigned flags, bool recursive) const;
    /// Scan a directory for specified files. Appends to result container instead of clearing it.
    void ScanDirAdd(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, unsigned flags, bool recursive) const;
    /// Return the program's directory.
    /// @property
    ea::string GetProgramDir() const;
    /// Return the program's executable file path, or empty string if not applicable.
    ea::string GetProgramFileName() const;
    /// Return executable path of interpreter program (for example path to mono executable on unixes for C# application), or empty string if not applicable.
    /// If application is executed directly (no interpreter) this will return same result as GetProgramFileName().
    ea::string GetInterpreterFileName() const;
    /// Return the user documents directory.
    /// @property
    ea::string GetUserDocumentsDir() const;
    /// Return the application preferences directory.
    ea::string GetAppPreferencesDir(const ea::string& org, const ea::string& app) const;
    /// Check if a file or directory exists at the specified path
    bool Exists(const ea::string& pathName) const { return FileExists(pathName) || DirExists(pathName); }
    /// Copy files from one directory to another.
    bool CopyDir(const ea::string& directoryIn, const ea::string& directoryOut);
    /// Create subdirectories. New subdirectories will be made only in a subpath specified by `subdirectory`.
    bool CreateDirs(const ea::string& root, const ea::string& subdirectory);
    /// Create specified subdirectory and any parent directory if it does not exist.
    bool CreateDirsRecursive(const ea::string& directoryIn);
    /// Remove files in a directory, or remove entire directory recursively.
    bool RemoveDir(const ea::string& directoryIn, bool recursive);
    /// Return path of temporary directory. Path always ends with a forward slash.
    /// @property
    ea::string GetTemporaryDir() const;

private:
    /// Scan directory, called internally.
    void ScanDirInternal
        (ea::vector<ea::string>& result, ea::string path, const ea::string& startPath, const ea::string& filter, unsigned flags, bool recursive) const;
    /// Handle begin frame event to check for completed async executions.
    void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
    /// Handle a console command event.
    void HandleConsoleCommand(StringHash eventType, VariantMap& eventData);

    /// Allowed directories.
    ea::hash_set<ea::string> allowedPaths_;
    /// Async execution queue.
    ea::list<AsyncExecRequest*> asyncExecQueue_;
    /// Next async execution ID.
    unsigned nextAsyncExecID_{1};
    /// Flag for executing engine console commands as OS-specific system command. Default to true.
    bool executeConsoleCommands_{};
};

/// Split a full path to path, filename and extension. The extension will be converted to lowercase by default.
URHO3D_API void
    SplitPath(const ea::string& fullPath, ea::string& pathName, ea::string& fileName, ea::string& extension, bool lowercaseExtension = true);
/// Return the path from a full path.
URHO3D_API ea::string GetPath(const ea::string& fullPath);
/// Return the filename from a full path.
URHO3D_API ea::string GetFileName(const ea::string& fullPath);
/// Return the extension from a full path, converted to lowercase by default.
URHO3D_API ea::string GetExtension(const ea::string& fullPath, bool lowercaseExtension = true);
/// Return the filename and extension from a full path. The case of the extension is preserved by default, so that the file can be opened in case-sensitive operating systems.
URHO3D_API ea::string GetFileNameAndExtension(const ea::string& fileName, bool lowercaseExtension = false);
/// Replace the extension of a file name with another.
URHO3D_API ea::string ReplaceExtension(const ea::string& fullPath, const ea::string& newExtension);
/// Add a slash at the end of the path if missing and convert to internal format (use slashes).
URHO3D_API ea::string AddTrailingSlash(const ea::string& pathName);
/// Remove the slash from the end of a path if exists and convert to internal format (use slashes).
URHO3D_API ea::string RemoveTrailingSlash(const ea::string& pathName);
/// Return the parent path, or the path itself if not available.
URHO3D_API ea::string GetParentPath(const ea::string& path);
/// Convert a path to internal format (use slashes).
URHO3D_API ea::string GetInternalPath(const ea::string& pathName);
/// Convert a path to the format required by the operating system.
URHO3D_API ea::string GetNativePath(const ea::string& pathName);
/// Convert a path to the format required by the operating system in wide characters.
URHO3D_API ea::wstring GetWideNativePath(const ea::string& pathName);
/// Return whether a path is absolute.
URHO3D_API bool IsAbsolutePath(const ea::string& pathName);
///
URHO3D_API bool IsAbsoluteParentPath(const ea::string& absParentPath, const ea::string& fullPath);
///
URHO3D_API ea::string GetSanitizedPath(const ea::string& path);
/// Given two absolute directory paths, get the relative path from one to the other
/// Returns false if either path isn't absolute, or if they are unrelated
URHO3D_API bool GetRelativePath(const ea::string& fromPath, const ea::string& toPath, ea::string& output);
/// Convert relative path to full path.
URHO3D_API ea::string GetAbsolutePath(const ea::string& path);

}
