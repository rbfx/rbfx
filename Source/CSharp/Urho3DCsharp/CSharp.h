#pragma once
#include <Urho3D/Urho3DAll.h>
#include <string>


/// Object that manages lifetime of native object which was passed to managed runtime.
struct NativeObjectHandle
{
    void* instance_ = nullptr;

    void (* deleter_)(NativeObjectHandle* handle) = nullptr;

    NativeObjectHandle(void* instance, void(* deleter)(NativeObjectHandle*))
        : instance_(instance), deleter_(deleter)
    {
    }

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
    /// Returns a native object handle which holds a reference to native refcounted object.
    NativeObjectHandle* GetObjectHandle(Urho3D::RefCounted* ref, bool doCopy)
    {
        MutexLock scoped(lock_);
        ref->AddRef();
        return handlesAllocator_.Reserve((void*) ref, [](NativeObjectHandle* handle)
        {
            ((Urho3D::RefCounted*) handle->instance_)->ReleaseRef();
        });
    }
    /// Returns a native object handle which holds a reference to native refcounted object.
    template<typename T>
    NativeObjectHandle* GetObjectHandle(const Urho3D::SharedPtr<T>& ref, bool doCopy)
    {
        MutexLock scoped(lock_);
        ref->AddRef();
        return handlesAllocator_.Reserve((void*) ref, [](NativeObjectHandle* handle)
        {
            ((T*) handle->instance_)->ReleaseRef();
        });
    }
    /// Returns a native object handle which holds a reference to native refcounted object.
    template<typename T>
    NativeObjectHandle* GetObjectHandle(const Urho3D::WeakPtr<T>& ref, bool doCopy)
    {
        MutexLock scoped(lock_);
        ref->AddRef();
        return handlesAllocator_.Reserve((void*) ref, [](NativeObjectHandle* handle)
        {
            ((T*) handle->instance_)->ReleaseRef();
        });
    }
    /// Returns a native object handle which holds a copy of native object.
    template<typename T>
    NativeObjectHandle* GetObjectHandle(const T& value, bool doCopy)
    {
        MutexLock scoped(lock_);
        return handlesAllocator_.Reserve((void*) new T(value), [](NativeObjectHandle* handle)
        {
            delete ((T*) handle->instance_);
        });
    }
    /// Returns a native object handle which holds a copy of native object.
    template<typename T>
    NativeObjectHandle* GetObjectHandle(const T* value, bool doCopy)
    {
        MutexLock scoped(lock_);
        if (doCopy)
            value = new T(*value);
        return handlesAllocator_.Reserve((void*)value, [](NativeObjectHandle* handle)
        {
            delete ((T*) handle->instance_);
        });
    }

    void FreeObjectHandle(NativeObjectHandle* handle)
    {
        MutexLock scoped(lock_);
        handlesAllocator_.Free(handle);
    }

protected:
    Mutex lock_;
    Allocator<NativeObjectHandle> handlesAllocator_;
};

extern ScriptSubsystem* script;
