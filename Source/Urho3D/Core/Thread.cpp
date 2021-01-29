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

#include "../Core/Thread.h"
#include "../IO/Log.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <SDL/SDL_hints.h>
#if defined(__ANDROID_API__) && __ANDROID_API__ < 26
#include <linux/prctl.h>
#include <sys/prctl.h>
#endif

#include "../DebugNew.h"

namespace Urho3D
{

#ifdef URHO3D_THREADING
#ifdef _WIN32

#if !defined(UWP)
typedef HRESULT (WINAPI *pfnSetThreadDescription)(HANDLE, PCWSTR);
static auto pSetThreadDescription = (pfnSetThreadDescription) GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription");
#endif

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; /* must be 0x1000 */
    LPCSTR szName; /* pointer to name (in user addr space) */
    DWORD dwThreadID; /* thread ID (-1=caller thread) */
    DWORD dwFlags; /* reserved for future use, must be zero */
} THREADNAME_INFO;
#pragma pack(pop)


typedef HRESULT (WINAPI *pfnSetThreadDescription)(HANDLE, PCWSTR);

DWORD WINAPI Thread::ThreadFunctionStatic(void* data)
{
    Thread* thread = static_cast<Thread*>(data);

    // Borrowed from SDL_systhread.c
#if !defined(UWP)
    if (pSetThreadDescription)
        pSetThreadDescription(GetCurrentThread(), MultiByteToWide(thread->name_).c_str());
    else
#endif
    if (IsDebuggerPresent())
    {
        // Presumably some version of Visual Studio will understand SetThreadDescription(),
        // but we still need to deal with older OSes and debuggers. Set it with the arcane
        // exception magic, too.

        THREADNAME_INFO inf;

        /* C# and friends will try to catch this Exception, let's avoid it. */
        if (!SDL_GetHintBoolean(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, SDL_TRUE))
        {
            // This magic tells the debugger to name a thread if it's listening.
            SDL_zero(inf);
            inf.dwType = 0x1000;
            inf.szName = thread->name_.c_str();
            inf.dwThreadID = (DWORD) -1;
            inf.dwFlags = 0;

            // The debugger catches this, renames the thread, continues on.
            RaiseException(0x406D1388, 0, sizeof(inf) / sizeof(ULONG), (const ULONG_PTR*) &inf);
        }
    }

    thread->ThreadFunction();
    return 0;
}

#else

void* Thread::ThreadFunctionStatic(void* data)
{
    auto* thread = static_cast<Thread*>(data);
#if defined(__ANDROID_API__)
#if __ANDROID_API__ < 26
    prctl(PR_SET_NAME, thread->name_.c_str(), 0, 0, 0);
#else
    pthread_setname_np(pthread_self(), thread->name_.c_str(), thread->name_.Length());
#endif
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), thread->name_.c_str());
#elif defined(__MACOSX__) || defined(__IPHONEOS__)
    pthread_setname_np(thread->name_.c_str());
#endif

    thread->ThreadFunction();
    pthread_exit((void*)nullptr);
    return nullptr;
}

#endif
#endif // URHO3D_THREADING

ThreadID Thread::mainThreadID;

Thread::Thread(const ea::string& name) :
    handle_(nullptr),
    shouldRun_(false),
    name_(name)
{
}

Thread::~Thread()
{
    Stop();
}

bool Thread::Run()
{
#ifdef URHO3D_THREADING
    // Check if already running
    if (handle_)
        return false;

    shouldRun_ = true;
#ifdef _WIN32
    handle_ = CreateThread(nullptr, 0, ThreadFunctionStatic, this, 0, nullptr);
#else
    pthread_attr_t type;
    pthread_attr_init(&type);
    pthread_attr_setdetachstate(&type, PTHREAD_CREATE_JOINABLE);
    pthread_create((pthread_t*)&handle_, &type, ThreadFunctionStatic, this);

#endif
    return handle_ != nullptr;
#else
    return false;
#endif // URHO3D_THREADING
}

void Thread::Stop()
{
#ifdef URHO3D_THREADING
    // Check if already stopped
    if (!handle_)
        return;

    shouldRun_ = false;
#ifdef _WIN32
    WaitForSingleObject((HANDLE)handle_, INFINITE);
    CloseHandle((HANDLE)handle_);
#else
    auto thread = (pthread_t)handle_;
    if (thread)
        pthread_join(thread, nullptr);
#endif
    handle_ = nullptr;
#endif // URHO3D_THREADING
}

void Thread::SetPriority(int priority)
{
#ifdef URHO3D_THREADING
#ifdef _WIN32
    if (handle_)
        SetThreadPriority((HANDLE)handle_, priority);
#elif defined(__linux__) && !defined(__ANDROID__) && !defined(__EMSCRIPTEN__)
    auto thread = (pthread_t)handle_;
    if (thread)
        pthread_setschedprio(thread, priority);
#endif
#endif // URHO3D_THREADING
}

void Thread::SetMainThread()
{
    mainThreadID = GetCurrentThreadID();
}

ThreadID Thread::GetCurrentThreadID()
{
#ifdef URHO3D_THREADING
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return pthread_self();
#endif
#else
    return ThreadID();
#endif // URHO3D_THREADING
}

bool Thread::IsMainThread()
{
#ifdef URHO3D_THREADING
    return GetCurrentThreadID() == mainThreadID;
#else
    return true;
#endif // URHO3D_THREADING
}

void Thread::SetName(const ea::string& name)
{
    if (handle_ != nullptr)
    {
        URHO3D_LOGERROR("Thread name must be set before thread is started.");
        return;
    }
    name_ = name;
}

}
