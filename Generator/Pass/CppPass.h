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


#include <Urho3D/Core/Object.h>
#include <cppast/cpp_entity.hpp>
#include <cppast/visitor.hpp>
#include "Declarations/Declaration.hpp"


namespace Urho3D
{

class CppAstPass : public Object
{
    URHO3D_OBJECT(CppAstPass, Object)
public:
    explicit CppAstPass(Context* context) : Object(context) { };

    virtual void Start() { }
    virtual void StartFile(const String& filePath) { }
    virtual bool Visit(const cppast::cpp_entity& e, cppast::visitor_info info) = 0;
    virtual void StopFile(const String& filePath) { }
    virtual void Stop() { }
};

class CppApiPass : public Object
{
URHO3D_OBJECT(CppApiPass, Object)
public:
    explicit CppApiPass(Context* context) : Object(context) { };

    enum Event
    {
        /// Set when visiting namespace before visiting it's children.
        ENTER,
        /// Set when visiting namespace after visiting it's children.
        EXIT,
        /// Set when visiting non-namespace declaration.
        LEAF
    };

    virtual void Start() { }
    virtual bool Visit(Declaration* decl, Event event) = 0;
    virtual void Stop() { }
};


}
