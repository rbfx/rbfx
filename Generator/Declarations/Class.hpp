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
                    if (func->kind_ != Declaration::Kind::Destructor && func->IsVirtual())
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
