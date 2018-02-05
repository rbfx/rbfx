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

#include <Urho3D/IO/Log.h>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_member_function.hpp>
#include "UnknownTypesPass.h"
#include "GeneratorContext.h"


namespace Urho3D
{

bool UnknownTypesPass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    auto generator = GetSubsystem<GeneratorContext>();
    auto checkFunctionTypes = [&](const cppast::cpp_type& returnType,
        const cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>& params) {
        if (!generator->IsKnownType(returnType))
        {
            URHO3D_LOGINFOF("Ignore: %s, unknown return type %s", GetSymbolName(e).CString(),
                cppast::to_string(returnType).c_str());
            return false;
        }
        for (const auto& param : params)
        {
            if (!generator->IsKnownType(param.type()))
            {
                URHO3D_LOGINFOF("Ignore: %s, unknown parameter type %s", GetSymbolName(e).CString(),
                    cppast::to_string(param.type()).c_str());
                return false;
            }
        }
        return true;
    };

    if (e.kind() == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_function&>(e);
        if (!checkFunctionTypes(func.return_type(), func.parameters()))
            GetUserData(e)->generated = false;
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);
        if (!checkFunctionTypes(func.return_type(), func.parameters()))
            GetUserData(e)->generated = false;
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_variable_t || e.kind() == cppast::cpp_entity_kind::variable_t)
    {
        const auto& var = dynamic_cast<const cppast::cpp_variable_base&>(e);
        if (!generator->IsKnownType(var.type()))
        {
            GetUserData(e)->generated = false;
            URHO3D_LOGINFOF("Ignore: %s, unknown return type %s", GetSymbolName(e).CString(),
                cppast::to_string(var.type()).c_str());
        }
    }
    else if (e.kind() == cppast::cpp_entity_kind::class_t)
    {
        if (!cppast::is_definition(e))
            GetUserData(e)->generated = false;  // Forward decl
        else
            GetUserData(e)->generated = generator->IsKnownType(GetSymbolName(e));
    }
    return true;
}
}
