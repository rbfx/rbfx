#pragma once
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/Log.h>
#include <string>
#include <type_traits>
#if URHO3D_CSHARP_MONO
#include <mono/utils/mono-publib.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>
#include <mono/metadata/appdomain.h>
#endif

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

    void QueueReleaseRef(RefCounted* instance)
    {
        MutexLock lock(mutex_);
        releaseQueue_.Push(instance);
    }

    void ReleaseRefCounted()
    {
        MutexLock lock(mutex_);
        for (auto* instance : releaseQueue_)
            instance->ReleaseRef();
        releaseQueue_.Clear();
    }

protected:
    HashMap<StringHash, const TypeInfo*> typeInfos_;
    PODVector<RefCounted*> releaseQueue_;
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
struct PInvokeArrayConverter { };

// Convert PODVector<T>
template<typename T>
struct PInvokeArrayConverter<PODVector<T>>
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
struct PInvokeArrayConverter<Vector<SharedPtr<T>>>
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

#if URHO3D_CSHARP_MONO
template<typename CppType> inline const char* GetMonoBuiltinType() { return "IntPtr"; };
template<> inline const char* GetMonoBuiltinType<bool>() { return "Boolean"; }
template<> inline const char* GetMonoBuiltinType<unsigned char>() { return "Byte"; }
template<> inline const char* GetMonoBuiltinType<signed char>() { return "SByte"; }
template<> inline const char* GetMonoBuiltinType<char>() { return "Char"; }
template<> inline const char* GetMonoBuiltinType<double>() { return "Double"; }
template<> inline const char* GetMonoBuiltinType<float>() { return "Single"; }
template<> inline const char* GetMonoBuiltinType<int>() { return "Int32"; }
template<> inline const char* GetMonoBuiltinType<long>() { return "Int32"; }
template<> inline const char* GetMonoBuiltinType<unsigned int>() { return "UInt32"; }
template<> inline const char* GetMonoBuiltinType<long long>() { return "Int64"; }
template<> inline const char* GetMonoBuiltinType<unsigned long long>() { return "UInt64"; }
template<> inline const char* GetMonoBuiltinType<short>() { return "Int16"; }
template<> inline const char* GetMonoBuiltinType<unsigned short>() { return "UInt16"; }

template<typename CppType>
struct MonoArrayConverter { };

// Convert PODVector<T>
template<typename T>
struct MonoArrayConverter<PODVector<T>>
{
    using CppType=PODVector<T>;
    using CType=MonoArray*;

    static CType ToCSharp(const CppType& value)
    {
        auto* klass = mono_class_from_name(mono_get_corlib(), "System", GetMonoBuiltinType<T>());
        auto* array = mono_array_new(mono_domain_get(), klass, value.Size());

        for (auto i = 0; i < value.Size(); i++)
            mono_array_set(array, T, i, value[i]);

        return array;
    }

    static CppType FromCSharp(CType value)
    {
        CppType result(mono_array_length(value));

        for (auto i = 0; i < result.Size(); i++)
            result[i] = mono_array_get(value, T, i);

        return result;
    }
};

// Convert Vector<SharedPtr<T>>
template<typename T>
struct MonoArrayConverter<Vector<SharedPtr<T>>>
{
    using CppType=Vector<SharedPtr<T>>;
    using CType=MonoArray*;

    static CType ToCSharp(const CppType& value)
    {
        auto* klass = mono_class_from_name(mono_get_corlib(), "System", "IntPtr");
        auto* array = mono_array_new(mono_domain_get(), klass, value.Size());

        for (auto i = 0; i < value.Size(); i++)
            mono_array_set(array, void*, i, (void*)value[i].Get());

        return array;
    }

    static CppType FromCSharp(CType value)
    {
        CppType result(mono_array_length(value));

        for (auto i = 0; i < result.Size(); i++)
            result[i] = (T*)mono_array_get(value, void*, i);

        return result;
    }
};
#endif

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

#if URHO3D_CSHARP_MONO
struct FreeMonoStringWhenDone
{
    char* string;

    explicit FreeMonoStringWhenDone(char* s)
    {
        string = s;
    }

    ~FreeMonoStringWhenDone()
    {
        mono_free(string);
        string = nullptr;
    }

    char* operator()() { return string; }
};
#endif
