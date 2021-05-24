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

#include <EASTL/allocator.h>

#include <Urho3D/Urho3D.h>

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
        refs_ = -1;
        weakRefs_ = -1;
    }

    /// Allocate RefCount using it's default allocator.
    static RefCount* Allocate();
    /// Free RefCount using it's default allocator.
    static void Free(RefCount* instance);

    /// Reference count. If below zero, the object has been destroyed.
    int refs_ = 0;
    /// Weak reference count.
    int weakRefs_ = 0;
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
    RefCount* RefCountPtr() { return refCount_; }
#if URHO3D_CSHARP
    /// Return true if script runtime object wrapping this native object exists.
    bool HasScriptObject() const { return scriptObject_ != nullptr; }
    /// Return true if script reference is strong.
    bool IsScriptStrongRef() const { return isScriptStrongRef_; }

protected:
    /// Returns handle to wrapper script object. This is scripting-runtime-dependent.
    void* GetScriptObject() const { return scriptObject_; }
    /// Sets handle to wrapper script object. This is scripting-runtime-dependent.
    void SetScriptObject(void* handle, bool isStrong);
    /// Clears script object value. Script object has to be freed externally.
    void ResetScriptObject();
#endif
private:
    /// Pointer to the reference count structure.
    RefCount* refCount_ = nullptr;
#if URHO3D_CSHARP
    /// A handle to script object that wraps this native instance.
    void* scriptObject_ = nullptr;
    /// GC Handle type (strong vs weak).
    bool isScriptStrongRef_ = false;
#endif
};

}
