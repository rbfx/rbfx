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

#ifdef _WIN32
#   include "Urho3D/WindowsSupport.h"
#else
#   include <mutex>
#endif

#include <Urho3D/Urho3D.h>
#include "../Core/NonCopyable.h"
#include "../Core/Profiler.h"

#include <atomic>
#include <thread>

namespace Urho3D
{

#if _WIN32
namespace Detail
{
struct URHO3D_API CriticalSection
{
    /// Construct.
    inline CriticalSection() noexcept { InitializeCriticalSection(&lock_); }
    /// Destruct.
    inline ~CriticalSection() { DeleteCriticalSection(&lock_); }

    /// Acquire the mutex. Block if already acquired.
    inline void lock() { EnterCriticalSection(&lock_); }
    /// Try to acquire the mutex without locking. Return true if successful.
    inline bool try_lock() { return TryEnterCriticalSection(&lock_) != FALSE; }
    /// Release the mutex.
    inline void unlock() { LeaveCriticalSection(&lock_); }

private:
    CRITICAL_SECTION lock_;
};
}
using MutexType = Detail::CriticalSection;
#else
using MutexType = std::recursive_mutex;
#endif

/// Spinlock mutex.
class URHO3D_API SpinLockMutex
{
public:
    /// Acquire the mutex. Block if already acquired.
    void Acquire()
    {
        const unsigned ticket = newTicket_.fetch_add(1, std::memory_order_relaxed);
        for (int spinCount = 0; currentTicket_.load(std::memory_order_acquire) != ticket; ++spinCount)
        {
            if (spinCount < 16)
                ; //_mm_pause();
            else
            {
                std::this_thread::yield();
                spinCount = 0;
            }
        }
    }

    /// Release the mutex.
    void Release()
    {
        currentTicket_.store(currentTicket_.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }

private:
    /// Next ticket to be served.
    std::atomic_uint32_t newTicket_{ 0 };
    /// Currently processed ticket.
    std::atomic_uint32_t currentTicket_{ 0 };
};

/// Operating system mutual exclusion primitive.
class URHO3D_API Mutex
{
public:
    /// Acquire the mutex. Block if already acquired.
    void Acquire() { lock_.lock(); }
    /// Try to acquire the mutex without locking. Return true if successful.
    bool TryAcquire() { return lock_.try_lock(); }
    /// Release the mutex.
    void Release() { lock_.unlock(); }

private:
    /// Underlying mutex object.
    MutexType lock_;
};

#if URHO3D_PROFILING
class URHO3D_API ProfiledMutex
{
public:
    /// Construct. Pass URHO3D_PROFILE_SRC_LOCATION("custom comment") as parameter.
    explicit ProfiledMutex(const tracy::SourceLocationData* sourceLocationData) : lock_(sourceLocationData) { }

    /// Acquire the mutex. Block if already acquired.
    void Acquire() { lock_.lock(); }
    /// Try to acquire the mutex without locking. Return true if successful.
    bool TryAcquire() { return lock_.try_lock(); }
    /// Release the mutex.
    void Release() { lock_.unlock(); }

private:
    /// Underlying mutex object.
    tracy::Lockable<MutexType> lock_;
};
#else
using ProfiledMutex = Mutex;
#endif

/// Lock that automatically acquires and releases a mutex.
template<typename Mutex>
class MutexLock : private NonCopyable
{
public:
    /// Construct and acquire the mutex.
    explicit MutexLock(Mutex& mutex) : mutex_(mutex) { mutex_.Acquire(); }
    /// Destruct. Release the mutex.
    ~MutexLock() { mutex_.Release(); }

private:
    /// Mutex reference.
    Mutex& mutex_;
};

}
