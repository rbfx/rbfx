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

#include <EASTL/internal/thread_support.h>
#include <EASTL/utility.h>

#include "../Container/RefCounted.h"

#include <cassert>
#include <cstddef>
#include <utility>

namespace Urho3D
{

namespace Detail
{

/// Returns true if the type is derived from RefCounted.
template <class T>
constexpr bool IsRefCountedType = ea::is_base_of_v<RefCounted, T>;

/// Base class for shared pointer.
template <class InterfaceType, class RefCountedType, class Enabled = void>
class SharedPtrBase;

template <class InterfaceType>
class SharedPtrBase<InterfaceType, InterfaceType, void>
{
public:
    SharedPtrBase() noexcept = default;

    explicit SharedPtrBase(const SharedPtrBase& rhs) noexcept
        : refCounted_(rhs.refCounted_)
    {
        if (refCounted_ != nullptr)
            refCounted_->AddRef();
    }

    explicit SharedPtrBase(SharedPtrBase&& rhs) noexcept
        : refCounted_(rhs.refCounted_)
    {
        rhs.refCounted_ = nullptr;
    }

    SharedPtrBase(InterfaceType* ptr, RefCounted* refCounted) noexcept
        : refCounted_(ptr)
    {
        if (refCounted_ != nullptr)
            refCounted_->AddRef();
    }

    ~SharedPtrBase()
    {
        if (refCounted_)
            refCounted_->ReleaseRef();
    }

    void Swap(SharedPtrBase& rhs) noexcept
    {
        std::swap(refCounted_, rhs.refCounted_);
    }

    InterfaceType* GetRefCounted() const noexcept { return refCounted_; }
    InterfaceType* GetPointer() const noexcept { return refCounted_; }

private:
    InterfaceType* refCounted_{};
};

template <class InterfaceType>
class SharedPtrBase<InterfaceType, RefCounted, ea::enable_if_t<!IsRefCountedType<InterfaceType>>>
{
public:
    SharedPtrBase() noexcept = default;

    explicit SharedPtrBase(const SharedPtrBase& rhs) noexcept
        : refCounted_(rhs.refCounted_)
        , ptr_(rhs.ptr_)
    {
        if (refCounted_ != nullptr)
            refCounted_->AddRef();
    }

    explicit SharedPtrBase(SharedPtrBase&& rhs) noexcept
        : refCounted_(rhs.refCounted_)
        , ptr_(rhs.ptr_)
    {
        rhs.refCounted_ = nullptr;
        rhs.ptr_ = nullptr;
    }

    SharedPtrBase(InterfaceType* ptr, RefCounted* refCounted) noexcept
        : refCounted_(refCounted)
        , ptr_(ptr)
    {
        if (refCounted_ != nullptr)
            refCounted_->AddRef();
    }

    ~SharedPtrBase()
    {
        if (refCounted_)
            refCounted_->ReleaseRef();
    }

    void Swap(SharedPtrBase& rhs) noexcept
    {
        std::swap(refCounted_, rhs.refCounted_);
        std::swap(ptr_, rhs.ptr_);
    }

    RefCounted* GetRefCounted() const noexcept { return refCounted_; }
    InterfaceType* GetPointer() const noexcept { return ptr_; }

private:
    RefCounted* refCounted_{};
    InterfaceType* ptr_{};
};

}

/// Shared pointer template class with intrusive reference counting.
template <class InterfaceType, class RefCountedType = InterfaceType>
class SharedPtr : public Detail::SharedPtrBase<InterfaceType, RefCountedType>
{
public:
    using ThisType = SharedPtr<InterfaceType, RefCountedType>;
    using BaseType = Detail::SharedPtrBase<InterfaceType, RefCountedType>;

    SharedPtr() noexcept = default;
    SharedPtr(std::nullptr_t) noexcept {}   // NOLINT(google-explicit-constructor)
    SharedPtr(const ThisType& rhs) noexcept = default;
    SharedPtr(ThisType&& rhs) noexcept = default;

