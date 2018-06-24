//
// Copyright (c) 2018 Rokas Kupstys
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Script/ScriptSubsystem.h>
#include <string>
#include <type_traits>

using namespace Urho3D;

#ifndef EXPORT_API
#   if _WIN32
#       define EXPORT_API __declspec(dllexport)
#   else
#       define EXPORT_API __attribute__ ((visibility("default")))
#   endif
#endif

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

struct CSharpObjConverter
{
    template<typename T> using RefCountedType = typename std::enable_if<std::is_base_of<Urho3D::RefCounted, T>::value, T>::type;
    template<typename T> using CopyableType = typename std::enable_if<!std::is_base_of<Urho3D::RefCounted, T>::value && std::is_copy_constructible<T>::value, T>::type;

    template<typename T> static T* ToCSharp(SharedPtr<T>&& object)               { return object.Detach(); }
    template<typename T> static T* ToCSharp(const SharedPtr<T>& object)          { return object.Get(); }
    template<typename T> static T* ToCSharp(SharedPtr<T>& object)                { return object.Get(); }
    template<typename T> static T* ToCSharp(const WeakPtr<T>& object)            { return object.Get(); }
    template<typename T> static T* ToCSharp(WeakPtr<T>& object)                  { return object.Get(); }
    template<typename T> static T* ToCSharp(const RefCountedType<T>* object)     { return const_cast<T*>(object); }
    template<typename T> static T* ToCSharp(CopyableType<T>&& object)            { return new T(std::move(object)); }
    template<typename T> static T* ToCSharp(const CopyableType<T>& object)       { return new T(object); }
    template<typename T> static T* ToCSharp(const CopyableType<T>* object)       { return object != nullptr ? new T(*object) : nullptr; }
};

template<typename T> struct CSharpConverter;

//////////////////////////////////////// Array converters //////////////////////////////////////////////////////////////

// Convert PODVector<T>
template<typename T>
struct CSharpConverter<PODVector<T>>
{
    using CppType=PODVector<T>;
    using CType=T*;

    // TODO: This can be optimized by passing &value.Front() memory buffer
    static CType ToCSharp(const CppType& value)
    {
        auto length = value.Size() * (unsigned)sizeof(T);
        auto* block = (T*)MarshalAllocator::Get().Alloc(length);
        memcpy(block, &value.Front(), length);
        return block;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};
        return CppType((const T*)value, MarshalAllocator::GetMemoryLength(value) / (unsigned)sizeof(T));
    }
};

// Convert Vector<SharedPtr<T>>
template<typename T>
struct CSharpConverter<Vector<SharedPtr<T>>>
{
    using CppType=Vector<SharedPtr<T>>;
    using CType=T**;

    static CType ToCSharp(const CppType& value)
    {
        auto** result = (T**)MarshalAllocator::Get().Alloc(value.Size() * (unsigned)sizeof(T*));
        auto** array = result;
        for (const auto& ptr : value)
            *array++ = ptr.Get();
        return result;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};
        CppType result{(unsigned)MarshalAllocator::GetMemoryLength(value) / (unsigned)sizeof(T*)};
        auto** array = (T**)value;
        for (auto& ptr : result)
            ptr = *array++;
        return result;
    }
};

// Convert Vector<WeakPtr<T>>
template<typename T>
struct CSharpConverter<Vector<WeakPtr<T>>>
{
    using CppType=Vector<WeakPtr<T>>;
    using CType=T**;

    static CType ToCSharp(const CppType& value)
    {
        auto** result = (T**)MarshalAllocator::Get().Alloc(value.Size() * (unsigned)sizeof(void*));
        auto** array = result;
        for (const auto& ptr : value)
            *array++ = ptr.Get();
        return result;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};
        CppType result{(unsigned)MarshalAllocator::GetMemoryLength(value) / (unsigned)sizeof(void*)};
        auto** array = (T**)value;
        for (auto& ptr : result)
            ptr = *array++;
        return result;
    }
};

// Convert Vector<T*>
template<typename T>
struct CSharpConverter<Vector<T*>>
{
    using CppType=Vector<T*>;
    using CType=T**;

    static CType ToCSharp(const CppType& value)
    {
        auto length = value.Size() * (unsigned)sizeof(void*);
        CType result = (CType)MarshalAllocator::Get().Alloc(length);
        memcpy(result, &value.Front(), length);
        return result;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};
        auto length = MarshalAllocator::GetMemoryLength(value);
        CppType result{length / (unsigned)sizeof(void*)};
        memcpy(&result.Front(), value, length);
        return result;
    }
};

template<>
struct CSharpConverter<Vector<StringHash>>
{
    using CppType=Vector<StringHash>;
    using CType=unsigned*;

    static CType ToCSharp(const CppType& value)
    {
        auto* result = (unsigned*)MarshalAllocator::Get().Alloc(value.Size() * (unsigned)sizeof(unsigned));
        auto* array = result;
        for (const auto& hash : value)
            *array++ = hash.ToHash();
        return result;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};

        CppType result{MarshalAllocator::GetMemoryLength(value) / (unsigned)sizeof(unsigned)};
        auto* array = (unsigned*)value;
        for (auto& hash : result)
            hash = StringHash(*array++);
        return result;
    }
};
////////////////////////////////////// String converters ///////////////////////////////////////////////////////////////

template<> struct CSharpConverter<Urho3D::String>
{
    // TODO: This can be optimized by passing &value.CString() memory buffer
    static inline const char* ToCSharp(const String& value)
    {
        auto* block = (char*)MarshalAllocator::Get().Alloc(value.Length() + 1);
        strcpy(block, value.CString());
        return block;
    }

    static inline Urho3D::String FromCSharp(const char* value)
    {
        return value;
    }
};

template<> struct CSharpConverter<char const*>
{
    // TODO: This can be optimized by passing value memory buffer
    static inline const char* ToCSharp(const char* value)
    {
        auto* block = (char*)MarshalAllocator::Get().Alloc(strlen(value) + 1);
        strcpy(block, value);
        return block;
    }

    static inline const char* FromCSharp(const char* value)
    {
        return value;
    }
};

template<> struct CSharpConverter<Urho3D::StringVector>
{
    static inline const char** ToCSharp(const Urho3D::StringVector& value)
    {
        const auto ptrArrayLength = (value.Size() + 1) * sizeof(const char*);
        unsigned memoryLength = ptrArrayLength;
        for (const auto& str : value)
            memoryLength += str.Length() + 1;

        auto* memory = MarshalAllocator::Get().Alloc(memoryLength);
        const char** ptrs = (const char**)memory;
        char* data = (char*)memory + ptrArrayLength;

        for (const auto& str : value)
        {
            strcpy(data, str.CString());
            ptrs[0] = data;
            data += str.Length() + 1;
            ptrs++;
        }

        return (const char**)memory;
    }

    static inline Urho3D::StringVector FromCSharp(const char** value)
    {
        Urho3D::Vector<Urho3D::String> result;
        if (value == nullptr)
            return result;

        while (*value)
            result.EmplaceBack(*value++);

        return result;
    }
};

///////////////////////////////////////// Utilities ////////////////////////////////////////////////////////////////////
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

