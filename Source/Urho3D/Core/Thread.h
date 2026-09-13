// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Urho3D.h"

#ifndef _WIN32
#include <pthread.h>
using ThreadID = pthread_t;
#else
using ThreadID = unsigned;
#endif

#include "Urho3D/Container/Str.h"

namespace Urho3D
{

/// Operating system thread.
class URHO3D_API Thread
{
public:
    /// Construct. Does not start the thread yet.
    Thread(const ea::string& name=EMPTY_STRING);
    /// Destruct. If running, stop and wait for thread to finish.
    virtual ~Thread();

    /// The function to run in the thread.
    virtual void ThreadFunction() = 0;

    /// Start running the thread. Return true if successful, or false if already running or if can not create the thread.
    bool Run();
    /// Set the running flag to false and wait for the thread to finish.
    void Stop();
    /// Set thread priority. The thread must have been started first.
    void SetPriority(int priority);

    /// Return whether thread exists.
    bool IsStarted() const { return handle_ != nullptr; }
    /// Set name of the platform thread on supported platforms. Must be called before Run().
    void SetName(const ea::string& name);

    /// Set the current thread as the main thread.
    static void SetMainThread();
    /// Return the current thread's ID.
    /// @nobind
    static ThreadID GetCurrentThreadID();
    /// Return whether is executing in the main thread.
    static bool IsMainThread();

protected:
    /// Helper that executes Thread::ThreadFunction().
#if _WIN32
    static unsigned long __stdcall ThreadFunctionStatic(void* data);
#else
    static void* ThreadFunctionStatic(void* data);
#endif
    /// Name of the thread. It will be propagated to underlying OS thread if possible.
    ea::string name_{};
    /// Thread handle.
    void* handle_;
    /// Running flag.
    volatile bool shouldRun_;

    /// Main thread's thread ID.
    static ThreadID mainThreadID;
};

}
