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


#include <Urho3D/Container/RefCounted.h>
#include <Urho3D/Core/Utils.h>
#include <cppast/cpp_entity.hpp>
#include <cassert>
#include "Utilities.h"


namespace cppast
{

static auto void_type_instance = cppast::cpp_builtin_type::build(cppast::cpp_void);
static auto int_type_instance = cppast::cpp_builtin_type::build(cppast::cpp_int);

}

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
            name_ = source->name();
            symbolName_ = sourceName_ = GetSymbolName(*source);
            baseSymbolName_ = GetBaseSymbolName(*source);
            if (name_.Empty())
            {
                name_ = symbolName_;
                sourceName_ = "";
            }
        }
    }

    /// Return string representation of this declaration. String does not include children of declaration (if any).
    virtual String ToString() const
    {
        return name_;
    }

    /// Removes declaration from API.
    void Ignore();

    /// Returns true if object is of `Function` type.
    bool IsFunctionLike() const
    {
        return kind_ == Kind::Function || kind_ == Kind::Method || kind_ == Kind::Constructor ||
            kind_ == Kind::Destructor;
    }

    /// Returns true if object is of or inherits `Namespace` type.
    bool IsNamespaceLike() const
    {
        return kind_ == Declaration::Kind::Namespace || kind_ == Declaration::Kind::Class || kind_ == Declaration::Kind::Enum;
    }

    /// Name of this declaration in a produced wrapper.
    String name_;
    /// Name of declaration that is to be wrapped.
    String sourceName_;
    /// Unique name pointing to entity in c++ ast. It includes function signature.
    String symbolName_;
    /// Not necessarily unique name pointing to entity. Same as symbolName_ without function signature.
    String baseSymbolName_;
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
    /// When set to true indicates that declaration does not belong to an instance of a object.
    bool isStatic_ = true;
    /// When set to true indicates that value of declaration never changes.
    bool isConstant_ = false;
    /// When set to true it hints generator to generate wrapper interface as properties where applicable.
    bool isProperty_ = false;
};

}
