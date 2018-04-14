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

class ScriptSubsystem
{
public:
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

    void QueueForDeletion(RefCounted* instance)
    {
        MutexLock lock(mutex_);
        deletionQueue_.Push(instance);
    }

    void DeleteRefCounted()
    {
        MutexLock lock(mutex_);
        for (auto* instance : deletionQueue_)
            delete instance;
        deletionQueue_.Clear();
    }

protected:
    HashMap<StringHash, const TypeInfo*> typeInfos_;
    PODVector<RefCounted*> deletionQueue_;
    Mutex mutex_;
};

extern ManagedInterface managedAPI;
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

struct CSharpObjConverter
{
    template<typename T> using RefCountedType = typename std::enable_if<std::is_base_of<Urho3D::RefCounted, T>::value, T>::type;
    template<typename T> using NonRefCountedType = typename std::enable_if<!std::is_base_of<Urho3D::RefCounted, T>::value && !std::is_copy_constructible<T>::value, T>::type;
    template<typename T> using CopyableType = typename std::enable_if<!std::is_base_of<Urho3D::RefCounted, T>::value && std::is_copy_constructible<T>::value, T>::type;

    template<typename T> static T* ToCSharp(const SharedPtr<T>& object)          { return object.Get(); }
    template<typename T> static T* ToCSharp(const WeakPtr<T>& object)            { return object.Get(); }
    template<typename T> static T* ToCSharp(const RefCountedType<T>* object)     { return const_cast<T*>(object); }
    template<typename T> static T* ToCSharp(CopyableType<T>&& object)            { return new T(object); }
    template<typename T> static T* ToCSharp(const CopyableType<T>& object)       { return (T*)&object; }
    template<typename T> static T* ToCSharp(const CopyableType<T>* object)       { return (T*)object; }
    template<typename T> static T* ToCSharp(const NonRefCountedType<T>&& object) { return (T*)&object; }
    template<typename T> static T* ToCSharp(const NonRefCountedType<T>& object)  { return (T*)&object; }
    template<typename T> static T* ToCSharp(const NonRefCountedType<T>* object)  { return (T*)object; }
};

template<typename T>
std::uintptr_t GetTypeID()
{
    return reinterpret_cast<std::uintptr_t>(&typeid(T));
}

template<typename T>
std::uintptr_t GetTypeID(const T* instance)
{
    return reinterpret_cast<std::uintptr_t>(&typeid(*instance));
}
