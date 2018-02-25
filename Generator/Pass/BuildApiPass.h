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
#include "CppPass.h"
#include "Utilities.h"

#include "Declarations/Declaration.hpp"
#include "Declarations/Namespace.hpp"
#include "Declarations/Class.hpp"
#include "Declarations/Enum.hpp"
#include "Declarations/Function.hpp"
#include "Declarations/Variable.hpp"

#include "Printer/CSharpPrinter.h"

namespace Urho3D
{

/// Walk AST and build API tree which later can be altered and used for generating a wrapper.
class BuildApiPass : public CppAstPass
{
    URHO3D_OBJECT(BuildApiPass, CppAstPass);
public:
    explicit BuildApiPass(Context* context) : CppAstPass(context) { };

    void Start() override;
    bool Visit(const cppast::cpp_entity& e, cppast::visitor_info info) override;

protected:
    template<typename T>
    T* GetDeclaration(const cppast::cpp_entity& e, const cppast::cpp_access_specifier_kind access);

    IncludedChecker symbolChecker_;
    Vector<Declaration*> stack_;
    GeneratorContext* generator_ = nullptr;
};

}
