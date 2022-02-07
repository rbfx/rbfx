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

#if _WIN32
#   include "Urho3D/WindowsSupport.h"
#else
#   include <condition_variable>
#endif
#include <Urho3D/Urho3D.h>

namespace Urho3D
{

/// %Condition on which a thread can wait.
class URHO3D_API Condition
{
public:
#if _WIN32
    /// Construct.
    Condition()
    {
        event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    }
    /// Destruct.
    ~Condition()
    {
        CloseHandle(event_);
        event_ = nullptr;
    }
#endif

    /// Set the condition. Will be automatically reset once a waiting thread wakes up.
    void Set()
    {
#if _WIN32
        SetEvent(event_);
#else
        event_.notify_all();
#endif
    }

    /// Wait on the condition.
    void Wait()
    {
#if _WIN32
        WaitForSingleObject(event_, INFINITE);
#else
        std::unique_lock<std::mutex> lock(mutex_);
        event_.wait(lock);
        lock.unlock();
#endif
    }

private:
#if _WIN32
    /// Operating system specific event.
    HANDLE event_;
#else
    /// Mutex for the event, necessary for std-based implementation.
    std::mutex mutex_;
    /// Event variable.
    std::condition_variable event_;
#endif
};

}