    /// Construct with explicit reference counter.
    SharedPtr(InterfaceType* ptr, RefCounted* refCounted) noexcept : BaseType(ptr, refCounted) {}

    /// Construct from another shared pointer.
    template <class U1, class U2>
    SharedPtr(const SharedPtr<U1, U2>& rhs) noexcept : BaseType(rhs.GetPointer(), rhs.GetRefCounted()) {} // NOLINT(google-explicit-constructor)

    /// Construct from a raw pointer.
    template <class U>
    explicit SharedPtr(U* ptr) noexcept : BaseType(ptr, ptr) {}

    /// Assign from another shared pointer.
    ThisType& operator=(const ThisType& rhs) noexcept
    {
        if (this->GetRefCounted() != rhs.GetRefCounted())
        {
            ThisType temp{rhs};
            this->Swap(temp);
        }
        return *this;
    }

    /// Assign from another shared pointer.
    template <class U1, class U2>
    ThisType& operator =(const SharedPtr<U1, U2>& rhs) noexcept
    {
        if (this->GetRefCounted() != rhs.GetRefCounted())
        {
            ThisType temp{rhs};
            this->Swap(temp);
        }
        return *this;
    }

    /// Move-assign from another shared pointer.
    ThisType& operator=(ThisType&& rhs) noexcept
    {
        if (this->GetRefCounted() != rhs.GetRefCounted())
        {
            ThisType temp{std::move(rhs)};
            this->Swap(temp);
        }
        return *this;
    }

    /// Move-assign from another shared pointer.
    template <class U1, class U2>
    ThisType& operator =(SharedPtr<U1, U2>&& rhs) noexcept
    {
        if (this->GetRefCounted() != rhs.GetRefCounted())
        {
            ThisType temp{ea::move(rhs)};
            this->Swap(temp);
        }
        return *this;
    }

    /// Assign from a raw pointer.
    template <class U>
    ThisType& operator =(U* ptr) noexcept
    {
        if (this->GetRefCounted() != ptr)
        {
            ThisType temp{ptr};
            this->Swap(temp);
        }
        return *this;
    }

    /// Point to the object.
    InterfaceType* operator ->() const noexcept
    {
        assert(this->GetPointer());
        return this->GetPointer();
    }

    /// Dereference the object.
    InterfaceType& operator *() const noexcept
    {
        assert(this->GetPointer());
        return *this->GetPointer();
    }

    /// Test for less than with another shared pointer.
    template <class U1, class U2> bool operator <(const SharedPtr<U1, U2>& rhs) const noexcept { return this->GetRefCounted() < rhs.GetRefCounted(); }

    /// Test for equality with another shared pointer.
    template <class U1, class U2> bool operator ==(const SharedPtr<U1, U2>& rhs) const noexcept { return this->GetRefCounted() == rhs.GetRefCounted(); }

    /// Test for inequality with another shared pointer.
    template <class U1, class U2> bool operator !=(const SharedPtr<U1, U2>& rhs) const noexcept { return this->GetRefCounted() != rhs.GetRefCounted(); }

    /// Convert to a raw pointer.
    operator InterfaceType*() const noexcept { return this->GetPointer(); }    // NOLINT(google-explicit-constructor)

    void Reset() noexcept
    {
        ThisType temp;
        this->Swap(temp);
    }

    /// Reset with another pointer.
    template <class U>
    void Reset(U* ptr) noexcept
    {
        ThisType temp(ptr);
        this->Swap(temp);
    }

    /// Reset with another pointers.
    void Reset(InterfaceType* ptr, RefCounted* refCounted) noexcept
    {
        ThisType temp(ptr, refCounted);
        this->Swap(temp);
    }

    /// Detach without destroying the object even if the refcount goes zero. To be used for scripting language interoperation.
    InterfaceType* Detach() noexcept
    {
        InterfaceType* ptr = this->GetPointer();
        if (ptr)
        {
            RefCount* refCount = RefCountPtr();
            ea::Internal::atomic_increment(&refCount->refs_); // 2 refs
            Reset(); // 1 ref
            ea::Internal::atomic_decrement(&refCount->refs_); // 0 refs
        }
        return ptr;
    }

