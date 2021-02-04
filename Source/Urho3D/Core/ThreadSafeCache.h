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
