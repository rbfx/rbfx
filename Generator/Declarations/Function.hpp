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


#include "Declaration.hpp"
#include <cppast/cpp_entity_kind.hpp>


namespace Urho3D
{

class Function : public Declaration
{
public:
    explicit Function(const cppast::cpp_entity* source)
        : Declaration(source)
    {
        switch (source_->kind())
        {
        case cppast::cpp_entity_kind::function_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_function&>(*source);
            for (const auto& param : func.parameters())
                parameters_.emplace_back(&param);
            kind_ = Kind::Function;
            isStatic_ = true;
            break;
        }
        case cppast::cpp_entity_kind::member_function_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_member_function&>(*source);
            for (const auto& param : func.parameters())
                parameters_.emplace_back(&param);
            isStatic_ = false;
            kind_ = Kind::Method;
            isConstant_ = func.cv_qualifier() == cppast::cpp_cv_const ||
                func.cv_qualifier() == cppast::cpp_cv_const_volatile;
            break;
        }
        case cppast::cpp_entity_kind::constructor_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_constructor&>(*source);
            for (const auto& param : func.parameters())
                parameters_.emplace_back(&param);
            isStatic_ = false;
            kind_ = Kind::Constructor;
            returnType_ = reinterpret_cast<const cppast::cpp_type*>(&cppast::void_type_instance);
            break;
        }
        case cppast::cpp_entity_kind::destructor_t:
        {
            isStatic_ = false;
            kind_ = Kind::Destructor;
            returnType_ = reinterpret_cast<const cppast::cpp_type*>(&cppast::void_type_instance);
            break;
        }
        default:
            assert(false);
            break;
        }
    }

    String ToString() const override
    {
        return cppast::to_string(*returnType_) + " " + symbolName_;
    }

    bool IsVirtual() const
    {
        switch (source_->kind())
        {
        case cppast::cpp_entity_kind::member_function_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_member_function&>(*source_);
            return func.is_virtual();
        }
        case cppast::cpp_entity_kind::destructor_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_destructor&>(*source_);
            return func.is_virtual();
        }
        default:
            return false;
        }
    }

    /// Return type of function. Constructors and destructors have a void type.
    const cppast::cpp_type* returnType_ = nullptr;
    /// Parameters of a function.
    std::vector<const cppast::cpp_function_parameter*> parameters_;
};

}
