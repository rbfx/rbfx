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

#pragma once


#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_variable.hpp>
#include <cppast/visitor.hpp>
#include "GeneratorContext.h"
#include "CppPass.h"
#include "Utilities.h"


namespace Urho3D
{

/// Walk AST and gather known defined classes. Exclude protected/private members from generation.
class UnknownTypesPass : public CppApiPass
{
public:
    explicit UnknownTypesPass() { };

    bool Visit(MetaEntity* entity, cppast::visitor_info info) override
    {
        if (entity->ast_ == nullptr || info.event == info.container_entity_exit)
            return true;

        using ParameterList = cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>;

        auto checkFunctionParams = [&](const ParameterList& params) {
            for (const auto& param : params)
            {
                if (!generator->IsAcceptableType(param.type()))
                {
                    spdlog::get("console")->info("Ignore: {}, unknown parameter type {}", entity->uniqueName_,
                        cppast::to_string(param.type()));
                    return false;
                }
            }
            return true;
        };

        auto checkFunctionTypes = [&](const cppast::cpp_type& returnType,
            const ParameterList& params) {
            if (!generator->IsAcceptableType(returnType))
            {
                spdlog::get("console")->info("Ignore: {}, unknown return type {}", entity->uniqueName_,
                    cppast::to_string(returnType));
                return false;
            }
            return checkFunctionParams(params);
        };

        switch (entity->ast_->kind())
        {
        case cppast::cpp_entity_kind::file_t:break;
        case cppast::cpp_entity_kind::macro_definition_t:break;
        case cppast::cpp_entity_kind::include_directive_t:break;
        case cppast::cpp_entity_kind::language_linkage_t:break;
        case cppast::cpp_entity_kind::namespace_t:break;
        case cppast::cpp_entity_kind::namespace_alias_t:break;
        case cppast::cpp_entity_kind::using_directive_t:break;
        case cppast::cpp_entity_kind::using_declaration_t:break;
        case cppast::cpp_entity_kind::type_alias_t:break;
        case cppast::cpp_entity_kind::enum_t:break;
        case cppast::cpp_entity_kind::enum_value_t:break;
        case cppast::cpp_entity_kind::class_t:
            if (entity->name_.empty())
                entity->Remove();
            break;
        case cppast::cpp_entity_kind::access_specifier_t:break;
        case cppast::cpp_entity_kind::base_class_t:break;
        case cppast::cpp_entity_kind::variable_t:
        {
            const auto& e = entity->Ast<cppast::cpp_variable>();
            if (!generator->IsAcceptableType(e.type()))
            {
                spdlog::get("console")->info("Ignore: {}, type {}", entity->uniqueName_, cppast::to_string(e.type()));
                entity->Remove();
            }
            break;
        }
        case cppast::cpp_entity_kind::member_variable_t:
        {
            const auto& e = entity->Ast<cppast::cpp_member_variable>();
            if (!generator->IsAcceptableType(e.type()))
            {
                spdlog::get("console")->info("Ignore: {}, type {}", entity->uniqueName_, cppast::to_string(e.type()));
                entity->Remove();
            }
            break;
        }
        case cppast::cpp_entity_kind::bitfield_t:break;
        case cppast::cpp_entity_kind::function_parameter_t:break;
        case cppast::cpp_entity_kind::function_t:
        {
            const auto& e = entity->Ast<cppast::cpp_function>();
            if (!checkFunctionTypes(e.return_type(), e.parameters()))
                entity->Remove();
            if (e.name().find("operator") == 0)
                entity->Remove();
            break;
        }
        case cppast::cpp_entity_kind::member_function_t:
        {
            const auto& e = entity->Ast<cppast::cpp_member_function>();
            if (!checkFunctionTypes(e.return_type(), e.parameters()))
                entity->Remove();
            if (e.name().find("operator") == 0)
                entity->Remove();
            break;
        }
        case cppast::cpp_entity_kind::conversion_op_t:break;
        case cppast::cpp_entity_kind::constructor_t:
        {
            const auto& e = entity->Ast<cppast::cpp_constructor>();
            if (!checkFunctionParams(e.parameters()))
                entity->Remove();
            break;
        }
        case cppast::cpp_entity_kind::destructor_t:break;
        case cppast::cpp_entity_kind::friend_t:break;
        case cppast::cpp_entity_kind::template_type_parameter_t:
        case cppast::cpp_entity_kind::non_type_template_parameter_t:
        case cppast::cpp_entity_kind::template_template_parameter_t:
        case cppast::cpp_entity_kind::alias_template_t:
        case cppast::cpp_entity_kind::variable_template_t:
        case cppast::cpp_entity_kind::function_template_t:
        case cppast::cpp_entity_kind::function_template_specialization_t:
        case cppast::cpp_entity_kind::class_template_t:
        case cppast::cpp_entity_kind::class_template_specialization_t:
        case cppast::cpp_entity_kind::static_assert_t:
            entity->Remove();
            break;
        case cppast::cpp_entity_kind::unexposed_t:break;
        case cppast::cpp_entity_kind::count:break;
        }

        return true;
    }
};

}
