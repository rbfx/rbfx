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
#include <cppast/cpp_member_function.hpp>
#include "GatherInfoPass.h"
#include "GeneratorContext.h"


namespace Urho3D
{

void GatherInfoPass::Start()
{
    URHO3D_LOGDEBUGF("~~~~~ GatherInfo ~~~~~");
    typeChecker_.Load(GetSubsystem<GeneratorContext>()->GetRules()->GetRoot().GetChild("types"));
}

void GatherInfoPass::StartFile(const String& filePath)
{
    access_ = cppast::cpp_private;
}

bool GatherInfoPass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    if (e.kind() == cppast::cpp_entity_kind::access_specifier_t)
    {
        access_ = dynamic_cast<const cppast::cpp_access_specifier&>(e).access_specifier();
        return true;
    }

    // Default visibility for structs is public
    if (e.kind() == cppast::cpp_entity_kind::class_t && info.event == info.container_entity_enter)
    {
        const auto& cls = dynamic_cast<const cppast::cpp_class&>(e);
        if (cls.class_kind() == cppast::cpp_class_kind::class_t)
            access_ = cppast::cpp_private;
        else
            access_ = cppast::cpp_public;
    }

    if (e.kind() == cppast::cpp_entity_kind::function_t || e.kind() == cppast::cpp_entity_kind::member_function_t || e.kind() == cppast::cpp_entity_kind::member_variable_t)
    {
        if (e.parent().value().kind() == cppast::cpp_entity_kind::class_t)
        {
            if (access_ == cppast::cpp_private)
            {
                GetUserData(e)->generated = false;
                if (info.event == cppast::visitor_info::container_entity_enter)
                    return false;
            }
            else if (access_ == cppast::cpp_protected)
                GetUserData(e.parent().value())->hasProtected = true;
        }
    }
    else if (e.kind() == cppast::cpp_entity_kind::class_t && cppast::is_definition(e) && !cppast::is_templated(e))
    {
        // TODO: typedefs, templates
        auto name = GetSymbolName(e);
        if (typeChecker_.IsIncluded(name))
            GetSubsystem<GeneratorContext>()->RegisterKnownType(name, e);
    }

    if (e.kind() == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);
        if (func.is_variadic())
            GetUserData(e)->generated = false;
        else if (func.is_virtual())
            GetUserData(e.parent().value())->hasVirtual = true;
    }

    return true;
}

}