    /// Perform a static cast from a shared pointer of another type.
    template <class U1, class U2> void StaticCast(const SharedPtr<U1, U2>& rhs) noexcept
    {
        ThisType temp(static_cast<InterfaceType*>(rhs.GetPointer()), rhs.GetRefCounted());
        this->Swap(temp);
    }

    /// Perform a dynamic cast from a shared pointer of another type.
    template <class U1, class U2> void DynamicCast(const SharedPtr<U1, U2>& rhs) noexcept
    {
        ThisType temp(dynamic_cast<InterfaceType*>(rhs.GetPointer()), rhs.GetRefCounted());
        this->Swap(temp);
    }

    /// Return the raw pointer.
    InterfaceType* Get() const noexcept { return this->GetPointer(); }

    /// Return the object's reference count, or 0 if the pointer is null.
    int Refs() const noexcept { return this->GetRefCounted() ? this->GetRefCounted()->Refs() : 0; }

    /// Return the object's weak reference count, or 0 if the pointer is null.
    int WeakRefs() const noexcept { return this->GetRefCounted() ? this->GetRefCounted()->WeakRefs() : 0; }

    /// Return pointer to the RefCount structure.
    RefCount* RefCountPtr() const noexcept { return this->GetRefCounted() ? this->GetRefCounted()->RefCountPtr() : nullptr; }

    /// Return hash value for HashSet & HashMap. Use the same hash function as for raw pointers!
    size_t ToHash() const noexcept { return size_t(uintptr_t(this->GetRefCounted())); }
};

/// Shared pointer alias with automatic type deduction. Can be used only for complete types!
template <class T>
using SharedPtrT = ea::conditional_t<Detail::IsRefCountedType<T>, SharedPtr<T, T>, SharedPtr<T, RefCounted>>;

/// Perform a static cast from one shared pointer type to another.
template <class T, class U1, class U2> SharedPtrT<T> StaticCast(const SharedPtr<U1, U2>& ptr) noexcept
{
    SharedPtrT<T> ret;
    ret.StaticCast(ptr);
    return ret;
}

/// Perform a dynamic cast from one weak pointer type to another.
template <class T, class U1, class U2> SharedPtrT<T> DynamicCast(const SharedPtr<U1, U2>& ptr) noexcept
{
    SharedPtrT<T> ret;
    ret.DynamicCast(ptr);
    return ret;
}

/// Weak pointer template class with intrusive reference counting. Does not keep the object pointed to alive.
template <class InterfaceType, class RefCountedType = InterfaceType>
class WeakPtr
{
public:
    using ThisType = WeakPtr<InterfaceType, RefCountedType>;
    using SharedPtrType = SharedPtr<InterfaceType, RefCountedType>;

    WeakPtr() noexcept = default;
    WeakPtr(std::nullptr_t) noexcept {} // NOLINT(google-explicit-constructor)

    /// Copy-construct from another weak pointer.
    WeakPtr(const ThisType& rhs) noexcept
        : ptr_(rhs.ptr_)
        , refCount_(rhs.refCount_)
    {
        AddRef();
    }

    /// Move-construct from another weak pointer.
    WeakPtr(ThisType&& rhs) noexcept
        : ptr_(rhs.ptr_)
        , refCount_(rhs.refCount_)
    {
        rhs.ptr_ = nullptr;
        rhs.refCount_ = nullptr;
    }

    /// Copy-construct from another weak pointer allowing implicit upcasting.
    template <class U1, class U2>
    WeakPtr(const WeakPtr<U1, U2>& rhs) noexcept   // NOLINT(google-explicit-constructor)
        : ptr_(rhs.ptr_)
        , refCount_(rhs.refCount_)
    {
        AddRef();
    }

    /// Construct from a shared pointer.
    template <class U1, class U2>
    WeakPtr(const SharedPtr<U1, U2>& rhs) noexcept   // NOLINT(google-explicit-constructor)
        : ptr_(rhs.Get())
        , refCount_(rhs.RefCountPtr())
    {
        AddRef();
    }

