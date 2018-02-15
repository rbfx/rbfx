#pragma once


#include "Declaration.hpp"

namespace Urho3D
{

class Namespace : public Declaration
{
public:
    explicit Namespace(const cppast::cpp_entity* source)
        : Declaration(source)
    {
        kind_ = Kind::Namespace;
    }

    String ToString() const override
    {
        return "namespace " + name_;
    }

    Vector<SharedPtr<Declaration>> children_;
};

}
