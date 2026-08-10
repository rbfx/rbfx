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

#include "Urho3D/Core/AssertBase.h"

#include <EASTL/allocator.h>

#include <atomic>

namespace Urho3D
{

/// Reference count structure.
struct URHO3D_API RefCount
{
protected:
    using Allocator = EASTLAllocatorType;

    /// Construct.
    RefCount() = default;

public:
    /// Destruct.
    ~RefCount()
    {
        // Set reference counts below zero to fire asserts if this object is still accessed
        refs_.store(-1, std::memory_order_release);
        weakRefs_.store(-1, std::memory_order_release);
    }

    /// Allocate RefCount using its default allocator.
    static RefCount* Allocate();
    /// Free RefCount using its default allocator.
    static void Free(RefCount* instance);

    /// Add strong reference. Returns new reference count.
    /// If the object has been destroyed, behavior is undefined.
    int AddRefStrong()
    {
        const int oldValue = refs_.fetch_add(1, std::memory_order_relaxed);
        URHO3D_ASSERT(oldValue >= 0);
        return oldValue + 1;
    }

    /// Release strong reference. Returns new reference count.
    /// If 0 is returned, the object must be destroyed immediately.
    /// If the object has been destroyed, behavior is undefined.
    int ReleaseRefStrong()
    {
        const int oldValue = refs_.fetch_sub(1, std::memory_order_acq_rel);
        URHO3D_ASSERT(oldValue >= 1);
        return oldValue - 1;
    }

    /// Add weak reference. Returns new reference count.
    /// If the control block has been destroyed, behavior is undefined.
    int AddRefWeak()
    {
        const int oldValue = weakRefs_.fetch_add(1, std::memory_order_relaxed);
        URHO3D_ASSERT(oldValue >= 0);
        return oldValue + 1;
    }

    /// Release weak reference. Returns new reference count.
    /// If 0 is returned, the control block must be destroyed immediately.
    int ReleaseRefWeak()
    {
        const int oldValue = weakRefs_.fetch_sub(1, std::memory_order_acq_rel);
        URHO3D_ASSERT(oldValue >= 1);
        return oldValue - 1;
    }

    /// Try add strong reference. The object may be destroyed.
    /// Returns true if strong reference was added.
    bool TryAddRefStrong()
    {
        int count = refs_.load(std::memory_order_acquire);
        while (count > 0)
        {
            if (refs_.compare_exchange_weak(count, count + 1, std::memory_order_acquire, std::memory_order_relaxed))
                return true;
        }

        return false;
    }

    /// Mark object as destroyed.
    void MarkObjectDestroyed() { refs_.store(-1, std::memory_order_release); }

    /// Return strong reference count.
    int Refs() const { return refs_.load(std::memory_order_relaxed); }

    /// Return weak reference count.
    int WeakRefs() const { return weakRefs_.load(std::memory_order_relaxed); }

private:
    /// Reference count. If below zero, the object has been destroyed.
    std::atomic_int32_t refs_{0};
    /// Weak reference count.
    std::atomic_int32_t weakRefs_{0};
};

/// Base class for intrusively reference-counted objects. These are noncopyable and non-assignable.
class URHO3D_API RefCounted
{
public:
    /// Construct. Allocate the reference count structure and set an initial self weak reference.
    RefCounted();
    /// Destruct. Mark as expired and also delete the reference count structure if no outside weak references exist.
    virtual ~RefCounted();

    /// Prevent copy construction.
    RefCounted(const RefCounted& rhs) = delete;
    /// Prevent assignment.
    RefCounted& operator =(const RefCounted& rhs) = delete;

    /// Increment reference count. Can also be called outside of a SharedPtr for traditional reference counting. Returns new reference count value. Operation is atomic.
    /// @manualbind
    int AddRef();
    /// Decrement reference count and delete self if no more references. Can also be called outside of a SharedPtr for traditional reference counting. Returns new reference count value. Operation is atomic.
    /// @manualbind
    int ReleaseRef();
    /// Return reference count.
    /// @property
    int Refs() const;
    /// Return weak reference count.
    /// @property
    int WeakRefs() const;

    /// Return pointer to the reference count structure.
    RefCount* RefCountPtr() const { return refCount_; }

    /// Return true if script runtime object wrapping this native object exists.
    bool HasScriptObject() const
    {
#if URHO3D_CSHARP
        return scriptObject_ != nullptr;
#else
        return false;
#endif
    }

    /// Return true if script reference is strong.
    bool IsScriptStrongRef() const
    {
#if URHO3D_CSHARP
        return isScriptStrongRef_;
#else
        return false;
#endif
    }

protected:
#if URHO3D_CSHARP
    /// Returns handle to wrapper script object. This is scripting-runtime-dependent.
    void* GetScriptObject() const { return scriptObject_; }
    /// Sets handle to wrapper script object. This is scripting-runtime-dependent.
    void SetScriptObject(void* handle, bool isStrong);
    /// Clears script object value. Script object has to be freed externally.
    void ResetScriptObject();
#endif

private:
    /// Pointer to the reference count structure.
    RefCount* refCount_{};

#if URHO3D_CSHARP
    /// A handle to script object that wraps this native instance.
    void* scriptObject_{};
    /// GC Handle type (strong vs weak).
    bool isScriptStrongRef_{};
#endif
};

} // namespace Urho3D
