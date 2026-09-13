// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../IO/FileWatcher.h"

namespace Urho3D
{

/// Watches a set of directories for files being modified.
class URHO3D_API MultiFileWatcher : public Object
{
    URHO3D_OBJECT(MultiFileWatcher, Object);

public:
    /// Construct.
    explicit MultiFileWatcher(Context* context) : Object(context) {}

    /// Start watching a directory. Return true if successful.
    bool StartWatching(const ea::string& pathName, bool watchSubDirs);
    /// Stop watching all the directories.
    void StopWatching();
    /// Set the delay in seconds before file changes are notified. This (hopefully) avoids notifying when a file save is still in progress. Default 1 second.
    void SetDelay(float interval);
    /// Return a file change (true if was found, false if not).
    bool GetNextChange(FileChange& dest);

    /// Return the delay in seconds for notifying file changes.
    float GetDelay() const { return delay_; }

private:
    /// Individual watchers.
    ea::vector<SharedPtr<FileWatcher>> watchers_;
    /// Notification delay.
    float delay_{ 1.0f };
};

}
