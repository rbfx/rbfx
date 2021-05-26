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

#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/FileWatcher.h"
#include "../IO/Log.h"
#include "../Core/Profiler.h"

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <sys/inotify.h>
#include <sys/ioctl.h>
extern "C"
{
// Need read/close for inotify
#include "unistd.h"
}
#elif defined(__APPLE__) && !defined(IOS) && !defined(TVOS)
extern "C"
{
#include "../IO/MacFileWatcher.h"
}
#endif

namespace Urho3D
{
#ifndef __APPLE__
static const unsigned BUFFERSIZE = 4096;
#endif

FileWatcher::FileWatcher(Context* context) :
    Object(context),
    fileSystem_(GetSubsystem<FileSystem>()),
    delay_(1.0f),
    watchSubDirs_(false)
{
#ifdef URHO3D_FILEWATCHER
#ifdef __linux__
    watchHandle_ = inotify_init();
#elif defined(__APPLE__) && !defined(IOS) && !defined(TVOS)
    supported_ = IsFileWatcherSupported();
#endif
#endif
}

FileWatcher::~FileWatcher()
{
    StopWatching();
#ifdef URHO3D_FILEWATCHER
#ifdef __linux__
    close(watchHandle_);
#endif
#endif
}

bool FileWatcher::StartWatching(const ea::string& pathName, bool watchSubDirs)
{
    if (!fileSystem_)
    {
        URHO3D_LOGERROR("No FileSystem, can not start watching");
        return false;
    }

    // Stop any previous watching
    StopWatching();

    SetName("Watcher for " + pathName);

#if defined(URHO3D_FILEWATCHER) && defined(URHO3D_THREADING)
#ifdef _WIN32
    ea::string nativePath = GetNativePath(RemoveTrailingSlash(pathName));

    dirHandle_ = (void*)CreateFileW(
        MultiByteToWide(nativePath).c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    if (dirHandle_ != INVALID_HANDLE_VALUE)
    {
        path_ = AddTrailingSlash(pathName);
        watchSubDirs_ = watchSubDirs;
        Run();

        URHO3D_LOGDEBUG("Started watching path " + pathName);
        return true;
    }
    else
    {
        URHO3D_LOGERROR("Failed to start watching path " + pathName);
        return false;
    }
#elif defined(__linux__)
    int flags = IN_CREATE | IN_DELETE | IN_MODIFY | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO;
    int handle = inotify_add_watch(watchHandle_, pathName.c_str(), (unsigned)flags);

    if (handle < 0)
    {
        URHO3D_LOGERROR("Failed to start watching path " + pathName);
        return false;
    }
    else
    {
        // Store the root path here when reconstructed with inotify later
        dirHandle_[handle] = "";
        path_ = AddTrailingSlash(pathName);
        watchSubDirs_ = watchSubDirs;

        if (watchSubDirs_)
        {
            ea::vector<ea::string> subDirs;
            fileSystem_->ScanDir(subDirs, pathName, "*", SCAN_DIRS, true);

            for (unsigned i = 0; i < subDirs.size(); ++i)
            {
                ea::string subDirFullPath = AddTrailingSlash(path_ + subDirs[i]);

                // Don't watch ./ or ../ sub-directories
                if (!subDirFullPath.ends_with("./"))
                {
                    handle = inotify_add_watch(watchHandle_, subDirFullPath.c_str(), (unsigned)flags);
                    if (handle < 0)
                        URHO3D_LOGERROR("Failed to start watching subdirectory path " + subDirFullPath);
                    else
                    {
                        // Store sub-directory to reconstruct later from inotify
                        dirHandle_[handle] = AddTrailingSlash(subDirs[i]);
                    }
                }
            }
        }
        Run();

        URHO3D_LOGDEBUG("Started watching path " + pathName);
        return true;
    }
#elif defined(__APPLE__) && !defined(IOS) && !defined(TVOS)
    if (!supported_)
    {
        URHO3D_LOGERROR("Individual file watching not supported by this OS version, can not start watching path " + pathName);
        return false;
    }

    watcher_ = CreateFileWatcher(pathName.c_str(), watchSubDirs);
    if (watcher_)
    {
        path_ = AddTrailingSlash(pathName);
        watchSubDirs_ = watchSubDirs;
        Run();

        URHO3D_LOGDEBUG("Started watching path " + pathName);
        return true;
    }
    else
    {
        URHO3D_LOGERROR("Failed to start watching path " + pathName);
        return false;
    }
#else
    URHO3D_LOGERROR("FileWatcher not implemented, can not start watching path " + pathName);
    return false;
#endif
#else
    URHO3D_LOGDEBUG("FileWatcher feature not enabled");
    return false;
#endif
}

void FileWatcher::StopWatching()
{
    if (handle_)
    {
        shouldRun_ = false;

        // Create and delete a dummy file to make sure the watcher loop terminates
        // This is only required on Windows platform
        // TODO: Remove this temp write approach as it depends on user write privilege
#ifdef _WIN32
        ea::string dummyFileName = path_ + "dummy.tmp";
        File file(context_, dummyFileName, FILE_WRITE);
        file.Close();
        if (fileSystem_)
            fileSystem_->Delete(dummyFileName);
#endif

#if defined(__APPLE__) && !defined(IOS) && !defined(TVOS)
        // Our implementation of file watcher requires the thread to be stopped first before closing the watcher
        Stop();
#endif

#ifdef _WIN32
        CloseHandle((HANDLE)dirHandle_);
#elif defined(__linux__)
        for (auto i = dirHandle_.begin(); i != dirHandle_.end(); ++i)
            inotify_rm_watch(watchHandle_, i->first);
        dirHandle_.clear();
#elif defined(__APPLE__) && !defined(IOS) && !defined(TVOS)
        CloseFileWatcher(watcher_);
#endif

#ifndef __APPLE__
        Stop();
#endif

        URHO3D_LOGDEBUG("Stopped watching path " + path_);
        path_.clear();
    }
}

void FileWatcher::SetDelay(float interval)
{
    delay_ = Max(interval, 0.0f);
}

void FileWatcher::ThreadFunction()
{
#ifdef URHO3D_FILEWATCHER
    URHO3D_PROFILE_THREAD("FileWatcher Thread");

#ifdef _WIN32
    unsigned char buffer[BUFFERSIZE];
    DWORD bytesFilled = 0;

    while (shouldRun_)
    {
        if (ReadDirectoryChangesW((HANDLE)dirHandle_,
            buffer,
            BUFFERSIZE,
            watchSubDirs_,
            FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesFilled,
            nullptr,
            nullptr))
        {
            unsigned offset = 0;
            FileChange rename{FILECHANGE_RENAMED, EMPTY_STRING, EMPTY_STRING};

            while (offset < bytesFilled)
            {
                FILE_NOTIFY_INFORMATION* record = (FILE_NOTIFY_INFORMATION*)&buffer[offset];

                ea::string fileName;
                const wchar_t* src = record->FileName;
                const wchar_t* end = src + record->FileNameLength / 2;
                while (src < end)
                    AppendUTF8(fileName, DecodeUTF16(src));

                fileName = GetInternalPath(fileName);

                if (record->Action == FILE_ACTION_MODIFIED)
                    AddChange({ FILECHANGE_MODIFIED, fileName, EMPTY_STRING });
                else if (record->Action == FILE_ACTION_ADDED)
                    AddChange({ FILECHANGE_ADDED, fileName, EMPTY_STRING });
                else if (record->Action == FILE_ACTION_REMOVED)
                    AddChange({ FILECHANGE_REMOVED, fileName, EMPTY_STRING });
                else if (record->Action == FILE_ACTION_RENAMED_OLD_NAME)
                    rename.oldFileName_ = fileName;
                else if (record->Action == FILE_ACTION_RENAMED_NEW_NAME)
                    rename.fileName_ = fileName;

                if (!rename.oldFileName_.empty() && !rename.fileName_.empty())
                {
                    AddChange(rename);
                    rename = {};
                }

                if (!record->NextEntryOffset)
                    break;
                else
                    offset += record->NextEntryOffset;
            }
        }
    }
#elif defined(__linux__)
    unsigned char buffer[BUFFERSIZE];

    while (shouldRun_)
    {
        unsigned available = 0;
        ioctl(watchHandle_, FIONREAD, &available);

        if (available == 0)
        {
            Time::Sleep(100);
            continue;
        }

        int i = 0;
        auto length = (int)read(watchHandle_, buffer, Min(available, sizeof(buffer)));

        if (length < 0)
            return;

        ea::unordered_map<unsigned, FileChange> renames;
        while (i < length)
        {
            auto* event = (inotify_event*)&buffer[i];

            if (event->len > 0)
            {
                ea::string fileName = dirHandle_[event->wd] + event->name;

                if ((event->mask & IN_CREATE) == IN_CREATE)
                    AddChange({FILECHANGE_ADDED, fileName, EMPTY_STRING});
                else if ((event->mask & IN_DELETE) == IN_DELETE)
                    AddChange({FILECHANGE_REMOVED, fileName, EMPTY_STRING});
                else if ((event->mask & IN_MODIFY) == IN_MODIFY || (event->mask & IN_ATTRIB) == IN_ATTRIB)
                    AddChange({FILECHANGE_MODIFIED, fileName, EMPTY_STRING});
                else if (event->mask & IN_MOVE)
                {
                    auto& entry = renames[event->cookie];
                    if ((event->mask & IN_MOVED_FROM) == IN_MOVED_FROM)
                        entry.oldFileName_ = fileName;
                    else if ((event->mask & IN_MOVED_TO) == IN_MOVED_TO)
                        entry.fileName_ = fileName;

                    if (!entry.oldFileName_.empty() && !entry.fileName_.empty())
                    {
                        entry.kind_ = FILECHANGE_RENAMED;
                        AddChange(entry);
                    }
                }
            }

            i += sizeof(inotify_event) + event->len;
        }
    }
#elif defined(__APPLE__) && !defined(IOS) && !defined(TVOS)
    while (shouldRun_)
    {
        Time::Sleep(100);

        ea::string changes = ReadFileWatcher(watcher_);
        if (!changes.empty())
        {
            ea::vector<ea::string> fileChanges = changes.split('\n');
            FileChange change{};
            for (const ea::string& fileResult : fileChanges)
            {
                change.kind_ = (FileChangeKind)fileResult[0];   // First byte is change kind.
                ea::string fileName = &fileResult.at(1);
                if (change.kind_ == FILECHANGE_RENAMED)
                {
                    if (GetSubsystem<FileSystem>()->FileExists(fileName))
                        change.fileName_ = std::move(fileName);
                    else
                        change.oldFileName_ = std::move(fileName);

                    if (!change.fileName_.empty() && !change.oldFileName_.empty())
                    {
                        AddChange(change);
                        change = {};
                    }
                }
                else
                {
                    change.fileName_ = std::move(fileName);
                    AddChange(change);
                    change = {};
                }
            }
        }
    }
#endif
#endif
}

void FileWatcher::AddChange(const FileChange& change)
{
    MutexLock lock(changesMutex_);

    auto it = changes_.find(change.fileName_);
    if (it == changes_.end())
        changes_[change.fileName_].change_ = change;
    else
        // Reset the timer associated with the filename. Will be notified once timer exceeds the delay
        it->second.timer_.Reset();
}

bool FileWatcher::GetNextChange(FileChange& dest)
{
    MutexLock lock(changesMutex_);

    auto delayMsec = (unsigned)(delay_ * 1000.0f);

    if (changes_.empty())
        return false;
    else
    {
        for (auto i = changes_.begin(); i != changes_.end(); ++i)
        {
            if (i->second.timer_.GetMSec(false) >= delayMsec)
            {
                dest = i->second.change_;
                changes_.erase(i);
                return true;
            }
        }

        return false;
    }
}

}