    /// Construct from a raw pointer.
    template <class U, ea::enable_if_t<ea::is_base_of_v<InterfaceType, U>, int> = 0>
    explicit WeakPtr(U* ptr) noexcept
        : ptr_(ptr)
        , refCount_(ptr ? ptr->RefCountPtr() : nullptr)
    {
        AddRef();
    }

    /// Construct from separate pointers.
    WeakPtr(InterfaceType* ptr, RefCounted* refCounted) noexcept
        : ptr_(ptr)
        , refCount_(refCounted->RefCountPtr())
    {
        AddRef();
    }

    /// Destruct. Release the weak reference to the object.
    ~WeakPtr() noexcept
    {
        ReleaseRef();
    }

    /// Assign from a shared pointer.
    template <class U1, class U2>
    ThisType& operator =(const SharedPtr<U1, U2>& rhs) noexcept
    {
        if (ptr_ == rhs.Get() && refCount_ == rhs.RefCountPtr())
            return *this;

        ThisType copy(rhs);
        Swap(copy);

        return *this;
    }

    /// Assign from a weak pointer.
    ThisType& operator =(const ThisType& rhs) noexcept
    {
        if (ptr_ == rhs.ptr_ && refCount_ == rhs.refCount_)
            return *this;

        ThisType copy(rhs);
        Swap(copy);

        return *this;
    }

    /// Move-assign from another weak pointer.
    ThisType& operator =(ThisType&& rhs) noexcept
    {
        ThisType copy(ea::move(rhs));
        Swap(copy);

        return *this;
    }

    /// Assign from another weak pointer allowing implicit upcasting.
    template <class U1, class U2>
    ThisType& operator =(const WeakPtr<U1, U2>& rhs) noexcept
    {
        if (ptr_ == rhs.ptr_ && refCount_ == rhs.refCount_)
            return *this;

        ReleaseRef();
        ptr_ = rhs.ptr_;
        refCount_ = rhs.refCount_;
        AddRef();

        return *this;
    }

    /// Assign from a raw pointer.
    ThisType& operator =(InterfaceType* ptr) noexcept
    {
        RefCount* refCount = ptr ? ptr->RefCountPtr() : nullptr;

        if (ptr_ == ptr && refCount_ == refCount)
            return *this;

        ReleaseRef();
        ptr_ = ptr;
        refCount_ = refCount;
        AddRef();

        return *this;
    }

    /// Convert to a shared pointer. If expired, return a null shared pointer.
    SharedPtrType Lock() const noexcept
    {
        static_assert(ea::is_convertible_v<InterfaceType*, RefCounted*>, "WeakPtr::Lock can be used only for types derived from RefCounted");
        if (Expired())
            return SharedPtrType();
        else
            return SharedPtrType(ptr_, static_cast<RefCounted*>(ptr_));
    }

    /// Return raw pointer. If expired, return null.
    InterfaceType* Get() const noexcept
    {
        return Expired() ? nullptr : ptr_;
    }

    /// Point to the object.
    InterfaceType* operator ->() const noexcept
    {
        assert(!Expired());
        return ptr_;
    }

    /// Dereference the object.
    InterfaceType& operator *() const noexcept
    {
        assert(!Expired());
        return *ptr_;
    }

    /// Test for equality with another weak pointer.
    template <class U1, class U2> bool operator ==(const WeakPtr<U1, U2>& rhs) const noexcept { return ptr_ == rhs.ptr_ && refCount_ == rhs.refCount_; }

    /// Test for inequality with another weak pointer.
    template <class U1, class U2> bool operator !=(const WeakPtr<U1, U2>& rhs) const noexcept { return ptr_ != rhs.ptr_ || refCount_ != rhs.refCount_; }

    /// Test for less than with another weak pointer.
    template <class U1, class U2> bool operator <(const WeakPtr<U1, U2>& rhs) const noexcept { return ptr_ < rhs.ptr_; }

