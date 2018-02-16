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

    void Remove(Declaration* decl)
    {
        children_.Remove(SharedPtr<Declaration>(decl));
    }

    void Add(Declaration* decl)
    {
        SharedPtr<Declaration> ref(decl);
        if (!decl->parent_.Expired())
            decl->parent_->Remove(decl);
        decl->parent_ = this;
        children_.Push(ref);
        if (!decl->isStatic_)
            isStatic_ = false;
    }

    Vector<SharedPtr<Declaration>> children_;
};

}
