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

    auto typeName = cppast::to_string(realType);

    // Typemap already generated
    if (generator->typeMaps_.find(typeName) != generator->typeMaps_.end())
        return;

    std::string csType;
    std::string pInvokeType = "IntPtr";
    std::string cppType;
    std::string vectorKind;
    bool isTypeMapped = false;

    if (typeName.find("PODVector<") == 0)
        vectorKind = "PODVector";
    else if (typeName.find("Vector<SharedPtr<") == 0)
        vectorKind = "Vector";
    else
        return;

    const auto& tpl = dynamic_cast<const cppast::cpp_template_instantiation_type&>(realType);
    if (tpl.arguments_exposed())
    {
        assert(false);  // TODO: finish implementing this when needed
        if (tpl.arguments().has_value())
        {
            const auto& args = tpl.arguments().value();
            const cppast::cpp_template_argument& tplArg = args[0u];
            const auto& tplType = tplArg.type().value();
            if (tplType.kind() == cppast::cpp_type_kind::builtin_t)
            {
                cppType = cppast::to_string(tplType);
                csType = pInvokeType = PrimitiveToPInvokeType(
                    dynamic_cast<const cppast::cpp_builtin_type&>(tplType).builtin_type_kind());
            }
            else if (tplType.kind() == cppast::cpp_type_kind::pointer_t)
                csType = Urho3D::GetTypeName(tplType);
        }
    }
    else
    {
        // cppast has no info for us. Make a best guess about the type in question.
        cppType = tpl.unexposed_arguments();
        auto primitiveType = PrimitiveToCppType(cppType);
        if (auto* map = generator->GetTypeMap(cppType))
        {
            isTypeMapped = true;
            if (map->isValueType_)  // is this needed?
            {
                csType = map->csType_;
                pInvokeType = map->pInvokeType_;
            }
        }
        else if (primitiveType == cppast::cpp_builtin_type_kind::cpp_void)
        {
            // Class pointer array
            if (cppType.find("SharedPtr<") == 0)                    // starts with
            {
                // Get T from SharedPtr<T>
                csType = cppType.substr(10, cppType.length() - 11);
            }
            else if (cppType.rfind(" *") == cppType.length() - 2)   // ends with
            {
                // Get pointer type
                csType = cppType.substr(0, cppType.length() - 2);
                if (csType.find("const ") == 0)
                    csType = csType.substr(6);
            }
            else
                csType = cppType;

            if (!container::contains(generator->symbols_, csType))
                // Undefined type, required because unknown types pass is yet to run
                return;

            str::replace_str(csType, "::", ".");
        }
        else
        {
            // Builtin type
            csType = pInvokeType = PrimitiveToPInvokeType(primitiveType);
        }
    }

    if (!csType.empty())
    {
        spdlog::get("console")->info("Auto-typemap: {}", typeName);
        TypeMap map;
        map.cppType_ = typeName;

        map.csType_ = fmt::format("{csType}[]", FMT_CAPTURE(csType));
        map.cType_ = "MonoArray*";
        map.pInvokeType_ = fmt::format("{pInvokeType}[]", FMT_CAPTURE(pInvokeType));
        map.cppToCTemplate_ = fmt::format(
            "MonoArrayConverter<Urho3D::{vectorKind}<{cppType}>>::ToCSharp({{value}})", FMT_CAPTURE(cppType),
            FMT_CAPTURE(vectorKind));
        map.cToCppTemplate_ = fmt::format(
            "MonoArrayConverter<Urho3D::{vectorKind}<{cppType}>>::FromCSharp({{value}})", FMT_CAPTURE(cppType),
            FMT_CAPTURE(vectorKind));
        if (isTypeMapped)
        {
            map.csToPInvokeTemplate_ = fmt::format("MarshalTools.ToPInvokeArray<{pInvokeType}, {csType}>({{value}})",
                FMT_CAPTURE(pInvokeType), FMT_CAPTURE(csType));
            map.pInvokeToCSTemplate_ = fmt::format("MarshalTools.ToCSharpArray<{pInvokeType}, {csType}>({{value}})",
                FMT_CAPTURE(pInvokeType), FMT_CAPTURE(csType));
        }
        else if (pInvokeType == "IntPtr")
        {
            map.csToPInvokeTemplate_ = fmt::format("MarshalTools.ToIntPtrArray<{csType}>({{value}})", FMT_CAPTURE(csType));
            map.pInvokeToCSTemplate_ = fmt::format("MarshalTools.ToObjectArray<{csType}>({{value}})", FMT_CAPTURE(csType));
        }
        map.isValueType_ = true;
        generator->typeMaps_[typeName] = map;
    }
}

}
