// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../IO/MultiFileWatcher.h"

namespace Urho3D
{

bool MultiFileWatcher::StartWatching(const ea::string& pathName, bool watchSubDirs)
{
    auto watcher = MakeShared<FileWatcher>(context_);
    watcher->SetDelay(delay_);
    if (!watcher->StartWatching(pathName, watchSubDirs))
        return false;

    watchers_.push_back(watcher);
    return true;
}

void MultiFileWatcher::StopWatching()
{
    watchers_.clear();
}

void MultiFileWatcher::SetDelay(float interval)
{
    delay_ = Max(interval, 0.0f);
    for (FileWatcher* watcher : watchers_)
        watcher->SetDelay(delay_);
}

bool MultiFileWatcher::GetNextChange(FileChange& dest)
{
    for (FileWatcher* watcher : watchers_)
    {
        if (watcher->GetNextChange(dest))
            return true;
    }
    return false;
}

}
