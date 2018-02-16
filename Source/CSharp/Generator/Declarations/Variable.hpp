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


#include <cppast/cpp_variable.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_member_variable.hpp>
#include "Declaration.hpp"

namespace Urho3D
{

class Variable : public Declaration
{
public:
    explicit Variable(const cppast::cpp_entity* source)
        : Declaration(source)
    {
        kind_ = Kind::Variable;

        if (source != nullptr)
        {
            // Global scope
            isStatic_ = source->kind() != cppast::cpp_entity_kind::member_variable_t;

            if (GetType().kind() == cppast::cpp_type_kind::cv_qualified_t)
            {
                const auto& cvVar = dynamic_cast<const cppast::cpp_cv_qualified_type&>(GetType());
                if (cppast::is_const(cvVar.cv_qualifier()))
                    isConstant_ = true;
            }

            if (isConstant_)
            {
                const cppast::cpp_expression* defaultValue = nullptr;
                if (source_->kind() == cppast::cpp_entity_kind::variable_t)
                {
                    const auto& var = dynamic_cast<const cppast::cpp_variable&>(*source_);
                    if (var.default_value().has_value())
                        defaultValue = &var.default_value().value();
                }
                else if (source_->kind() == cppast::cpp_entity_kind::member_variable_t)
                {
                    const auto& var = dynamic_cast<const cppast::cpp_member_variable&>(*source_);
                    if (var.default_value().has_value())
                        defaultValue = &var.default_value().value();
                }

                if (defaultValue != nullptr)
                {
                    if (defaultValue->kind() == cppast::cpp_expression_kind::literal_t)
                    {
                        defaultValue_ = dynamic_cast<const cppast::cpp_literal_expression*>(defaultValue)->value();
                        isLitteral_ = true;
                    }
                    else
                        defaultValue_ = dynamic_cast<const cppast::cpp_unexposed_expression*>(defaultValue)->expression().as_string();
                }
            }
        }
    }

    String ToString() const override
    {
        String result = String() + cppast::to_string(GetType()) + " " + name_;
        if (!defaultValue_.Empty())
            result += " = " + defaultValue_;
        return result;
    }

    const cppast::cpp_type& GetType() const
    {
        if (source_->kind() == cppast::cpp_entity_kind::variable_t)
            return dynamic_cast<const cppast::cpp_variable&>(*source_).type();
        else if (source_->kind() == cppast::cpp_entity_kind::member_variable_t)
            return dynamic_cast<const cppast::cpp_member_variable&>(*source_).type();
        else
            assert(false);
    }

    bool isLitteral_ = false;
    String defaultValue_ = "";
};

}
