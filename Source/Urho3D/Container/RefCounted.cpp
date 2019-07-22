//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include <cassert>

#include <EASTL/internal/thread_support.h>

#include "../Container/RefCounted.h"
#include "../Core/Macros.h"
#if URHO3D_CSHARP
#   include "../Script/Script.h"
#endif

namespace Urho3D
{

RefCount* RefCount::Allocate()
{
    void* const memory = EASTLAlloc(*ea::get_default_allocator((Allocator*)nullptr), sizeof(RefCount));
    assert(memory != nullptr);
    return ::new(memory) RefCount();
}

void RefCount::Free(RefCount* instance)
{
    instance->~RefCount();
    EASTLFree(*ea::get_default_allocator((Allocator*)nullptr), instance, sizeof(RefCount));
}

RefCounted::RefCounted()
    : refCount_(RefCount::Allocate())
{
    // Hold a weak ref to self to avoid possible double delete of the refcount
    refCount_->weakRefs_++;
}

RefCounted::~RefCounted()
{
    assert(refCount_);
    assert(refCount_->refs_ == 0);
    assert(refCount_->weakRefs_ > 0);

    // Mark object as expired, release the self weak ref and delete the refcount if no other weak refs exist
    refCount_->refs_ = -1;

    if (ea::Internal::atomic_decrement(&refCount_->weakRefs_) == 0)
        RefCount::Free(refCount_);

    refCount_ = nullptr;

#if URHO3D_CSHARP
    if (scriptObject_)
    {
        // Last reference to this object was released while we still have a handle to managed script. This happens only
        // when this object is wrapped as a director class and user inherited from it. User lost all managed references
        // to this class, however engine kept a reference. It is then possible that managed finalizer was executed and
        // wrapper replaced weak reference with a strong one and resurrected managed object. This happens because native
        // instance depends on managed instance for logic implementation. So we likely have a strong reference and
        // therefore we must dispose of managed object, release this strong reference in order to allow garbage
        // collection of managed object.
        if (ScriptRuntimeApi* api = Script::GetRuntimeApi())
        {
            api->FreeGCHandle(scriptObject_);
            scriptObject_ = 0;
        }
    }
#endif
}

int RefCounted::AddRef()
{
    int refs = ea::Internal::atomic_increment(&refCount_->refs_);
    assert(refs > 0);

#if URHO3D_CSHARP
    if (URHO3D_UNLIKELY(refs == 2 && scriptObject_))
    {
        // There is at least 1 script reference and 1 engine reference
        // Convert to strong ref in order to prevent freeing of script object
        if (ScriptRuntimeApi* api = Script::GetRuntimeApi())
            scriptObject_ = api->RecreateGCHandle(scriptObject_, true);
    }
#endif
    return refs;
}

int RefCounted::ReleaseRef()
{
    int refs = ea::Internal::atomic_decrement(&refCount_->refs_);
    assert(refs >= 0);

    if (refs == 0)
        delete this;

#if URHO3D_CSHARP
    else if (URHO3D_UNLIKELY(refs == 1 && scriptObject_))
    {
        // Only script object holds 1 reference to this native object
        // Convert to weak ref in order to allow object deletion when script runtime no longer references this object
        if (ScriptRuntimeApi* api = Script::GetRuntimeApi())
            scriptObject_ = api->RecreateGCHandle(scriptObject_, false);
    }
#endif
    return refs;
}

int RefCounted::Refs() const
{
    return refCount_->refs_;
}

int RefCounted::WeakRefs() const
{
    // Subtract one to not return the internally held reference
    return refCount_->weakRefs_ - 1;
}
#if URHO3D_CSHARP
uintptr_t RefCounted::SwapScriptObject(uintptr_t handle)
{
    ea::swap(handle, scriptObject_);
    return handle;
}
#endif
}
