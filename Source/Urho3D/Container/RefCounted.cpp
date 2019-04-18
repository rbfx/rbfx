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

#include "../Container/RefCounted.h"

#include "../DebugNew.h"

namespace Urho3D
{

RefCounted::RefCounted() :
    stl::enable_shared_from_this<RefCounted>()
{
    {
        // Creates refcount object early so assigning this object in subclass constructor to smart pointers works.
        stl::shared_ptr<RefCounted> ptr(this);
        // Ensure that shared pointer destructor does not free this object
        stl::Internal::atomic_increment(&mWeakPtr.get_refcount_pointer()->mRefCount);
    }
    // Restore proper refcount. Object may have 0 references and yet be alive when it is not managed by smart pointer.
    stl::Internal::atomic_decrement(&mWeakPtr.get_refcount_pointer()->mRefCount);
}

RefCounted::~RefCounted()
{
}

void RefCounted::AddRef()
{
    assert(!mWeakPtr.expired());
    mWeakPtr.get_refcount_pointer()->addref();
}

void RefCounted::ReleaseRef()
{
    assert(!mWeakPtr.expired());
    mWeakPtr.get_refcount_pointer()->release();
}

int RefCounted::Refs() const
{
    if (auto* refCount = mWeakPtr.get_refcount_pointer())
        return refCount->mRefCount;
    return 0;
}

int RefCounted::WeakRefs() const
{
    if (auto* refCount = mWeakPtr.get_refcount_pointer())
        return refCount->mWeakRefCount;
    return 0;
}

}
