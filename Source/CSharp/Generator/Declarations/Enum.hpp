#pragma once


#include "Declaration.hpp"

namespace Urho3D
{

class Enum : public Declaration
{
public:
    explicit Enum(const cppast::cpp_entity* source)
        : Declaration(source)
    {
        kind_ = Kind::Enum;
    }

    String ToString() const override
    {
        return "enum " + name_;
    }
};

}
