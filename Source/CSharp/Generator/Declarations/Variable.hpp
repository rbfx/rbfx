#pragma once


#include <cppast/cpp_variable.hpp>
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
            if (source->kind() == cppast::cpp_entity_kind::variable_t)
                isStatic_ = true;

            if (GetType().kind() == cppast::cpp_type_kind::cv_qualified_t)
            {
                const auto& cvVar = dynamic_cast<const cppast::cpp_cv_qualified_type&>(GetType());
                if (cppast::is_const(cvVar.cv_qualifier()))
                    isConstant_ = true;
            }
        }
    }

    String ToString() const override
    {
        return String() + cppast::to_string(GetType()) + " " + name_;
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


    bool isStatic_ = false;
    bool isConstant_ = false;
    bool isProperty_ = false;
};

}
