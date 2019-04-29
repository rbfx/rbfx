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

#pragma once


#include <EASTL/shared_ptr.h>
#include <EASTL/weak_ptr.h>

#include <Urho3D/Urho3D.h>

#include "../Container/Hash.h"

namespace Urho3D
{

/// Base class for intrusively reference-counted objects. These are noncopyable and non-assignable.
class URHO3D_API RefCounted : public ea::enable_shared_from_this<RefCounted>
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

    /// Increment reference count. Can also be called outside of a ea::shared_ptr for traditional reference counting.
    void AddRef();
    /// Decrement reference count and delete self if no more references. Can also be called outside of a ea::shared_ptr for traditional reference counting.
    void ReleaseRef();
    /// Return reference count.
    int Refs() const;
    /// Return weak reference count.
    int WeakRefs() const;
};

}
