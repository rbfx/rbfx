#pragma once


#include "Declaration.hpp"
#include <cppast/cpp_entity_kind.hpp>

namespace cppast
{

static auto void_type_instance = cppast::cpp_builtin_type::build(cppast::cpp_void);

}

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
            kind_ = Kind::Function;
            isStatic_ = true;
            break;
        }
        case cppast::cpp_entity_kind::member_function_t:
        {
            kind_ = Kind::Method;
            const auto& func = dynamic_cast<const cppast::cpp_member_function&>(*source);
            isConstant_ = func.cv_qualifier() == cppast::cpp_cv_const ||
                func.cv_qualifier() == cppast::cpp_cv_const_volatile;
            break;
        }
        case cppast::cpp_entity_kind::constructor_t:
        {
            kind_ = Kind::Constructor;
            break;
        }
        case cppast::cpp_entity_kind::destructor_t:
        {
            kind_ = Kind::Destructor;
            break;
        }
        default:
            assert(false);
            break;
        }
    }

    String ToString() const override
    {
        if (source_ == nullptr)
            return name_;

        switch (source_->kind())
        {
        case cppast::cpp_entity_kind::function_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_function&>(*source_);
            return String() + cppast::to_string(func.return_type()) + " " + name_ + func.signature();
        }
        case cppast::cpp_entity_kind::member_function_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_member_function&>(*source_);
            return String() + cppast::to_string(func.return_type()) + " " + name_ + func.signature();
        }
        case cppast::cpp_entity_kind::constructor_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_constructor&>(*source_);
            return name_ + func.signature();
        }
        case cppast::cpp_entity_kind::destructor_t:
        {
            const auto& func = dynamic_cast<const cppast::cpp_destructor&>(*source_);
            return name_ + func.signature();
        }
        default:
            assert(false);
            break;
        }
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

    cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter> GetParameters() const
    {
        assert(source_ != nullptr);

        switch (source_->kind())
        {
        case cppast::cpp_entity_kind::function_t:
            return dynamic_cast<const cppast::cpp_function&>(*source_).parameters();
        case cppast::cpp_entity_kind::member_function_t:
            return dynamic_cast<const cppast::cpp_member_function&>(*source_).parameters();
        case cppast::cpp_entity_kind::constructor_t:
            return dynamic_cast<const cppast::cpp_constructor&>(*source_).parameters();
        case cppast::cpp_entity_kind::destructor_t:
            return dynamic_cast<const cppast::cpp_destructor&>(*source_).parameters();
        default:
            assert(false);
            break;
        }
    }

    const cppast::cpp_type& GetReturnType() const
    {
        assert(source_ != nullptr);

        switch (source_->kind())
        {
        case cppast::cpp_entity_kind::function_t:
            return dynamic_cast<const cppast::cpp_function&>(*source_).return_type();
        case cppast::cpp_entity_kind::member_function_t:
            return dynamic_cast<const cppast::cpp_member_function&>(*source_).return_type();
        case cppast::cpp_entity_kind::constructor_t:
            return *cppast::void_type_instance.get();
        case cppast::cpp_entity_kind::destructor_t:
            return *cppast::void_type_instance.get();
        default:
            assert(false);
            break;
        }
    }

    bool isStatic_ = false;
    bool isConstant_ = false;
    bool isProperty_ = false;
};

}
