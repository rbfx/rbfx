#pragma once


#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/Core/Utils.h>
#include <cppast/cpp_entity.hpp>
#include <cassert>
#include "Utilities.h"


namespace Urho3D
{

class Namespace;


class Declaration : public RefCounted
{
public:
    enum class Kind : unsigned
    {
        Unknown,
        Namespace,
        Class,
        Enum,
        Variable,
        Function,
        Method,
        Constructor,
        Destructor,
        Operator,
    };

    explicit Declaration(const cppast::cpp_entity* source)
        : source_(source)
    {
        if (source != nullptr)
        {
            name_ = sourceName_ = source->name();
            symbolName_ = GetSymbolName(*source);
        }
    }

    /// Return string representation of this declaration. String does not include children of declaration (if any).
    virtual String ToString() const
    {
        return name_;
    }

    void Ignore() { isIgnored_ = true; }

    /// Returns true if object is of `Function` type.
    bool IsFunctionLike() const
    {
        return kind_ == Kind::Function || kind_ == Kind::Method || kind_ == Kind::Constructor ||
            kind_ == Kind::Destructor;
    }

    /// Returns true if object is of or inherits `Namespace` type.
    bool IsNamespaceLike() const
    {
        return kind_ == Declaration::Kind::Namespace || kind_ == Declaration::Kind::Class;
    }

    /// Name of this declaration in a produced wrapper.
    String name_;
    /// Name of declaration that is to be wrapped.
    String sourceName_;
    /// Unique name pointing to entity in c++ ast.
    String symbolName_;
    /// C API function (base) name of this declaration.
    String cFunctionName_;
    /// When `false` declaration is protected.
    bool isPublic_ = true;
    /// C++ ast entity from which declaration is being generated. Can be null.
    const cppast::cpp_entity* source_;
    /// Contextual information about declaration.
    Kind kind_ = Kind::Unknown;
    /// Pointer to owner of this declaration.
    WeakPtr<Namespace> parent_;
    /// When set to true passes will not iterate over this declaration.
    bool isIgnored_ = false;
};

}