    /// Convert to a raw pointer, null if the object is expired.
    operator InterfaceType*() const noexcept { return Get(); }   // NOLINT(google-explicit-constructor)

    /// Swap with another WeakPtr.
    void Swap(ThisType& rhs) noexcept
    {
        ea::swap(ptr_, rhs.ptr_);
        ea::swap(refCount_, rhs.refCount_);
    }

    /// Reset with another pointer.
    void Reset(InterfaceType* ptr = nullptr) noexcept
    {
        ThisType copy(ptr);
        Swap(copy);
    }

    /// Perform a static cast from a weak pointer of another type.
    template <class U1, class U2>
    void StaticCast(const WeakPtr<U1, U2>& rhs) noexcept
    {
        ReleaseRef();
        ptr_ = static_cast<InterfaceType*>(rhs.Get());
        refCount_ = rhs.refCount_;
        AddRef();
    }

    /// Perform a dynamic cast from a weak pointer of another type.
    template <class U1, class U2>
    void DynamicCast(const WeakPtr<U1, U2>& rhs) noexcept
    {
        ReleaseRef();
        ptr_ = dynamic_cast<InterfaceType*>(rhs.Get());

        if (ptr_)
        {
            refCount_ = rhs.refCount_;
            AddRef();
        }
        else
            refCount_ = 0;
    }

    /// Return the object's reference count, or 0 if null pointer or if object has expired.
    int Refs() const noexcept { return (refCount_ && refCount_->refs_ >= 0) ? refCount_->refs_ : 0; }

    /// Return the object's weak reference count.
    int WeakRefs() const noexcept
    {
        if (!Expired())
            return refCount_->weakRefs_ - 1;
        else
            return refCount_ ? refCount_->weakRefs_ : 0;
    }

    /// Return whether the object has expired. If null pointer, always return true.
    bool Expired() const noexcept { return refCount_ ? refCount_->refs_ < 0 : true; }

    /// Return pointer to the RefCount structure.
    RefCount* RefCountPtr() const noexcept { return refCount_; }

    /// Return hash value for HashSet & HashMap. Use the same hash function as for raw pointers!
    size_t ToHash() const noexcept { return size_t(uintptr_t(ptr_)); }

private:
    template <class U1, class U2> friend class WeakPtr;

    /// Add a weak reference to the object pointed to.
    void AddRef() noexcept
    {
        if (refCount_)
        {
            assert(refCount_->weakRefs_ >= 0);
            ea::Internal::atomic_increment(&refCount_->weakRefs_);
        }
    }

    /// Release the weak reference. Delete the Refcount structure if necessary.
    void ReleaseRef() noexcept
    {
        if (refCount_)
        {
            assert(refCount_->weakRefs_ > 0);
            int weakRefs = ea::Internal::atomic_decrement(&refCount_->weakRefs_);

            if (Expired() && weakRefs == 0)
                RefCount::Free(refCount_);
        }

        ptr_ = nullptr;
        refCount_ = nullptr;
    }

    /// Pointer to the object.
    InterfaceType* ptr_{};
    /// Pointer to the RefCount structure.
    RefCount* refCount_{};
};

/// Weak pointer alias with automatic type deduction. Can be used only for complete types!
template <class T>
using WeakPtrT = ea::conditional_t<Detail::IsRefCountedType<T>, WeakPtr<T, T>, WeakPtr<T, RefCounted>>;

/// Construct SharedPtr.
template <class T, class ... Args> SharedPtrT<T> MakeShared(Args && ... args)
{
    return SharedPtrT<T>(new T(std::forward<Args>(args)...));
}

#ifndef SWIG
template <class T> SharedPtr(T* ptr) -> SharedPtr<T, ea::conditional_t<Detail::IsRefCountedType<T>, T, RefCounted>>;
template <class T> WeakPtr(T* ptr) -> WeakPtr<T, ea::conditional_t<Detail::IsRefCountedType<T>, T, RefCounted>>;
#endif

}
