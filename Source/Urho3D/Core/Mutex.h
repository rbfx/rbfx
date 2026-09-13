// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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

/// No-op mutex. Useful for template code.
class DummyMutex
{
public:
    void Acquire() {}
    bool TryAcquire() { return true; }
    void Release() {}
 };

/// Lock that automatically acquires and releases a mutex.
template<typename T>
class MutexLock : private NonCopyable
{
public:
    /// Construct and acquire the mutex.
    explicit MutexLock(T& mutex) : mutex_(mutex) { mutex_.Acquire(); }
    /// Destruct. Release the mutex.
    ~MutexLock() { mutex_.Release(); }

private:
    /// Mutex reference.
    T& mutex_;
};

}
