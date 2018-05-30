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
#include <string>
#include <type_traits>
#include <mono/utils/mono-publib.h>
#include <mono/metadata/object.h>
#include <mono/metadata/class.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/loader.h>

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

extern ScriptSubsystem* scriptSubsystem;

}

using gchandle = void*;

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

struct SafeArray
{
    void* data;
    int size;
};

template<typename T>
static void* GetScratchSpace(int length)
{
    static thread_local int scratchLength = 0;
    static thread_local void* scratch = nullptr;

    if (scratchLength < length)
    {
        scratchLength = length;
        scratch = realloc(scratch, scratchLength);
    }
    return scratch;
}

// Convert PODVector<T>
template<typename T>
struct CSharpConverter<PODVector<T>>
{
    using CppType=PODVector<T>;
    using CType=SafeArray;

    static CType ToCSharp(const CppType& value)
    {
        return {(void*)&value.Front(), (int)(value.Size() * sizeof(T))};
    }

    static CType ToCSharp(const CppType&& value)
    {
        int length = (int)value.Size() * sizeof(T);
        auto* memory = GetScratchSpace<CSharpConverter<PODVector<T>>>(length);
        memcpy(memory, &value.Front(), length);
        return {memory, length};
    }

    static CppType FromCSharp(const SafeArray& value)
    {
        return CppType((const T*)value.data, (unsigned)value.size / (unsigned)sizeof(T));
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
        int length = (int)value.Size() * sizeof(void*);
        auto* memory = GetScratchSpace<CSharpConverter<Vector<SharedPtr<T>>>>(length);

        auto** array = (T**)memory;
        for (const auto& ptr : value)
            *array++ = ptr.Get();

        return {memory, length};
    }

    static CppType FromCSharp(const SafeArray& value)
    {
        CppType result{(unsigned)value.size / (unsigned)sizeof(void*)};
        auto** array = (T**)value.data;
        for (auto& ptr : result)
            ptr = *array++;
        return result;
    }
};

////////////////////////////////////// String converters ///////////////////////////////////////////////////////////////

template<> struct CSharpConverter<Urho3D::String>
{
    static inline const char* ToCSharp(const char* value) { return value; }
    static inline const char* ToCSharp(const String& value) { return value.CString(); }
    static inline const char* ToCSharp(const String&& value) { return strdup(value.CString()); }
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

