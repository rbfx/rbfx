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

/// Thread-safe dirty flag that can be used for maintaining caches.
class URHO3D_API DirtyFlag
{
public:
    /// Mark dirty. It's not safe to concurrently call MarkDirty and Clean.
    void MarkDirty() { dirty_.store(true, std::memory_order_relaxed); }

    /// Return whether is dirty.
    bool IsDirty() const { return dirty_.load(std::memory_order_acquire); }

    /// Clean dirty flag.
    void Clean() { dirty_.store(false, std::memory_order_release); }

    /// Execute callback thread-safely and clean dirty flag.
    template <class T>
    void Clean(T callback)
    {
        MutexLock<SpinLockMutex> lock(mutex_);
        callback();
        dirty_.store(false, std::memory_order_release);
    }

private:
    /// Whether dirty flag is set.
    std::atomic_bool dirty_ = true;
    /// Spinlock mutex for updating cached object.
    SpinLockMutex mutex_;
};

}
