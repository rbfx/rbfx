#pragma once


#include "Namespace.hpp"
#include "Function.hpp"


namespace Urho3D
{

class Class : public Namespace
{
public:
    explicit Class(const cppast::cpp_entity* source)
        : Namespace(source)
    {
        kind_ = Kind::Class;
    }

    String ToString() const override
    {
        return "class " + name_;
    }

    bool HasProtected() const
    {
        for (const auto& child : children_)
        {
            if (!child->isPublic_)
                return true;
        }
        return false;
    }

    bool HasVirtual() const
    {
        for (const auto& child : children_)
        {
            if (child->IsFunctionLike())
            {
                if (const auto* func = dynamic_cast<const Function*>(child.Get()))
                {
                    if (func->IsVirtual())
                        return true;
                }
            }
        }
        return false;
    }

    bool IsSubclassOf(String symbolName)
    {
        symbolName.Replaced(".", "::");
        if (symbolName == symbolName_)
            return true;

        for (const auto& base : bases_)
        {
            if (!base.Expired() && base->IsSubclassOf(symbolName))
                return true;
        }

        return false;
    }

    Vector<WeakPtr<Class>> bases_;
    bool isInterface_ = false;
};

}
