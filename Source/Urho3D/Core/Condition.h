// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
