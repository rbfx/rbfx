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
#include <Declarations/Class.hpp>
#include <Declarations/Variable.hpp>
#include "GeneratorContext.h"
#include "GenerateCSApiPass.h"

namespace Urho3D
{

void GenerateCSApiPass::Start()
{
    generator_ = GetSubsystem<GeneratorContext>();
    typeMapper_ = &generator_->typeMapper_;

    printer_ << "using System;";
    printer_ << "using CSharp;";
    printer_ << "";
    printer_ << "namespace Urho3D";
    printer_ << "{";
    printer_ << "";
}

bool GenerateCSApiPass::Visit(Declaration* decl, Event event)
{
    auto mapToPInvoke = [&](const cppast::cpp_function_parameter& param) {
        return typeMapper_->MapToPInvoke(param.type(), EnsureNotKeyword(param.name()));
    };

    auto mapToCS = [&](const cppast::cpp_function_parameter& param) {
        return typeMapper_->MapToCS(param.type(), EnsureNotKeyword(param.name()));
    };

    if (decl->kind_ == Declaration::Kind::Class)
    {
        Class* cls = dynamic_cast<Class*>(decl);
        if (event == Event::ENTER)
        {
            if (cls->isStatic_)
            {
                // A class will not have any methods. This is likely a dummy class for constant storage or something
                // similar.
                printer_ << fmt("public static partial class {{name}}", {{"name", cls->name_.CString()}});
                printer_.Indent();
                return true;
            }

            Vector<String> bases;
            for (const auto& base : cls->bases_)
                bases.Push(base->name_);

            auto vars = fmt({
                {"name", cls->name_.CString()},
                {"bases", String::Joined(bases, ", ").CString()},
                {"has_bases", !cls->bases_.Empty()},
            });

            printer_ << fmt("public partial class {{name}} : {{#has_bases}}{{bases}}, {{/has_bases}}IDisposable", vars);
            printer_.Indent();
        }
        else if (event == Event::EXIT)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }
    else if (decl->kind_ == Declaration::Kind::Constructor)
    {
        Function* ctor = dynamic_cast<Function*>(decl);
        Class* cls = dynamic_cast<Class*>(decl->parent_.Get());

        auto vars = fmt({
            {"class_name", cls->name_.CString()},
            {"parameter_list", ParameterList(ctor->GetParameters(), std::bind(&TypeMapper::ToCSType, typeMapper_, std::placeholders::_1)).CString()},
            {"symbol_name", Sanitize(cls->symbolName_).CString()},
            {"param_name_list", ParameterNameList(ctor->GetParameters(), mapToPInvoke).CString()},
            {"has_base", !cls->bases_.Empty()},
            {"c_function_name", decl->cFunctionName_.CString()},
            {"access", decl->isPublic_ ? "public" : "protected"}
        });

        // If class has a base class we call base constructor that does nothing. Class will be fully constructed here.
        printer_ << fmt("{{access}} {{class_name}}({{parameter_list}}){{#has_base}} : base(IntPtr.Zero){{/has_base}}", vars);
        printer_.Indent();
        {
            printer_ << fmt("instance_ = {{c_function_name}}({{param_name_list}});", vars);
            printer_ << fmt("InstanceCache.Add<{{class_name}}>(instance_, this);", vars);

            for (const auto& child : cls->children_)
            {
                if (child->kind_ == Declaration::Kind::Method)
                {
                    const auto& func = dynamic_cast<Function*>(child.Get());
                    if (func->IsVirtual())
                    {
                        auto vars = fmt({
                            {"class_name", cls->name_.CString()},
                            {"name", func->name_.CString()},
                            {"has_params", !func->GetParameters().empty()},
                            {"param_name_list", ParameterNameList(func->GetParameters()).CString()},
                            {"cs_param_name_list", ParameterNameList(func->GetParameters(), mapToCS).CString()},
                            {"cs_param_type_list", ParameterTypeList(func->GetParameters(), [&](const cppast::cpp_type& type) {
                                return ToString("typeof(%s)", generator->typeMapper_.ToCSType(type).CString());
                            }).CString()},
                            {"return", !IsVoid(func->GetReturnType()) ? "return " : ""},
                            {"source_class_name", Sanitize(cls->sourceName_).CString()},
                            {"c_function_name", func->cFunctionName_.CString()},
                        });
                        // Optimization: do not route c++ virtual method calls through .NET if user does not override
                        // such method in a managed class.
                        printer_ << fmt("if (GetType().HasOverride(nameof({{name}}){{#has_params}}, {{/has_params}}{{cs_param_type_list}}))", vars);
                        printer_.Indent();
                        {
                            printer_ << fmt("set_{{source_class_name}}_fn{{c_function_name}}(instance_, "
                                            "(instance__{{#has_params}}, {{param_name_list}}{{/has_params}}) =>", vars);
                            printer_.Indent();
                            {
                                printer_ << fmt("{{return}}this.{{name}}({{cs_param_name_list}});", vars);
                            }
                            printer_.Dedent("});");
                        }
                        printer_.Dedent();
                    }
                }
            }
        }
        printer_.Dedent();
        printer_ << "";

        // Implicit constructors with one parameter get conversion operators generated for them.
        if (ctor->source_ != nullptr && Count(ctor->GetParameters()) == 1)
        {
            const auto* astCtor = (const cppast::cpp_constructor*) ctor->source_;
            if (!astCtor->is_explicit())
            {
                printer_ << fmt("public static implicit operator {{class_name}}({{parameter_list}})", vars);
                printer_.Indent();
                {
                    printer_ << fmt("return new {{class_name}}({{param_name_list}});", vars);
                }
                printer_.Dedent();
                printer_ << "";
            }
        }
    }
    else if (decl->kind_ == Declaration::Kind::Method)
    {
        Function* func = dynamic_cast<Function*>(decl);

        auto vars = fmt({
            {"name", func->name_.CString()},
            {"return_type", typeMapper_->ToCSType(func->GetReturnType()).CString()},
            {"parameter_list", ParameterList(func->GetParameters(), std::bind(&TypeMapper::ToCSType, typeMapper_, std::placeholders::_1)).CString()},
            {"c_function_name", func->cFunctionName_.CString()},
            {"param_name_list", ParameterNameList(func->GetParameters(), mapToPInvoke).CString()},
            {"has_params", !func->GetParameters().empty()},
            {"virtual", func->IsVirtual() ? "virtual " : ""},
            {"access", decl->isPublic_ ? "public" : "protected"}
        });
        printer_ << fmt("{{access}} {{virtual}}{{return_type}} {{name}}({{parameter_list}})", vars);
        printer_.Indent();
        {
            String call = fmt("{{c_function_name}}(instance_{{#has_params}}, {{/has_params}}{{param_name_list}})", vars);
            if (IsVoid(func->GetReturnType()))
                printer_ << call + ";";
            else
                printer_ << "return " + typeMapper_->MapToCS(func->GetReturnType(), call) + ";";
        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (decl->kind_ == Declaration::Kind::Variable)
    {
        Variable* var = dynamic_cast<Variable*>(decl);
        Namespace* ns = dynamic_cast<Namespace*>(var->parent_.Get());

        bool isStatic = decl->isStatic_;
        if (ns->isStatic_ && !var->defaultValue_.Empty())
            // C# class is marked as static and this is a constant value. Constants in static classes must not be marked
            // as static in c# for some strange reason.
            isStatic = false;

        auto vars = fmt({
            {"cs_type", typeMapper_->ToCSType(var->GetType()).CString()},
            {"name", var->name_.CString()},
            {"ns_symbol", Sanitize(ns->symbolName_).CString()},
            {"access", decl->isPublic_ ? "public " : "protected "},
            {"static", isStatic ? "static " : ""},
            {"const", decl->isConstant_ && !var->defaultValue_.Empty() ? "const " : ""},
            {"not_static", !decl->isStatic_},
            {"not_enum", ns->kind_ != Declaration::Kind::Enum}
        });

        String line = fmt("{{#not_enum}}{{access}}{{static}}{{const}}{{cs_type}} {{/not_enum}}{{name}}", vars);
        if ((!var->defaultValue_.Empty() || ns->kind_ == Declaration::Kind::Enum) && var->isConstant_)
        {
            // A constant value
            if (!var->defaultValue_.Empty())
                line += " = " + var->defaultValue_;
            if (ns->kind_ == Declaration::Kind::Enum)
                line += ",";
            else
                line += ";";
            printer_ << line;
        }
        else
        {
            // A property with getters and setters
            printer_ << line;
            printer_.Indent();
            {
                // Getter
                String call = typeMapper_->MapToCS(var->GetType(), fmt("get_{{ns_symbol}}_{{name}}({{#not_static}}instance_{{/not_static}})",
                                                                       vars));
                printer_ << fmt("get { return {{call}}; }", {{"call", call.CString()}});
                // Setter
                if (!var->isConstant_)
                {
                    vars.set("value", typeMapper_->MapToPInvoke(var->GetType(), "value").CString());
                    printer_ << fmt("set { set_{{ns_symbol}}_{{name}}({{#not_static}}instance_, {{/not_static}}{{value}}); }", vars);
                }
            }
            printer_.Dedent();
        }
    }
    else if (decl->kind_ == Declaration::Kind::Enum)
    {
        if (event == Event::ENTER)
        {
            printer_ << "public enum " + decl->name_;
            printer_.Indent();
        }
        else if (event == Event::EXIT)
            printer_.Dedent();
    }

    return true;
}

void GenerateCSApiPass::Stop()
{
    printer_ << "}";    // namespace Urho3D

    auto* generator = GetSubsystem<GeneratorContext>();
    String outputFile = generator->outputDir_ + "Urho3D.cs";
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
