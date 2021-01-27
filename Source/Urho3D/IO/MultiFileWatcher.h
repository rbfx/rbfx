//
// Copyright (c) 2017-2020 the rbfx project.
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
