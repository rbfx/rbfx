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

#include <Urho3D/Urho3D.h>

#ifndef _WIN32
#include <pthread.h>
using ThreadID = pthread_t;
#else
using ThreadID = unsigned;
#endif

#include "../Container/Str.h"

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
