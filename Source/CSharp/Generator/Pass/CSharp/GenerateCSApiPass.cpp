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
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <cppast/cpp_member_variable.hpp>
#include "GeneratorContext.h"
#include "GenerateCSApiPass.h"

namespace Urho3D
{

void GenerateCSApiPass::Start()
{
    generator_ = GetSubsystem<GeneratorContext>();
    typeMapper_ = &generator_->GetTypeMapper();

    printer_ << "using System;";
    printer_ << "";
    printer_ << "namespace Urho3D";
    printer_ << "{";
    printer_ << "";
}

bool GenerateCSApiPass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    auto* generator = GetSubsystem<GeneratorContext>();
    auto mapToPInvoke = [&](const cppast::cpp_function_parameter& param) {
        return typeMapper_->MapToPInvoke(param.type(), EnsureNotKeyword(param.name()));
    };
    if (e.kind() == cppast::cpp_entity_kind::class_t)
    {
        const auto& cls = dynamic_cast<const cppast::cpp_class&>(e);
        if (info.event == info.container_entity_enter)
        {
            Vector<String> bases;
            for (const auto& base : cls.bases())
                bases.Push(base.name().c_str());

            auto vars = fmt({
                {"name", cls.name()},
                {"bases", String::Joined(bases, ", ").CString()},
                {"has_bases", !cls.bases().empty()},
            });

            printer_ << fmt("public partial class {{name}} : {{#has_bases}}{{bases}}, {{/has_bases}}IDisposable", vars);
            printer_.Indent();
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }
    else if (e.kind() == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& ctor = dynamic_cast<const cppast::cpp_constructor&>(e);
        const auto& cls = dynamic_cast<const cppast::cpp_class&>(e.parent().value());

        auto vars = fmt({
            {"class_name", e.parent().value().name()},
            {"parameter_list", ParameterList(ctor.parameters(), std::bind(&TypeMapper::ToCSType, typeMapper_, std::placeholders::_1)).CString()},
            {"symbol_name", Sanitize(GetSymbolName(e.parent().value())).CString()},
            {"param_name_list", ParameterNameList(ctor.parameters(), mapToPInvoke).CString()},
            {"has_base", !cls.bases().empty()},
            {"c_function_name", GetUserData(e)->cFunctionName.CString()},
            {"access", info.access == cppast::cpp_public ? "public" : "protected"}
        });

        // If class has a base class we call base constructor that does nothing. Class will be fully constructed here.
        printer_ << fmt("{{access}} {{class_name}}({{parameter_list}}){{#has_base}} : base(IntPtr.Zero){{/has_base}}", vars);
        printer_.Indent();
        {
            printer_ << fmt("instance_ = {{c_function_name}}({{param_name_list}});", vars);
            printer_ << fmt("{{class_name}}.cache_[instance_] = this;", vars);

            for (const auto& child : cls)
            {
                if (child.kind() == cppast::cpp_entity_kind::member_function_t)
                {
                    const auto& func = dynamic_cast<const cppast::cpp_member_function&>(child);
                    if (func.is_virtual())
                    {
                        if (!GetUserData(func)->generated)
                            continue;

                        if (generator->IsSubclassOf(cls, "Urho3D::RefCounted"))
                        {
                            // These are covered by c# itself.
                            if (func.name() == "GetType" || func.name() == "GetTypeName" ||
                                func.name() == "GetTypeInfo")
                                continue;
                        }

                        auto vars = fmt({
                            {"class_name", e.parent().value().name()},
                            {"name", func.name()},
                            {"has_params", !func.parameters().empty()},
                            {"param_name_list", ParameterNameList(func.parameters()).CString()},
                        });
                        printer_ << fmt("set_{{class_name}}_fn{{name}}(instance_, (instance{{#has_params}}, {{param_name_list}}{{/has_params}}) =>", vars);
                        printer_.Indent();
                        {
                            printer_ << fmt("this.{{name}}({{param_name_list}});", vars);
                        }
                        printer_.Dedent("});");
                    }
                }
            }

        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);

        auto vars = fmt({
            {"name", func.name()},
            {"return_type", typeMapper_->ToCSType(func.return_type()).CString()},
            {"parameter_list", ParameterList(func.parameters(), std::bind(&TypeMapper::ToCSType, typeMapper_, std::placeholders::_1)).CString()},
            {"c_function_name", GetUserData(func)->cFunctionName.CString()},
            {"param_name_list", ParameterNameList(func.parameters(), mapToPInvoke).CString()},
            {"has_params", !func.parameters().empty()},
            {"virtual", func.is_virtual() ? "virtual " : ""},
            {"access", info.access == cppast::cpp_public ? "public" : "protected"}
        });
        printer_ << fmt("{{access}} {{virtual}}{{return_type}} {{name}}({{parameter_list}})", vars);
        printer_.Indent();
        {
            String call = fmt("{{c_function_name}}(instance_{{#has_params}}, {{/has_params}}{{param_name_list}})", vars);
            if (IsVoid(func.return_type()))
                printer_ << call + ";";
            else
                printer_ << "return " + typeMapper_->MapToCS(func.return_type(), call, true) + ";";
        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = dynamic_cast<const cppast::cpp_member_variable&>(e);

        auto vars = fmt({
            {"cs_type", typeMapper_->ToCSType(var.type()).CString()},
            {"name", var.name()},
            {"class_symbol", Sanitize(GetSymbolName(var.parent().value())).CString()},
            {"access", info.access == cppast::cpp_public ? "public" : "protected"}
        });
        printer_ << fmt("{{access}} {{cs_type}} {{name}}", vars);
        printer_.Indent();
        {
            // Getter
            String call = typeMapper_->MapToCS(var.type(), fmt("get_{{class_symbol}}_{{name}}(instance_)", vars), false);
            printer_ << fmt("get { return {{call}}; }", {{"call", call.CString()}});
            // Setter
            vars.set("value", typeMapper_->MapToPInvoke(var.type(), "value").CString());
            printer_ << fmt("set { set_{{class_symbol}}_{{name}}(instance_, {{value}}); }", vars);
        }
        printer_.Dedent();
    }

    return true;
}

void GenerateCSApiPass::Stop()
{
    printer_ << "}";    // namespace Urho3D

    auto* generator = GetSubsystem<GeneratorContext>();
    String outputFile = generator->GetOutputDir() + "Urho3D.cs";
    File file(context_, outputFile, FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERRORF("Failed writing %s", outputFile.CString());
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

}
