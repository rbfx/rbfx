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

#include <cppast/cpp_template.hpp>
#include <fmt/format.h>
#include "GeneratorContext.h"
#include "Urho3DTypeMaps.h"


namespace Urho3D
{

bool Urho3DTypeMaps::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->ast_ == nullptr)
        return true;

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
    if (generator->GetTypeMap(typeName) != nullptr)
        return;

    std::string csType;
    std::string pInvokeType = "IntPtr";
    std::string cppType;
    std::string vectorKind;
    const TypeMap* typeMap = nullptr;

    if (str::starts_with(typeName, "PODVector<"))
        vectorKind = "PODVector";
    else if (str::starts_with(typeName, "Vector<SharedPtr<") || str::starts_with(typeName, "Vector<WeakPtr<"))
        vectorKind = "Vector";
    else if (str::starts_with(typeName, "Vector<") && str::ends_with(typeName, " *>"))
        vectorKind = "Vector";
    else if (str::starts_with(typeName, "SharedPtr<") || str::starts_with(typeName, "WeakPtr<"))
    {
        // Wrap smart pointers
        spdlog::get("console")->info("Auto-typemap: {}", typeName);
        TypeMap map;
        map.cppType_ = typeName;
        if (str::starts_with(typeName, "SharedPtr<"))
        {
            // Get T from SharedPtr<T>
            map.csType_ = typeName.substr(10, typeName.length() - 11);
            map.cToCppTemplate_ = fmt::format("SharedPtr<{}>({{value}})", map.csType_);
            map.cppToCTemplate_ = "{value}.Detach()";
        }
        else
        {
            // Get T from WeakPtr<T>
            map.csType_ = typeName.substr(8, typeName.length() - 9);
            map.cToCppTemplate_ = fmt::format("WeakPtr<{}>({{value}})", map.csType_);
            map.cppToCTemplate_ = "{value}.Get()";
        }
        map.cType_ = map.csType_ + "*";
        str::replace_str(map.csType_, "::", ".");
        map.pInvokeToCSTemplate_ = fmt::format("{}.GetManagedInstance({{value}}, false)", map.csType_);
        map.csToPInvokeTemplate_ = fmt::format("{}.GetNativeInstance({{value}})", map.csType_);
        generator->typeMaps_[typeName] = map;
        return;
    }

    if (!vectorKind.empty())
    {
        // Wrap vectors
        const auto& tpl = dynamic_cast<const cppast::cpp_template_instantiation_type&>(realType);
        // cppast has no info for us. Make a best guess about the type in question.
        cppType = tpl.unexposed_arguments();
        auto primitiveType = PrimitiveToCppType(cppType);
        if (primitiveType == cppast::cpp_builtin_type_kind::cpp_void)
        {
            // Class pointer array
            if (str::starts_with(cppType, "SharedPtr<"))                    // starts with
            {
                // Get T from SharedPtr<T>
                csType = cppType.substr(10, cppType.length() - 11);
            }
            else if (str::starts_with(cppType, "WeakPtr<"))
            {
                // Get T from SharedPtr<T>
                csType = cppType.substr(8, cppType.length() - 9);
            }
            else if (str::ends_with(cppType, " *"))
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

        if (!csType.empty())
        {
            if (csType == "string")
            {
                spdlog::get("console")->info("TODO: Implement StrArrayMarshaller");
                // Size of string is not constant therefore they must be handled differently.
                throw std::exception();
            }

            spdlog::get("console")->info("Auto-typemap: {}", typeName);
            TypeMap map;
            map.cppType_ = typeName;

            map.csType_ = map.pInvokeType_ = fmt::format("{csType}[]", FMT_CAPTURE(csType));
            map.cType_ = "void*";
            map.cppToCTemplate_ = fmt::format("CSharpConverter<Urho3D::{vectorKind}<{cppType}>>::ToCSharp({{value}})",
                                              FMT_CAPTURE(cppType), FMT_CAPTURE(vectorKind));
            map.cToCppTemplate_ = fmt::format("CSharpConverter<Urho3D::{vectorKind}<{cppType}>>::FromCSharp({{value}})",
                                              FMT_CAPTURE(cppType), FMT_CAPTURE(vectorKind));
            if (IsBuiltinPInvokeType(csType) || (typeMap != nullptr && typeMap->isValueType_))
                map.customMarshaller_ = fmt::format("PodArrayMarshaller<{}>", csType);
            else
                map.customMarshaller_ = fmt::format("ObjArrayMarshaller<{}>", csType);
            map.isValueType_ = true;
            map.isArray_ = true;
            generator->typeMaps_[typeName] = map;
            return;
        }
    }

    // spdlog::get("console")->debug(">>>>>> {}", typeName);
}

}
