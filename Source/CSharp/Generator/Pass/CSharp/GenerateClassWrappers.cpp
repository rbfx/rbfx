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
#include "CppTypeInfo.h"
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
        if (!data->hasWrapperClass)
        {
            // Skip children for classes that do not have virtual or protected members
            if (info.event == info.container_entity_enter)
                return false;
        }
        else
        {
            if (info.event == info.container_entity_enter)
            {
                printer_ << fmt("class URHO3D_EXPORT_API {{name}}Ex : public {{symbol}}", {
                    {"name", e.name()},
                    {"symbol", GetSymbolName(e).CString()},
                });
                printer_.Indent();
                printer_.WriteLine("public:", false);

                Vector<const cppast::cpp_member_function*> wrappedList;
                auto implementWrapperClassMembers = [&](const cppast::cpp_class& cls)
                {
                    for (auto it = cls.begin(); it != cls.end(); it++)
                    {
                        const auto& e = *it;
                        // Manually iterating subtree may encounter ignored nodes.
                        if (!GetUserData(e)->generated)
                            continue;

                        auto access = GetUserData(e)->access;

                        if (e.kind() == cppast::cpp_entity_kind::member_variable_t && access == cppast::cpp_protected)
                        {
                            // Getters and setters for protected class variables.
                            const auto& var = dynamic_cast<const cppast::cpp_member_variable&>(e);
                            auto vars = fmt({
                                {"name", e.name()},
                                {"type", cppast::to_string(var.type())}
                            });
                            printer_ << fmt("{{type}} __get_{{name}}() const { return {{name}}; }", vars);
                            printer_ << fmt("void __set_{{name}}({{type}} value) { {{name}} = value; }", vars);
                        }
                        else if (e.kind() == cppast::cpp_entity_kind::member_function_t)
                        {
                            const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);

                            bool skip = false;
                            for (const auto* wrapped : wrappedList)
                            {
                                skip = func.name() == wrapped->name() && func.signature() == wrapped->signature();
                                if (skip)
                                    // Method was already wrapped as overload of downstream class.
                                    break;
                            }

                            if (!skip)
                            {
                                // Function pointer that virtual method will call
                                auto vars = fmt({
                                    {"type",                cppast::to_string(func.return_type())},
                                    {"name",                e.name()},
                                    {"class_name",          func.parent().value().name()},
                                    {"parameter_list",      ParameterList(func.parameters()).CString()},
                                    {"parameter_name_list", ParameterNameList(func.parameters()).CString()},
                                    {"return",              IsVoid(func.return_type()) ? "" : "return"},
                                    {"const",               func.cv_qualifier() == cppast::cpp_cv_const ? "const " : ""},
                                    {"has_params",          !func.parameters().empty()},
                                });
                                if (func.is_virtual())
                                {
                                    printer_ << fmt("{{type}}(*fn{{name}})({{class_name}} {{const}}*{{#has_params}}, {{/has_params}}{{parameter_list}}) = nullptr;", vars);
                                    // Virtual method that calls said pointer
                                    printer_ << fmt("{{type}} {{name}}({{parameter_list}}) {{const}}override { {{return}}(fn{{name}})(this{{#has_params}}, {{/has_params}}{{parameter_name_list}}); }", vars);
                                }
                                if (access == cppast::cpp_protected)
                                {
                                    printer_ << fmt("{{type}} __public_{{name}}({{parameter_list}})", vars);
                                    printer_.Indent();
                                    printer_ << fmt("{{name}}({{parameter_name_list}});", vars);
                                    printer_.Dedent();
                                }
                                wrappedList.Push(&func);
                            }
                        }
                    }
                };

                auto generator = GetSubsystem<GeneratorContext>();
                std::function<void(const cppast::cpp_class&)> implementBaseWrapperClassMembers = [&](const cppast::cpp_class& cls)
                {
                    for (const auto& base : cls.bases())
                    {
                        if (base.access_specifier() == cppast::cpp_private)
                            continue;

                        const auto* parentCls = dynamic_cast<const cppast::cpp_class*>(generator->GetKnownType(CppTypeInfo(base.type()).name_));
                        if (parentCls != nullptr)
                        {
                            implementBaseWrapperClassMembers(*parentCls);
                            implementWrapperClassMembers(*parentCls);
                        }
                        else
                            URHO3D_LOGWARNINGF("Base class %s not found!", base.name().c_str());
                    }
                };

                const auto& cls = dynamic_cast<const cppast::cpp_class&>(e);
                implementBaseWrapperClassMembers(cls);
                implementWrapperClassMembers(cls);

                printer_.Dedent("};");
                printer_ << "";
            }
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
