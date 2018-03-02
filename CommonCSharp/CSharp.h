#pragma once
#include <Urho3D/Urho3DAll.h>
#include <string>
#include <type_traits>

/// Object that manages lifetime of native object which was passed to managed runtime.
struct NativeObjectHandler
{
    /// Pointer to the instance of the object.
    void* instance_ = nullptr;
    /// Function that handles releasing of native resources. May be null.
    void (*deleter_)(NativeObjectHandler* handler) = nullptr;

    ~NativeObjectHandler()
    {
        if (deleter_ != nullptr)
            deleter_(this);
        deleter_ = nullptr;
        instance_ = nullptr;
    }
};

class ScriptSubsystem
{
public:
    RefCounted* AddRefCountedRef(RefCounted* instance)
    {
        MutexLock scoped(lock_);
        if (instanceToHandler_.Contains((void*)instance))
            return instance;

        instance->AddRef();
        auto* handler = handlesAllocator_.Reserve();
        handler->instance_ = (void*)instance;
        handler->deleter_ = [](NativeObjectHandler* handler) {
            ((RefCounted*)handler->instance_)->ReleaseRef();
        };
        instanceToHandler_[handler->instance_] = handler;
        return instance;
    }

    template<typename T> T* TakePointerOwnership(const T* instance)
    {
        MutexLock scoped(lock_);
        if (instanceToHandler_.Contains((void*)instance))
            return (T*)instance;

        auto* handler = handlesAllocator_.Reserve();
        handler->instance_ = (void*)instance;
        handler->deleter_ = [](NativeObjectHandler* handler){
            delete ((T*)handler->instance_);
        };
        instanceToHandler_[handler->instance_] = handler;
        return (T*)handler->instance_;
    }

    template<typename T> T* TakePointerReference(const T* instance)
    {
        MutexLock scoped(lock_);
        if (instanceToHandler_.Contains((void*)instance))
            return (T*)instance;

        auto* handler = handlesAllocator_.Reserve();
        handler->instance_ = (void*)instance;
        handler->deleter_ = nullptr;
        instanceToHandler_[handler->instance_] = handler;
        return (T*)handler->instance_;
    }

    template<typename T> using RefCountedType = typename std::enable_if<std::is_base_of<Urho3D::RefCounted, T>::value, T>::type;
    template<typename T> using NonRefCountedType = typename std::enable_if<!std::is_base_of<Urho3D::RefCounted, T>::value && !std::is_copy_constructible<T>::value, T>::type;
    template<typename T> using CopyableType = typename std::enable_if<!std::is_base_of<Urho3D::RefCounted, T>::value && std::is_copy_constructible<T>::value, T>::type;

    // Type is RefCounted, always return a reference.
    template<typename T> T* AddRef(const SharedPtr<T>& object)          { return (T*)AddRefCountedRef(object.Get()); }
    template<typename T> T* AddRef(const WeakPtr<T>& object)            { return (T*)AddRefCountedRef(object.Get()); }
    template<typename T> T* AddRef(const RefCountedType<T>* object)     { return (T*)AddRefCountedRef(const_cast<T*>(object)); }
    // Type is copy-constructibe or rvalue reference, most likely being returned by value. Make a copy.
    template<typename T> T* AddRef(const CopyableType<T>&& object)      { return (T*)TakePointerOwnership<T>(new T(object)); }
    template<typename T> T* AddRef(const CopyableType<T>& object)       { return (T*)TakePointerOwnership<T>(new T(object)); }
    template<typename T> T* AddRef(const CopyableType<T>* object)       { return (T*)TakePointerOwnership<T>(new T(*object)); }
    // Type is non-refcounted and non-copyable, return reference
    template<typename T> T* AddRef(const NonRefCountedType<T>&& object) { return (T*)TakePointerOwnership<T>(&object); }
    template<typename T> T* AddRef(const NonRefCountedType<T>& object)  { return (T*)TakePointerOwnership<T>(&object); }
    template<typename T> T* AddRef(const NonRefCountedType<T>* object)  { return (T*)TakePointerOwnership<T>(object); }
    // Pointer to any RefCounted object. Refcount increased/decreased as usual.
    template<typename T> T* TakeOwnership(RefCountedType<T>* object)    { return (T*)AddRefCountedRef(object); }
    template<typename T> T* TakeOwnership(NonRefCountedType<T>* object) { return (T*)TakePointerOwnership<T>(object); }
    template<typename T> T* TakeOwnership(CopyableType<T>* object)      { return (T*)TakePointerOwnership<T>(object); }
    // Type is some rvalue reference backed by existing storage - return a reference. User is responsible to make sure
    // everything does not blow up.
//    template<typename T> T* AddRef(const T& object)    { return TakePointerReference<T>(&object); }


    // Should usually not be called. Target runtime is responsible for freeing this string.
    const char* AddRef(const String& object)           { return strdup(object.CString()); }
    const char* AddRef(const std::string& object)      { return strdup(object.c_str()); }

    template<typename T>
    void ReleaseRef(T* instance)
    {
        MutexLock scoped(lock_);
        auto it = instanceToHandler_.Find((void*)instance);
        if (it == instanceToHandler_.End())
        {
            URHO3D_LOGERROR("Tried to release unreferenced script object!");
            return;
        }
        handlesAllocator_.Free(it->second_);
        instanceToHandler_.Erase(it);
    }

    NativeObjectHandler* GetHandler(void* instance)
    {
        MutexLock scoped(lock_);
        auto it = instanceToHandler_.Find(instance);
        if (it == instanceToHandler_.End())
            return nullptr;
        return it->second_;
    }

protected:
    Mutex lock_;
    Allocator<NativeObjectHandler> handlesAllocator_;
    HashMap<void*, NativeObjectHandler*> instanceToHandler_;
};

extern ScriptSubsystem* script;
