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
