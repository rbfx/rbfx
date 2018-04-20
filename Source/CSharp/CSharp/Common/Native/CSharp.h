//
// Copyright (c) 2018 Rokas Kupstys.
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

namespace Urho3D
{

extern ScriptSubsystem* scriptSubsystem;

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

////////////////////////////////////// String converters ///////////////////////////////////////////////////////////////
template<typename T> struct CSharpConverter;

template<> struct CSharpConverter<MonoString>
{
    static inline MonoString* ToCSharp(const char* value) { return mono_string_new(mono_domain_get(), value); }
    static inline MonoString* ToCSharp(const String& value) { return mono_string_new(mono_domain_get(), value.CString()); }
    static inline MonoString* ToCSharp(const WString& value)
    {
        if (sizeof(wchar_t) == 2)
            return mono_string_new_utf16(mono_domain_get(), (const mono_unichar2*)value.CString(), value.Length());
        else
            return mono_string_new_utf32(mono_domain_get(), (const mono_unichar4*)value.CString(), value.Length());
    }

    template<typename T> static T FromCSharp(MonoString* value);
};

template<> inline String CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(MonoString* value)
{
    if (sizeof(wchar_t) == 2)
        return String((wchar_t*)mono_string_chars(value));
    else
    {
        auto* rawString = mono_string_to_utf8(value);
        String result(rawString);
        mono_free(rawString);
        return result;
    }
}

template<> inline Urho3D::WString CSharpConverter<MonoString>::FromCSharp<Urho3D::WString>(MonoString* value)
{
    return Urho3D::WString(CSharpConverter<MonoString>::FromCSharp<Urho3D::String>(value));
}

struct MonoStringHolder
{
    char* string;

    explicit MonoStringHolder(char* s) { string = s; }
    operator char*()                   { return string; }

    ~MonoStringHolder()
    {
        mono_free(string);
        string = nullptr;
    }
};

template<> inline MonoStringHolder CSharpConverter<MonoString>::FromCSharp<MonoStringHolder>(MonoString* value)
{
    return MonoStringHolder(mono_string_to_utf8(value));
}

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

#define MONO_INTERNAL_CALL(destination, function) \
    mono_add_internal_call(#destination "::" #function, (void*)&function)
