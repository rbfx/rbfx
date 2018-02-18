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

#include <Urho3D/IO/Log.h>
#include <cppast/cpp_member_function.hpp>
#include <Urho3D/IO/File.h>
#include <Printer/CodePrinter.h>
#include "GeneratorContext.h"
#include "BuildApi.h"


namespace Urho3D
{

void BuildApiPass::Start()
{
    generator_ = GetSubsystem<GeneratorContext>();
    symbolChecker_.Load(generator_->rules_->GetRoot().GetChild("symbols"));
    stack_.Push(generator_->apiRoot_.Get());
}

bool BuildApiPass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    if (e.kind() == cppast::cpp_entity_kind::file_t || e.kind() == cppast::cpp_entity_kind::language_linkage_t)
        return true;

    auto symbolName = GetSymbolName(e);
    if (!symbolName.StartsWith("anonymous_") /* we may need children of anonymous */ && !symbolChecker_.IsIncluded(symbolName))
        return info.event != cppast::visitor_info::container_entity_enter;

    // Skip children of private entities
    if (info.access == cppast::cpp_private)
        return info.event != cppast::visitor_info::container_entity_enter;

    if (e.kind() == cppast::cpp_entity_kind::class_t)
    {
        // Ignore forward-declarations
        if (!cppast::is_definition(e))
            return info.event != cppast::visitor_info::container_entity_enter;
    }

    // May be null.
    auto* parent = dynamic_cast<Namespace*>(stack_.Back());
    Declaration* declaration = nullptr;

    switch (e.kind())
    {
    case cppast::cpp_entity_kind::file_t:break;
    case cppast::cpp_entity_kind::macro_definition_t:break;
    case cppast::cpp_entity_kind::include_directive_t:break;
    case cppast::cpp_entity_kind::language_linkage_t:break;
    case cppast::cpp_entity_kind::namespace_t:
        declaration = GetDeclaration<Namespace>(e, info.access);
        break;
    case cppast::cpp_entity_kind::namespace_alias_t:break;
    case cppast::cpp_entity_kind::using_directive_t:break;
    case cppast::cpp_entity_kind::using_declaration_t:break;
    case cppast::cpp_entity_kind::type_alias_t:break;
    case cppast::cpp_entity_kind::enum_t:
        declaration = GetDeclaration<Enum>(e, info.access);
        break;
    case cppast::cpp_entity_kind::class_t:
        declaration = GetDeclaration<Class>(e, info.access);
        break;
    case cppast::cpp_entity_kind::access_specifier_t:break;
    case cppast::cpp_entity_kind::base_class_t:break;
    case cppast::cpp_entity_kind::variable_t:
    case cppast::cpp_entity_kind::enum_value_t:
    case cppast::cpp_entity_kind::member_variable_t:
        declaration = GetDeclaration<Variable>(e, info.access);
        break;
    case cppast::cpp_entity_kind::bitfield_t:break;
    case cppast::cpp_entity_kind::function_parameter_t:break;
    case cppast::cpp_entity_kind::function_t:
        declaration = GetDeclaration<Function>(e, info.access);
        break;
    case cppast::cpp_entity_kind::member_function_t:
        declaration = GetDeclaration<Function>(e, info.access);
        break;
    case cppast::cpp_entity_kind::conversion_op_t:
        break;
    case cppast::cpp_entity_kind::constructor_t:
    {
        const auto& ctor = dynamic_cast<const cppast::cpp_constructor&>(e);
        if (ctor.body_kind() == cppast::cpp_function_body_kind::cpp_function_deleted)
            return info.event != cppast::visitor_info::container_entity_enter;

        declaration = GetDeclaration<Function>(e, info.access);
        break;
    }
    case cppast::cpp_entity_kind::destructor_t:
    {
        const auto& dtor = dynamic_cast<const cppast::cpp_destructor&>(e);
        if (dtor.body_kind() == cppast::cpp_function_body_kind::cpp_function_deleted)
            return info.event != cppast::visitor_info::container_entity_enter;

        declaration = GetDeclaration<Function>(e, info.access);
        break;
    }
    case cppast::cpp_entity_kind::friend_t:break;
    case cppast::cpp_entity_kind::template_type_parameter_t:break;
    case cppast::cpp_entity_kind::non_type_template_parameter_t:break;
    case cppast::cpp_entity_kind::template_template_parameter_t:break;
    case cppast::cpp_entity_kind::alias_template_t:break;
    case cppast::cpp_entity_kind::variable_template_t:break;
    case cppast::cpp_entity_kind::function_template_t:break;
    case cppast::cpp_entity_kind::function_template_specialization_t:break;
    case cppast::cpp_entity_kind::class_template_t:break;
    case cppast::cpp_entity_kind::class_template_specialization_t:break;
    case cppast::cpp_entity_kind::static_assert_t:break;
    case cppast::cpp_entity_kind::unexposed_t:break;
    case cppast::cpp_entity_kind::count:break;
    }

    if (declaration != nullptr)
    {
        if (info.event == cppast::visitor_info::container_entity_exit)
        {
            stack_.Pop();
            return true;
        }
        else
        {
            if (parent != nullptr && declaration->parent_ == nullptr)
                parent->Add(declaration);

            if (info.event == cppast::visitor_info::container_entity_enter)
                stack_.Push(declaration);
        }
    }

    return true;
}

template<typename T>
T* BuildApiPass::GetDeclaration(const cppast::cpp_entity& e, const cppast::cpp_access_specifier_kind access)
{
    String name = GetSymbolName(e);
    auto* result = generator_->symbols_.Get(name);
    if (result != nullptr)
        assert(dynamic_cast<T*>(result) != nullptr);
    else
    {
        result = new T(&e);
        generator_->symbols_.Add(name, result);
    }
    result->isPublic_ = access == cppast::cpp_public;
    return static_cast<T*>(result);
}

}
