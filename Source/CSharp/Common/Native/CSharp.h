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


namespace Urho3D
{

URHO3D_API extern ScriptSubsystem* scriptSubsystem;

}

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

template<typename T> struct CSharpConverter;

//////////////////////////////////////// Array converters //////////////////////////////////////////////////////////////

// Convert PODVector<T>
template<typename T>
struct CSharpConverter<PODVector<T>>
{
    using CppType=PODVector<T>;
    using CType=MarshalAllocator::Block*;

    // TODO: This can be optimized by passing &value.Front() memory buffer
    static CType ToCSharp(const CppType& value)
    {
        if (value.Empty())
            return nullptr;

        auto* block = MarshalAllocator::Get().AllocArray<T>(value.Size());
        memcpy(block->memory_, &value.Front(), value.Size() * sizeof(T));
        return block;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};

        return CppType((const T*)value->memory_, value->itemCount_);
    }
};

// Convert Vector<SharedPtr<T>>
template<typename T>
struct CSharpConverter<Vector<SharedPtr<T>>>
{
    using CppType=Vector<SharedPtr<T>>;
    using CType=MarshalAllocator::Block*;

    static CType ToCSharp(const CppType& value)
    {
        if (value.Empty())
            return nullptr;

        CType block = MarshalAllocator::Get().AllocArray<T*>(value.Size());

        auto** array = (T**)block->memory_;
        for (const auto& ptr : value)
            *array++ = ptr.Get();

        return block;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};

        CppType result{(unsigned)value->itemCount_};
        auto** array = (T**)value->memory_;
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
    using CType=MarshalAllocator::Block*;

    static CType ToCSharp(const CppType& value)
    {
        if (value.Empty())
            return nullptr;

        CType block = MarshalAllocator::Get().AllocArray<T*>(value.Size());

        auto** array = (T**)block->memory_;
        for (const auto& ptr : value)
            *array++ = ptr.Get();

        return block;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};

        CppType result{(unsigned)value->itemCount_};
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
    using CType=MarshalAllocator::Block*;

    // TODO: This can be optimized by passing &value.Front() memory buffer
    static CType ToCSharp(const CppType& value)
    {
        if (value.Empty())
            return nullptr;

        CType result = MarshalAllocator::Get().AllocArray<T*>(value.Size());

        auto** array = (T**)result->memory_;
        for (const auto& ptr : value)
            *array++ = ptr;

        return result;
    }

    static CppType FromCSharp(CType value)
    {
        if (value == nullptr)
            return CppType{};

        CppType result{(unsigned)value->itemCount_};
        auto** array = (T**)value;
        for (auto& ptr : result)
            ptr = *array++;
        return result;
    }
};

////////////////////////////////////// String converters ///////////////////////////////////////////////////////////////

template<> struct CSharpConverter<Urho3D::String>
{
    // TODO: This can be optimized by passing &value.CString() memory buffer
    static inline MarshalAllocator::Block* ToCSharp(const String& value)
    {
        auto* block = MarshalAllocator::Get().Alloc(value.Length() + 1);
        strcpy((char*)block->memory_, value.CString());
        return block;
    }

    static inline Urho3D::String FromCSharp(MarshalAllocator::Block* value)
    {
        if (value == nullptr)
            return String::EMPTY;
        return Urho3D::String((const char*)value->memory_);
    }
};

template<> struct CSharpConverter<char const*>
{
    // TODO: This can be optimized by passing value memory buffer
    static inline MarshalAllocator::Block* ToCSharp(const char* value)
    {
        auto* block = MarshalAllocator::Get().Alloc(strlen(value) + 1);
        strcpy((char*)block->memory_, value);
        return block;
    }

    static inline const char* FromCSharp(MarshalAllocator::Block* value)
    {
        if (value == nullptr)
            return nullptr;
        return (const char*)value->memory_;
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

