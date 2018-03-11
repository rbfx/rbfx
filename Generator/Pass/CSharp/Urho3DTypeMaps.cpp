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

#include <cppast/cpp_template.hpp>
#include <fmt/format.h>
#include "GeneratorContext.h"
#include "Urho3DTypeMaps.h"


namespace Urho3D
{

bool Urho3DTypeMaps::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
        HandleType(entity->Ast<cppast::cpp_member_variable>().type());
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
        HandleType(entity->Ast<cppast::cpp_variable>().type());
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        HandleType(entity->Ast<cppast::cpp_member_function>().return_type());
        for (const auto& param : entity->children_)
            HandleType(param->Ast<cppast::cpp_function_parameter>().type());
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::function_t)
    {
        HandleType(entity->Ast<cppast::cpp_function>().return_type());
        for (const auto& param : entity->children_)
            HandleType(param->Ast<cppast::cpp_function_parameter>().type());
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::constructor_t)
    {
        for (const auto& param : entity->children_)
            HandleType(param->Ast<cppast::cpp_function_parameter>().type());
    }
    return true;
}

void Urho3DTypeMaps::HandleType(const cppast::cpp_type& type)
{
    const auto& realType = GetBaseType(type);
    if (realType.kind() != cppast::cpp_type_kind::template_instantiation_t)
        return;

    auto typeName = cppast::to_string(type);
    if (typeName.find("PODVector") == std::string::npos)
        return;

    // Typemap already generated
    if (generator->typeMaps_.find(typeName) != generator->typeMaps_.end())
        return;

    const auto& tpl = dynamic_cast<const cppast::cpp_template_instantiation_type&>(realType);
    std::string csType;

    if (tpl.arguments_exposed())
    {
        if (tpl.arguments().has_value())
        {
            const auto& args = tpl.arguments().value();
            const cppast::cpp_template_argument& tplArg = args[0u];
            const auto& tplType = tplArg.type().value();
            if (tplType.kind() == cppast::cpp_type_kind::builtin_t)
                csType = PrimitiveToPInvokeType(dynamic_cast<const cppast::cpp_builtin_type&>(tplType).builtin_type_kind());
            else if (tplType.kind() == cppast::cpp_type_kind::pointer_t)
                csType = Urho3D::GetTypeName(tplType);
        }
    }
    else
    {
        // cppast has no info for us. Make a best guess about the type in question.
        auto cppType = tpl.unexposed_arguments();
        auto primitiveType = PrimitiveToCppType(cppType);
        if (primitiveType == cppast::cpp_builtin_type_kind::cpp_void)
        {
            // Class pointer array
            if (cppType.rfind(" *") == cppType.length() - 2)
            {
                csType = cppType.substr(0, cppType.length() - 2);
                if (csType.find("const ") == 0)
                    csType = csType.substr(6);
                str::replace_str(csType, "::", ".");
                csType = "global::" + csType;
            }
        }
        else
        {
            // Builtin type
            csType = PrimitiveToPInvokeType(primitiveType);
        }
    }

    if (!csType.empty())
    {
        TypeMap map;
        map.cppType_ = typeName;
        map.cType_ = "Urho3D::VectorBase*";
        map.pInvokeType_ = "global::Urho3D.VectorBase*";
        map.csType_ = fmt::format("global::Urho3D.PODVector<{csType}>", FMT_CAPTURE(csType));
        map.cppToCTemplate_ = "script->AddRef<Urho3D::VectorBase>({value})";
        const auto& nonConstType = cppast::remove_const(type);
        const auto& conversionType = cppast::to_string(realType);
        if (nonConstType.kind() == cppast::cpp_type_kind::pointer_t)
            map.cToCppTemplate_ = fmt::format("({conversionType}*){{value}}", FMT_CAPTURE(conversionType));
        else
            map.cToCppTemplate_ = fmt::format("({conversionType}&)*{{value}}", FMT_CAPTURE(conversionType));
        map.csToPInvokeTemplate_ = fmt::format("global::Urho3D.PODVector<{csType}>.__ToPInvoke({{value}})", FMT_CAPTURE(csType));
        map.pInvokeToCSTemplate_ = fmt::format("global::Urho3D.PODVector<{csType}>.__FromPInvoke({{value}})", FMT_CAPTURE(csType));

        generator->typeMaps_[typeName] = map;
    }
}

}
