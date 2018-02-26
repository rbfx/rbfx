//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include <Urho3D/Resource/XMLElement.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Printer/CodePrinter.h>
#include "TypeMapper.h"
#include "Utilities.h"
#include "GeneratorContext.h"


namespace Urho3D
{

TypeMapper::TypeMapper(Context* context)
    : Object(context)
{
}

void TypeMapper::Load(const JSONValue& rules)
{
    const JSONArray& typeMaps = rules.Get("typemaps").GetArray();
    for (const auto& typeMap : typeMaps)
    {
        TypeMap map;
        map.cppType_ = typeMap.Get("type").GetString().CString();
        map.cType_ = typeMap.Get("ctype").GetString().CString();
        map.csType_ = typeMap.Get("cstype").GetString().CString();
        map.pInvokeType_ = typeMap.Get("ptype").GetString().CString();

        if (map.cType_.empty())
            map.cType_ = map.cppType_;

        if (map.pInvokeType_.empty())
            map.pInvokeType_ = ToPInvokeType(map.cType_, "");

        if (map.csType_.empty())
            map.csType_ = map.pInvokeType_;

        const auto& cppToC = typeMap.Get("cpp_to_c");
        if (!cppToC.IsNull())
            map.cppToCTemplate_ = cppToC.GetString().CString();

        const auto& cToCpp = typeMap.Get("c_to_cpp");
        if (!cToCpp.IsNull())
            map.cToCppTemplate_ = cToCpp.GetString().CString();

        const auto& toCS = typeMap.Get("pinvoke_to_cs");
        if (!toCS.IsNull())
            map.pInvokeToCSTemplate_ = toCS.GetString().CString();

        const auto& toPInvoke = typeMap.Get("cs_to_pinvoke");
        if (!toPInvoke.IsNull())
            map.csToPInvokeTemplate_ = toPInvoke.GetString().CString();

        typeMaps_[map.cppType_] = map;
    }
}

const TypeMap* TypeMapper::GetTypeMap(const cppast::cpp_type& type)
{
    std::string baseName = Urho3D::GetTypeName(type);
    std::string fullName = cppast::to_string(type);

    auto it = typeMaps_.find(baseName);
    if (it == typeMaps_.end())
        it = typeMaps_.find(fullName);

    if (it != typeMaps_.end())
        return &it->second;

    return nullptr;
}

const TypeMap* TypeMapper::GetTypeMap(const std::string& typeName)
{
    auto it = typeMaps_.find(typeName);
    if (it != typeMaps_.end())
        return &it->second;

    return nullptr;
}

std::string TypeMapper::ToCType(const cppast::cpp_type& type)
{
    if (const auto* map = GetTypeMap(type))
        return map->cType_;

    std::string typeName = cppast::to_string(type);

    if (IsEnumType(type))
        return typeName;

    if (IsComplexValueType(type))
        // A value type is turned into pointer.
        return Urho3D::GetTypeName(type) + "*";

    // Builtin type
    return typeName;
}

std::string TypeMapper::ToPInvokeType(const cppast::cpp_type& type, const std::string& default_)
{
    if (const auto* map = GetTypeMap(type))
        return map->pInvokeType_;
    else if (IsEnumType(type))
        return "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
    else
    {
        std::string name = cppast::to_string(type);
        std::string result = ToPInvokeType(name, ToPInvokeType(Urho3D::GetTypeName(type), default_));
        return result;
    }
}

std::string TypeMapper::ToPInvokeType(const std::string& name, const std::string& default_)
{
    if (name == "char const*")
        return "string";
    if (name == "void*" || name == "signed char*" || name == "void const*")
        return "IntPtr";
    if (name == "char" || name == "signed char")
        return "char";
    if (name == "unsigned char")
        return "byte";
    if (name == "short")
        return "short";
    if (name == "unsigned short")
        return "ushort";
    if (name == "int")
        return "int";
    if (name == "unsigned int" || name == "unsigned")
        return "uint";
    if (name == "long long")
        return "long";
    if (name == "unsigned long long")
        return "ulong";
    if (name == "void")
        return "void";
    if (name == "bool")
        return "bool";
    if (name == "float")
        return "float";
    if (name == "double")
        return "double";
    if (name == "char const*")
        return "string";

    return default_;
}

std::string TypeMapper::ToPInvokeTypeReturn(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(cppast::remove_const(type));
    return result;
}

std::string TypeMapper::ToPInvokeTypeParam(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(cppast::remove_const(type));
    if (result == "string")
        return "[param: MarshalAs(UnmanagedType.LPUTF8Str)]" + result;
    return result;
}

std::string TypeMapper::MapToC(const cppast::cpp_type& type, const std::string& expression)
{
    const auto* map = GetTypeMap(type);
    std::string result = expression;

    if (map)
        result = fmt(map->cppToCTemplate_.c_str(), {{"value", result}});
    else if (IsComplexValueType(type))
    {
        result = fmt("script->AddRef<{{type}}>({{value}})", {
            {"value", result},
            {"type", Urho3D::GetTypeName(type)},
        });
    }

    return result;
}

std::string TypeMapper::MapToCNoCopy(const std::string& type, const std::string& expression)
{
    const auto* map = GetTypeMap(type);
    std::string result = expression;

    if (map)
        result = fmt(map->cppToCTemplate_.c_str(), {{"value", result}});
    else if (ToPInvokeType(type, "").empty())
    {
        result = fmt("script->TakeOwnership<{{type}}>({{value}})", {
            {"value", result},
            {"type", type},
        });
    }

    return result;
}

std::string TypeMapper::MapToCpp(const cppast::cpp_type& type, const std::string& expression)
{
    const auto* map = GetTypeMap(type);
    std::string result = expression;

    if (map)
        result = fmt(map->cToCppTemplate_.c_str(), {{"value", result}});
    else if (IsComplexValueType(type))
    {
        if (type.kind() != cppast::cpp_type_kind::pointer_t)
            result = "*" + result;
    }

    return result;
}

std::string TypeMapper::ToCSType(const cppast::cpp_type& type)
{
    std::string result;
    if (const auto* map = GetTypeMap(type))
        result = map->csType_;
    else if (generator->symbols_.Has(type))
        return "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
    else
        result = ToPInvokeType(type);
    return result;
}

std::string TypeMapper::MapToPInvoke(const cppast::cpp_type& type, const std::string& expression)
{
    std::string name = cppast::to_string(type);
    if (const auto* map = GetTypeMap(type))
        return fmt(map->csToPInvokeTemplate_.c_str(), {{"value", expression}});
    else if (IsComplexValueType(type))
    {
        std::string returnType = "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
        return fmt("{{type}}.__ToPInvoke({{call}})", {{"type", returnType},
                                                      {"call", expression}});
    }
    return expression;
}

std::string TypeMapper::MapToCS(const cppast::cpp_type& type, const std::string& expression)
{
    if (const auto* map = GetTypeMap(type))
        return fmt(map->pInvokeToCSTemplate_.c_str(), {{"value", expression}});
    else if (IsComplexValueType(type))
    {
        std::string returnType = "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
        return fmt("{{type}}.__FromPInvoke({{call}})", {{"type", returnType}, {"call", expression}});
    }
    return expression;
}

}
