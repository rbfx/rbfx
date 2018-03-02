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
    else if (IsComplexValueType(type))
        return default_;
    else
        return BuiltinToPInvokeType(type);
}

std::string TypeMapper::BuiltinToPInvokeType(const cppast::cpp_type& type)
{
    switch (type.kind())
    {

    case cppast::cpp_type_kind::builtin_t:
    {
        const auto& builtin = dynamic_cast<const cppast::cpp_builtin_type&>(type);
        switch (builtin.builtin_type_kind())
        {
        case cppast::cpp_void: return "void";
        case cppast::cpp_bool: return "bool";
        case cppast::cpp_uchar: return "byte";
        case cppast::cpp_ushort: return "ushort";
        case cppast::cpp_uint: return "uint";
        case cppast::cpp_ulong: return "uint";
        case cppast::cpp_ulonglong: return "ulong";
        case cppast::cpp_uint128: assert(false);
        case cppast::cpp_schar: return "byte";
        case cppast::cpp_short: return "short";
        case cppast::cpp_int: return "int";
        case cppast::cpp_long: return "int";
        case cppast::cpp_longlong: return "long";
        case cppast::cpp_int128: assert(false);
        case cppast::cpp_float: return "float";
        case cppast::cpp_double: return "double";
        case cppast::cpp_longdouble: assert(false);
        case cppast::cpp_float128: assert(false);
        case cppast::cpp_char: return "char";
        case cppast::cpp_wchar: assert(false);
        case cppast::cpp_char16: assert(false);
        case cppast::cpp_char32: assert(false);
        case cppast::cpp_nullptr: return "IntPtr";
        }
        break;
    }
    case cppast::cpp_type_kind::user_defined_t: return "IntPtr";
    case cppast::cpp_type_kind::cv_qualified_t:
    {
        auto name = BuiltinToPInvokeType(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
        if (name == "char*")
            return "string";
        return name;
    }
    case cppast::cpp_type_kind::pointer_t:
        return BuiltinToPInvokeType(dynamic_cast<const cppast::cpp_pointer_type&>(type).pointee()) + "*";
    case cppast::cpp_type_kind::reference_t:
        return BuiltinToPInvokeType(dynamic_cast<const cppast::cpp_reference_type&>(type).referee()) + "*";
    default:
        assert(false);
    }
}

std::string TypeMapper::ToPInvokeTypeReturn(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(type);
    return result;
}

std::string TypeMapper::ToPInvokeTypeParam(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(type);
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
    else if (generator->symbols_.Contains(type))
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
    else if (GetEntity(type) != nullptr)
        return "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
    else
        result = ToPInvokeType(type);
    return result;
}

std::string TypeMapper::MapToPInvoke(const cppast::cpp_type& type, const std::string& expression)
{
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
