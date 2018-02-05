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

#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Log.h>
#include <ThirdParty/cppast/include/cppast/cpp_member_variable.hpp>
#include <ThirdParty/cppast/include/cppast/cpp_member_function.hpp>
#include "GenerateClassWrappers.h"
#include "GeneratorContext.h"


namespace Urho3D
{

void GenerateClassWrappers::Start()
{
    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "#include \"CSharp.h\"";
    printer_ << "";
}

bool GenerateClassWrappers::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    if (e.kind() == cppast::cpp_entity_kind::class_t)
    {
        auto* data = GetUserData(e);
        if (data->hasWrapperClass)
        {
            if (info.event == info.container_entity_enter)
            {
                printer_ < "class URHO3D_EXPORT_API " < e.name() < "Ex : public " < GetSymbolName(e) < "\n";
                printer_.Indent();
                printer_.WriteLine("public:", false);
            }
            else if (info.event == info.container_entity_exit)
            {
                printer_.Dedent("};");
                printer_ << "";
            }
        }
        else if (info.event == info.container_entity_enter)
            return false;   // Skip children for classes that do not have virtual or protected members
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_variable_t && info.access == cppast::cpp_protected)
    {
        // Getters and setters for protected class variables.
        const auto& var = dynamic_cast<const cppast::cpp_member_variable&>(e);
        printer_ < cppast::to_string(var.type()) < " __get_" < e.name() < "() const { return " < e.name() < "; }\n";
        printer_ < "void __set_" < e.name() < "(" < cppast::to_string(var.type()) < " value) { " < e.name() < " = value; }\n";
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);
        if (info.access == cppast::cpp_protected)
        {

        }

        if (func.is_virtual())
        {
            // Function pointer that virtual method will call
            printer_ < cppast::to_string(func.return_type()) < "(" < func.parent().value().name() < "::*fn" < e.name() < ")(" < ParameterList(func.parameters()) < ") = nullptr;\n";

            // Virtual method that calls said pointer
            printer_ < "virtual " < cppast::to_string(func.return_type()) < " " < func.name() < "(" < ParameterList(func.parameters()) < ")";
            printer_.Indent();
            {
                if (!IsVoid(func.return_type()))
                    printer_ < "return ";
                printer_ < "(this->*fn" < e.name() < ")(" < ParameterNameList(func.parameters()) < ");";
            }
            printer_.Dedent();
        }
    }
    return true;
}

void GenerateClassWrappers::Stop()
{
    File file(context_, GetSubsystem<GeneratorContext>()->GetOutputDir() + "ClassWrappers.hpp", FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERROR("Failed saving ClassWrappers.hpp");
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

}
