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
#include <Urho3D/IO/File.h>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>
#include "GenerateCApiPass.h"
#include "GeneratorContext.h"
#include "CppTypeInfo.h"


namespace Urho3D
{

void GenerateCApiPass::Start()
{
    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "#include \"CSharp.h\"";
    printer_ << "#include \"ClassWrappers.hpp\"";
    printer_ << "";
    printer_ << "using namespace Urho3D;";
    printer_ << "";
    printer_ << "extern \"C\"";
    printer_ << "{";
    printer_ << "";
}

bool GenerateCApiPass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    auto generator = GetSubsystem<GeneratorContext>();

    // Visit entities just once
    if (info.event == cppast::visitor_info::container_entity_exit)
        return true;

    auto toCType = [&](const cppast::cpp_type& type) { return generator->GetTypeMap(type).cType; };
    auto toCppType = [](const cppast::cpp_function_parameter& param) {
        return fmt("CSharpTypeConverter<{{type}}>::toCpp({{name}})", {
            {"type", GetConversionType(param.type()).CString()},
            {"name", param.name()}
        });
    };
    auto toCTypeReturn = [&](const cppast::cpp_type& returnType, const String& callExpr) {
        CppTypeInfo typeInfo(returnType);
        String expr = fmt("return CSharpTypeConverter<{{type}}>::toC({{call}}", {
            {"type", GetConversionType(returnType).CString()},
            {"call", callExpr.CString()}
        });
        // Value types returned by copy
        if (!typeInfo.pointer_)
            expr += ", true";
        expr += ");";
        return expr;
    };

    if (e.kind() == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_function&>(e);
        String symbolName = GetSymbolName(e);
        UserData* data = GetUserData(e);
        data->cFunctionName = GetUniqueName(Sanitize(symbolName));

        auto vars = fmt({
            {"c_return_type", toCType(func.return_type()).CString()},
            {"c_function_name", data->cFunctionName.CString()},
            {"c_parameter_list",    ParameterList(func.parameters(), toCType).CString()},
            {"parameter_name_list", ParameterNameList(func.parameters(), toCppType).CString()},
            {"symbol_name", symbolName.CString()},
        });

        // c wrapper function declaration
        printer_ << fmt("URHO3D_EXPORT_API {{c_return_type}} {{c_function_name}}({{c_parameter_list}})", vars);

        // function body that calls actual api
        printer_.Indent();
        {
            String call = fmt("{{symbol_name}}({{parameter_list}})", vars);

            if (IsVoid(func.return_type()))
                printer_ << call + ";";
            else
                printer_ << toCTypeReturn(func.return_type(), call);
        }
        printer_.Dedent();
        printer_.WriteLine();
    }
    else if (e.kind() == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_constructor&>(e);

        String symbolName = GetSymbolName(e);
        UserData* data = GetUserData(e);
        data->cFunctionName = GetUniqueName(Sanitize(symbolName));

        const auto& parent = dynamic_cast<const cppast::cpp_class&>(e.parent().value());
        String className = parent.name().c_str();
        if (GetUserData(parent)->hasWrapperClass)
            className += "Ex";

        auto vars = fmt({
            {"c_function_name",     data->cFunctionName.CString()},
            {"class_name",          className.CString()},
            {"c_parameter_list",    ParameterList(func.parameters(), toCType).CString()},
            {"parameter_name_list", ParameterNameList(func.parameters(), toCppType).CString()},
        });

        printer_ << fmt("URHO3D_EXPORT_API {{class_name}}* construct_{{c_function_name}}({{c_parameter_list}})", vars);
        printer_.Indent();
        {
            printer_ << fmt("return new {{class_name}}({{parameter_name_list}});", vars);
        }
        printer_.Dedent();
    }
    else if (e.kind() == cppast::cpp_entity_kind::destructor_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_destructor&>(e);

        String symbolName = GetSymbolName(e);
        UserData* data = GetUserData(e);
        data->cFunctionName = GetUniqueName(Sanitize(symbolName));

        const auto& parent = dynamic_cast<const cppast::cpp_class&>(e.parent().value());
        String className = parent.name().c_str();
        if (GetUserData(parent)->hasWrapperClass)
            className += "Ex";

        printer_ << fmt("URHO3D_EXPORT_API void destruct_{{c_function_name}}({{class_name}}* cls)", {
            {"c_function_name",     data->cFunctionName.CString()},
            {"class_name",          className.CString()},
        });
        printer_.Indent();
        {
            printer_ << "delete cls;";
        }
        printer_.Dedent();
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);

