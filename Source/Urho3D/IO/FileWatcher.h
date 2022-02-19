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

#include "../Core/Mutex.h"
#include "../Core/Object.h"
#include "../Core/Thread.h"
#include "../Core/Timer.h"

namespace Urho3D
{

class FileSystem;

enum FileChangeKind
{
    /// New file was created.
    FILECHANGE_ADDED,
    /// File was deleted.
    FILECHANGE_REMOVED,
    /// File was renamed.
    FILECHANGE_RENAMED,
    /// File was modified.
    FILECHANGE_MODIFIED,
};

/// File change information.
struct FileChange
{
    /// File change kind.
    FileChangeKind kind_;
    /// Name of modified file name. Always set.
    ea::string fileName_;
    /// Previous file name in case of FILECHANGE_MODIFIED event. Empty otherwise.
    ea::string oldFileName_;
};


/// Watches a directory and its subdirectories for files being modified.
class URHO3D_API FileWatcher : public Object, public Thread
{
    URHO3D_OBJECT(FileWatcher, Object);

public:
    /// Construct.
    explicit FileWatcher(Context* context);
    /// Destruct.
    ~FileWatcher() override;

    /// Directory watching loop.
    void ThreadFunction() override;

    /// Start watching a directory. Return true if successful.
    bool StartWatching(const ea::string& pathName, bool watchSubDirs);
    /// Stop watching the directory.
    void StopWatching();
    /// Set the delay in seconds before file changes are notified. This (hopefully) avoids notifying when a file save is still in progress. Default 1 second.
    void SetDelay(float interval);
    /// Add a file change into the changes queue.
    void AddChange(const FileChange& change);
    /// Return a file change (true if was found, false if not).
    bool GetNextChange(FileChange& dest);

    /// Return the path being watched, or empty if not watching.
    const ea::string& GetPath() const { return path_; }

    /// Return the delay in seconds for notifying file changes.
    float GetDelay() const { return delay_; }

private:
    struct TimedFileChange
    {
        /// File change information.
        FileChange change_;
        /// Timer used to filter out repeated events when file is being written.
        Timer timer_;
    };

    /// Filesystem.
    SharedPtr<FileSystem> fileSystem_;
    /// The path being watched.
    ea::string path_;
    /// Pending changes. These will be returned and removed from the list when their timer has exceeded the delay.
    ea::unordered_map<ea::string, TimedFileChange> changes_;
    /// Mutex for the change buffer.
    Mutex changesMutex_;
    /// Delay in seconds for notifying changes.
    float delay_;
    /// Watch subdirectories flag.
    bool watchSubDirs_;

#ifdef _WIN32

    /// Directory handle for the path being watched.
    void* dirHandle_;

#elif __linux__

    /// HashMap for the directory and sub-directories (needed for inotify's int handles).
    ea::unordered_map<int, ea::string> dirHandle_;
    /// Linux inotify needs a handle.
    int watchHandle_;

#elif defined(__APPLE__) && !defined(IOS) && !defined(TVOS)

    /// Flag indicating whether the running OS supports individual file watching.
    bool supported_;
    /// Pointer to internal MacFileWatcher delegate.
    void* watcher_;

#endif
};

}
