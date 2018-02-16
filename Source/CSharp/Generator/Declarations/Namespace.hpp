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