        String symbolName = GetSymbolName(e);
        UserData* data = GetUserData(e);
        data->cFunctionName = GetUniqueName(Sanitize(symbolName));

        const auto& parent = dynamic_cast<const cppast::cpp_class&>(e.parent().value());
        String className = parent.name().c_str();
        if (GetUserData(parent)->hasWrapperClass)
            className += "Ex";

        auto parameterNameList = ParameterNameList(func.parameters(), toCppType);
        auto cReturnType = generator->GetTypeMap(func.return_type()).cType;
        auto cParameterList = ParameterList(func.parameters(), toCType);

        // Method c wrappers always have first parameter implicitly.
        if (!cParameterList.Empty())
            cParameterList = ", " + cParameterList;

        auto vars = fmt({
            {"c_return_type",       cReturnType.CString()},
            {"c_function_name",     data->cFunctionName.CString()},
            {"class_name",          className.CString()},
            {"c_parameter_list",    cParameterList.CString()},
            {"name",                func.name()},
            {"parameter_name_list", parameterNameList.CString()},
        });

        printer_ << fmt("URHO3D_EXPORT_API {{c_return_type}} {{c_function_name}}({{class_name}}* cls{{c_parameter_list}})", vars);
        printer_.Indent();
        {
            String call = fmt("cls->{{name}}({{parameter_name_list}})", vars);
            if (IsVoid(func.return_type()))
                printer_ << call + ";";
            else
                printer_ << toCTypeReturn(func.return_type(), call);
        }
        printer_.Dedent();
        printer_.WriteLine();

        if (func.is_virtual())
        {
            printer_ << fmt("URHO3D_EXPORT_API void set_{{class_name}}_fn{{name}}({{class_name}}* cls, void* fn)",
                vars);
            printer_.Indent();
            {
                printer_ << fmt("cls->fn{{name}} = (decltype(cls->fn{{name}}))fn;", vars);
            }
            printer_.Dedent();
            printer_.WriteLine();
        }
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = dynamic_cast<const cppast::cpp_member_variable&>(e);
        const auto& cls = dynamic_cast<const cppast::cpp_class&>(e.parent().value());
        UserData* data = GetUserData(e);

        auto className = cls.name();
        if (GetUserData(cls)->hasWrapperClass)
            className += "Ex";

        data->cFunctionName = Sanitize(GetSymbolName(var.parent().value()) + "_" + var.name().c_str());
        auto vars = fmt({{"c_type", generator->GetTypeMap(var.type()).cType.CString()},
                         {"c_function_name", data->cFunctionName.CString()},
                         {"class_name",      className},
                         {"name",            var.name()},
                         {"type",            GetConversionType(var.type()).CString()}
        });
        // Getter
        printer_ << fmt("URHO3D_EXPORT_API {{c_type}} get_{{c_function_name}}({{class_name}}* cls)", vars);
        printer_.Indent();
        if (info.access == cppast::cpp_protected)
            printer_ << fmt("return CSharpTypeConverter<{{type}}>::toC(cls->__get_{{name}}());", vars);
        else
            printer_ << fmt("return CSharpTypeConverter<{{type}}>::toC(cls->{{name}});", vars);

        printer_.Dedent();
        printer_.WriteLine();

        // Setter
        printer_ << fmt("URHO3D_EXPORT_API void set_{{c_function_name}}({{class_name}}* cls, {{c_type}} value)", vars);
        printer_.Indent();
        if (info.access == cppast::cpp_protected)
            printer_ << fmt("cls->__set_{{name}}(CSharpTypeConverter<{{type}}>::toC(value));", vars);
        else
            printer_ << fmt("cls->{{name}} = CSharpTypeConverter<{{type}}>::toC(value);", vars);
        printer_.Dedent();
        printer_.WriteLine();
    }

    return true;
}

void GenerateCApiPass::Stop()
{
    printer_ << "}";    // Close extern "C"

    File file(context_, GetSubsystem<GeneratorContext>()->GetOutputDir() + "CApi.cpp", FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERROR("Failed saving CApi.cpp");
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

String GenerateCApiPass::GetUniqueName(const String& baseName)
{
    unsigned index = 0;
    String newName;
    for (newName = Sanitize(baseName); usedNames_.Contains(newName); index++)
        newName = baseName + String(index);

    usedNames_.Push(newName);
    return newName;
}

}
