// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Mutex.h"

#include <atomic>

namespace Urho3D
{

/// Thread-safe cache that holds an object.
/// It's safe to:
/// - Invalidate cached object from multiple threads;
/// - Restore cached object from multiple threads, as long as all threads assign the same value.
/// It's unsafe to both invalidate and restore cached object from multiple threads simultaneously.
/// If different threads assign different values on Restore, cache will keep first provided value.
template <class T>
class ThreadSafeCache
{
public:
    /// Invalidate cached object.
    void Invalidate() { dirty_.store(true, std::memory_order_relaxed); }

    /// Return whether the object is invalid and has to be restored.
    bool IsInvalidated() const { return dirty_.load(std::memory_order_acquire); }

    /// Restore cached object. This call may be ignored if cache is already restored.
    void Restore(const T& object)
    {
        MutexLock<SpinLockMutex> lock(mutex_);
        if (dirty_.load(std::memory_order_acquire))
        {
            object_ = object;
            dirty_.store(false, std::memory_order_release);
        }
    }

    /// Same as Restore.
    ThreadSafeCache<T>& operator=(const T& object)
    {
        Restore(object);
        return *this;
    }

    /// Return object value. Intentionally unchecked, caller must ensure that cache is valid.
    const T& Get() const { return object_; }

private:
    /// Whether dirty flag is set.
    std::atomic_bool dirty_ = true;
    /// Spinlock mutex for updating cached object.
    SpinLockMutex mutex_;
    /// Cached object.
    T object_{};
};

}
