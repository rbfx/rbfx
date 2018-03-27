#pragma once
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/Log.h>
#include <string>
#include <type_traits>

using namespace Urho3D;

struct ManagedInterface
{
    void(*FreeGCHandle)(void* gcHandle);
    void*(*CloneGCHandle)(void* gcHandle);
    Object*(*CreateObject)(Context* context, unsigned managedType);
};

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
        if (instance == nullptr)
            return nullptr;

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
        if (instance == nullptr)
            return nullptr;

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
        if (instance == nullptr)
            return nullptr;

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
    template<typename T> T* AddRef(const SharedPtr<T>& object)          { return object.Get(); }
    template<typename T> T* AddRef(const WeakPtr<T>& object)            { return object.Get(); }
    template<typename T> T* AddRef(const RefCountedType<T>* object)     { return const_cast<T*>(object); }
    // Type is copy-constructibe or rvalue reference, most likely being returned by value. Make a copy.
    template<typename T> T* AddRef(CopyableType<T>&& object)            { return (T*)TakePointerOwnership<T>(new T(object)); }
    template<typename T> T* AddRef(const CopyableType<T>&& object)      { return (T*)TakePointerOwnership<T>(new T(object)); }
    template<typename T> T* AddRef(CopyableType<T>& object)             { return (T*)TakePointerReference<T>(&object); }
    template<typename T> T* AddRef(const CopyableType<T>& object)       { return (T*)TakePointerReference<T>(&object); }
    template<typename T> T* AddRef(CopyableType<T>* object)             { return (T*)TakePointerReference<T>(object); }
    template<typename T> T* AddRef(const CopyableType<T>* object)       { return (T*)TakePointerReference<T>(object); }
    // Type is non-refcounted and non-copyable, return reference
    template<typename T> T* AddRef(const NonRefCountedType<T>&& object) { return (T*)TakePointerOwnership<T>(&object); }
    template<typename T> T* AddRef(const NonRefCountedType<T>& object)  { return (T*)TakePointerOwnership<T>(&object); }
    template<typename T> T* AddRef(const NonRefCountedType<T>* object)  { return (T*)TakePointerOwnership<T>(object); }
    // Pointer to any RefCounted object. Refcount increased/decreased as usual.
    template<typename T> T* TakeOwnership(RefCountedType<T>* object)    { return object; }
    template<typename T> T* TakeOwnership(NonRefCountedType<T>* object) { return (T*)TakePointerOwnership<T>(object); }
    template<typename T> T* TakeOwnership(CopyableType<T>* object)      { return (T*)TakePointerOwnership<T>(object); }
    // Type is some rvalue reference backed by existing storage - return a reference. User is responsible to make sure
    // everything does not blow up.
//    template<typename T> T* AddRef(const T& object)    { return TakePointerReference<T>(&object); }


    // Should usually not be called. Target runtime is responsible for freeing this string.
    const char* AddRef(const String& object)           { return strdup(object.CString()); }
    const char* AddRef(const std::string& object)      { return strdup(object.c_str()); }

    // RefCounted instances are released on managed side. Swallow these calls to avoid warning in function below.
    template<typename T> void ReleaseRef(RefCountedType<T>* object) { }

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

    template<typename T>
    void RegisterType()
    {
        typeInfos_[T::GetTypeStatic()] = T::GetTypeInfoStatic();
    }

    const TypeInfo* GetRegisteredType(StringHash type)
    {
        const TypeInfo* typeInfo = nullptr;
        typeInfos_.TryGetValue(type, typeInfo);
        return typeInfo;
    }

    ManagedInterface net_;

protected:
    Mutex lock_;
    Allocator<NativeObjectHandler> handlesAllocator_;
    HashMap<void*, NativeObjectHandler*> instanceToHandler_;
    HashMap<StringHash, const TypeInfo*> typeInfos_;
};

extern ScriptSubsystem* script;

/// Force-cast between incompatible types.
template<typename T0, typename T1>
inline T0 force_cast(T1 input)
{
    union
    {
        T1 input;
        T0 output;
    } u = { input };
    return u.output;
};

#define URHO3D_OBJECT_STATIC(typeName, baseTypeName) \
    public: \
        using ClassName = typeName; \
        using BaseClassName = baseTypeName; \
        static Urho3D::StringHash GetTypeStatic() { return GetTypeInfoStatic()->GetType(); } \
        static const Urho3D::String& GetTypeNameStatic() { return GetTypeInfoStatic()->GetTypeName(); } \
        static const Urho3D::TypeInfo* GetTypeInfoStatic() { static const Urho3D::TypeInfo typeInfoStatic(#typeName, BaseClassName::GetTypeInfoStatic()); return &typeInfoStatic; }


template<typename Derived, typename Base>
inline size_t GetBaseClassOffset()
{
    // Dragons be here
    return reinterpret_cast<uintptr_t>(static_cast<Base*>(reinterpret_cast<Derived*>(1))) - 1;
};

struct SafeArray
{
    void* data;
    unsigned size;
    bool owns;
};

template<typename CppType>
struct CSharpConverter { };

// Convert PODVector<T>
template<typename T>
struct CSharpConverter<PODVector<T>>
{
    using CppType=PODVector<T>;
    using CType=SafeArray;

    static CType ToCSharp(const CppType& value)
    {
        SafeArray result{nullptr, value.Size() * (unsigned)sizeof(T), true};
        result.data = malloc(result.size);
        memcpy(result.data, &value.Front(), result.size);
        return result;
    }

    static CppType FromCSharp(const SafeArray& value)
    {
        CppType result((const T*)value.data, value.size / (unsigned)sizeof(T));
        if (value.owns)
            free(value.data);
        return result;
    }
};

// Convert Vector<SharedPtr<T>>
template<typename T>
struct CSharpConverter<Vector<SharedPtr<T>>>
{
    using CppType=Vector<SharedPtr<T>>;
    using CType=SafeArray;

    static CType ToCSharp(const CppType& value)
    {
        SafeArray result{nullptr, value.Size() * (unsigned)sizeof(void*), true};
        result.data = malloc(result.size);
        auto** array = (T**)result.data;
        for (const auto& ptr : value)
            *array++ = ptr.Get();
        return result;
    }

    static CppType FromCSharp(const SafeArray& value)
    {
        CppType result{value.size / (unsigned)sizeof(void*)};
        auto** array = (T**)value.data;
        for (auto& ptr : result)
            ptr = *array++;
        if (value.owns)
            free(value.data);
        return result;
    }
};
