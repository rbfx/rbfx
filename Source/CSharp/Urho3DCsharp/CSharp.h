#pragma once
#include <Urho3D/Urho3DAll.h>
#include <string>
#include <type_traits>


/// Object that manages lifetime of native object which was passed to managed runtime.
struct NativeObjectHandle
{
    void* instance_ = nullptr;
    void (* deleter_)(NativeObjectHandle* handle) = nullptr;

    ~NativeObjectHandle()
    {
        deleter_(this);
        deleter_ = nullptr;
        instance_ = nullptr;
    }
};

class ScriptSubsystem
{
public:
    /// Strings are always duplicated using strdup() and returned as pointers
    NativeObjectHandle* GetObjectHandle(const char* str) = delete;
    NativeObjectHandle* GetObjectHandle(const std::string& str) = delete;
    NativeObjectHandle* GetObjectHandle(const Urho3D::String& str) = delete;
    NativeObjectHandle* GetObjectHandle(const Urho3D::WString& str) = delete;
    NativeObjectHandle* GetObjectHandle(const std::wstring& str) = delete;

    /// Returns a native object handle which holds a copy of native object.
    template<typename T>
    NativeObjectHandle* GetObjectHandle(T* value)
    {
        MutexLock scoped(lock_);
        auto* handle = handlesAllocator_.Reserve();
        TypeHandler<T, std::is_base_of<Urho3D::RefCounted, T>::value, std::is_copy_constructible<T>::value>::AddRef(value);
        handle->instance_ = (void*)value;
        handle->deleter_ = [](NativeObjectHandle* handle) { TypeHandler<T, std::is_base_of<Urho3D::RefCounted, T>::value, std::is_copy_constructible<T>::value>::ReleaseRef((T*) handle->instance_); };
        return handle;
    }
    template<typename T>
    NativeObjectHandle* GetObjectCopyHandle(T* value)
    {
        T* copy = TypeHandler<T, std::is_base_of<Urho3D::RefCounted, T>::value, std::is_copy_constructible<T>::value>::Copy(value);
        return GetObjectHandle(copy);  // Takes ownership of `copy`.
    }

    /// Returns a native object handle which holds a copy of native object.
    template<typename T>
    NativeObjectHandle* GetObjectHandle(const T& value) { return GetObjectHandle(const_cast<T*>(&value)); }
    template<typename T>
    NativeObjectHandle* GetObjectCopyHandle(const T& value) { return GetObjectCopyHandle(const_cast<T*>(&value)); }

    void FreeObjectHandle(NativeObjectHandle* handle)
    {
        MutexLock scoped(lock_);
        handlesAllocator_.Free(handle);
    }

protected:
    template<typename T, bool TRefCounted, bool TCopyable> struct TypeHandler { };

    // Refcounted
    template<typename T> struct TypeHandler<T, true, true>
    {
        static void AddRef(T* object) { object->AddRef(); }
        static void ReleaseRef(T* object) { object->ReleaseRef(); }
        static T* Copy(T* object) { return object; }
    };
    template<typename T> struct TypeHandler<T, true, false>
    {
        static void AddRef(T* object) { object->AddRef(); }
        static void ReleaseRef(T* object) { object->ReleaseRef(); }
        static T* Copy(T* object) { return object; }
    };

    // Copyable
    template<typename T> struct TypeHandler<T, false, true>
    {
        static void AddRef(T* object) { }
        static void ReleaseRef(T* object) { delete object; }
        static T* Copy(T* object) { return new T(*object); }
    };

    // Non-copyable
    template<typename T> struct TypeHandler<T, false, false>
    {
        static void AddRef(T* object) { }
        static void ReleaseRef(T* object) { }
        static T* Copy(T* object) { return object; }
    };

    Mutex lock_;
    Allocator<NativeObjectHandle> handlesAllocator_;
};

extern ScriptSubsystem* script;
