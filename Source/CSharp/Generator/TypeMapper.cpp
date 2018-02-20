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

void TypeMapper::Load(XMLFile* rules)
{
    XMLElement typeMaps = rules->GetRoot().GetChild("typemaps");
    for (auto typeMap = typeMaps.GetChild("typemap"); typeMap.NotNull(); typeMap = typeMap.GetNext("typemap"))
    {
        TypeMap map;
        map.cppType_ = typeMap.GetAttribute("type");
        map.cType_ = typeMap.GetAttribute("ctype");
        map.csType_ = typeMap.GetAttribute("cstype");
        map.pInvokeType_ = typeMap.GetAttribute("ptype");

        if (map.cType_.Empty())
            map.cType_ = map.cppType_;

        if (map.pInvokeType_.Empty())
            map.pInvokeType_ = ToPInvokeType(map.cType_, "");

        if (map.csType_.Empty())
            map.csType_ = map.pInvokeType_;

        if (auto cppToC = typeMap.GetChild("cpp_to_c"))
            map.cppToCTemplate_ = cppToC.GetValue();

        if (auto cToCpp = typeMap.GetChild("c_to_cpp"))
            map.cToCppTemplate_ = cToCpp.GetValue();

        if (auto toCS = typeMap.GetChild("pinvoke_to_cs"))
            map.pInvokeToCSTemplate_ = toCS.GetValue();

        if (auto toPInvoke = typeMap.GetChild("cs_to_pinvoke"))
            map.csToPInvokeTemplate_ = toPInvoke.GetValue();

        typeMaps_[map.cppType_] = map;
    }
}

const TypeMap* TypeMapper::GetTypeMap(const cppast::cpp_type& type)
{
    String baseName = Urho3D::GetTypeName(type);
    String fullName = cppast::to_string(type);

    auto it = typeMaps_.Find(baseName);
    if (it == typeMaps_.End())
        it = typeMaps_.Find(fullName);

    if (it != typeMaps_.End())
        return &it->second_;

    return nullptr;
}

const TypeMap* TypeMapper::GetTypeMap(const String& typeName)
{
    auto it = typeMaps_.Find(typeName);
    if (it != typeMaps_.End())
        return &it->second_;

    return nullptr;
}

String TypeMapper::ToCType(const cppast::cpp_type& type)
{
    if (const auto* map = GetTypeMap(type))
        return map->cType_;

    String typeName = cppast::to_string(type);

    if (IsEnumType(type))
        return typeName;

    if (IsComplexValueType(type))
        // A value type is turned into pointer.
        return "NativeObjectHandle*";

    // Builtin type
    return typeName;
}

String TypeMapper::ToPInvokeType(const cppast::cpp_type& type, const String& default_)
{
    if (const auto* map = GetTypeMap(type))
        return map->pInvokeType_;
    else if (IsEnumType(type))
        return "global::" + Urho3D::GetTypeName(type).Replaced("::", ".");
    else
    {
        String name = cppast::to_string(type);
        String result = ToPInvokeType(name, ToPInvokeType(Urho3D::GetTypeName(type), default_));
        return result;
    }
}

String TypeMapper::ToPInvokeType(const String& name, const String& default_)
{
    if (name == "char const*")
        return "string";
    if (name == "void*" || name == "signed char*")
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

String TypeMapper::ToPInvokeTypeReturn(const cppast::cpp_type& type, bool canCopy)
{
    String result = ToPInvokeType(cppast::remove_const(type));
    return result;
}

String TypeMapper::ToPInvokeTypeParam(const cppast::cpp_type& type)
{
    String result = ToPInvokeType(cppast::remove_const(type));
    if (result == "string")
        return "[param: MarshalAs(UnmanagedType.LPUTF8Str)]" + result;
    return result;
}

String TypeMapper::MapToC(const cppast::cpp_type& type, const String& expression, bool canCopy)
{
    const auto* map = GetTypeMap(type);
    String result = expression;

    if (map)
        result = fmt(map->cppToCTemplate_.CString(), {{"value", result.CString()}});
    else if (IsComplexValueType(type) || (IsVoid(type) && !canCopy))
        // Stinks! If cosntructor is being mapped then it's return value naturally is void in the AST. However
        // constructors immediately yield ownership of the object. Exploit `canCopy` to handle this case.
    {
        // A unmapped type. Refcounted objects will be returned as references ignoring `canCopy` value. Value types will
        // always get copied because it is the only sensible way to move them. Objects pointed by pointers will be
        // copied only if `canCopy` is `true`, otherwise handle will assume ownership if such object.
        result = fmt("script->GetObject{{#copy}}Copy{{/copy}}Handle({{value}})", {
            {"value", result.CString()},
            {"copy", canCopy},
        });
    }

    return result;
}

String TypeMapper::MapToCpp(const cppast::cpp_type& type, const String& expression)
{
    const auto* map = GetTypeMap(type);
    String result = expression;

    if (map)
        result = fmt(map->cToCppTemplate_.CString(), {{"value", result.CString()}});
    else if (IsComplexValueType(type))
    {
        result = fmt("({{value}})->instance_", {{"value", result.CString()}});
        result = fmt("({{type_name}}*)({{value}})", {
            {"type_name", Urho3D::GetTypeName(type).CString()},
            {"value",     result.CString()},
        });

        if (type.kind() != cppast::cpp_type_kind::pointer_t)
            result = "*" + result;
    }

    return result;
}

String TypeMapper::ToCSType(const cppast::cpp_type& type)
{
    String result;
    if (const auto* map = GetTypeMap(type))
        result = map->csType_;
    else if (generator->symbols_.Has(type))
        return "global::" + Urho3D::GetTypeName(type).Replaced("::", ".");
    else
        result = ToPInvokeType(type);
    return result;
}

String TypeMapper::MapToPInvoke(const cppast::cpp_type& type, const String& expression)
{
    String name = cppast::to_string(type);
    if (const auto* map = GetTypeMap(type))
        return fmt(map->csToPInvokeTemplate_.CString(), {{"value", expression.CString()}});
    else if (IsComplexValueType(type))
    {
        String returnType = "global::" + Urho3D::GetTypeName(type).Replaced("::", ".");
        return fmt("{{type}}.__ToPInvoke({{call}})", {{"type", returnType.CString()},
                                                      {"call", expression.CString()}});
    }
    return expression;
}

String TypeMapper::MapToCS(const cppast::cpp_type& type, const String& expression, bool canCopy)
{
    if (const auto* map = GetTypeMap(type))
        return fmt(map->pInvokeToCSTemplate_.CString(), {{"value", expression.CString()}});
    else if (IsComplexValueType(type))
    {
        String returnType = "global::" + Urho3D::GetTypeName(type).Replaced("::", ".");
        return fmt("{{type}}.__FromPInvoke({{call}})", {{"type", returnType.CString()},
                                                        {"call", expression.CString()}});
    }
    return expression;
}

}
