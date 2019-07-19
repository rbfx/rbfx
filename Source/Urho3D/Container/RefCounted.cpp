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

#include "../Container/RefCounted.h"
#if URHO3D_CSHARP
#   include "../Script/Script.h"
#endif

namespace Urho3D
{

RefCounted::RefCounted()
{
    EASTLAllocatorType allocator;
    ea::default_delete<RefCounted> deleter;

    void* const pMemory = EASTLAlloc(allocator, sizeof(RefCount));
    assert(pMemory != nullptr);

    refCount_ = ::new(pMemory) RefCount(this, eastl::move(deleter), eastl::move(allocator));

    // RefCount is constructed with 1 strong ref and 1 weak ref. Urho3D expects 1 weak ref only.
    refCount_->mRefCount = 0;
}

RefCounted::~RefCounted()
{
    assert(refCount_);
    assert(refCount_->mRefCount == 0);
    assert(refCount_->mWeakRefCount > 0);

    // Mark object as expired, release the self weak ref and delete the refcount if no other weak refs exist
    refCount_->mRefCount = -1;
    refCount_->weak_release();
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
        Script::GetRuntimeApi()->Dispose(this);
    }
#endif
}

void RefCounted::AddRef()
{
    assert(refCount_->mRefCount >= 0);
    refCount_->addref();
}

void RefCounted::ReleaseRef()
{
    assert(refCount_->mRefCount > 0);
    refCount_->release();
}

int RefCounted::Refs() const
{
    return refCount_->mRefCount;
}

int RefCounted::WeakRefs() const
{
    // Subtract one to not return the internally held reference
    // Subtract strong refs because eastl increases both strong and weak refs when adding a reference
    return refCount_->mWeakRefCount - refCount_->mRefCount - 1;
}

}
