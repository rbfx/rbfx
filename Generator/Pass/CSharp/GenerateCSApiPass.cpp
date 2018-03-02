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
#include <cppast/cpp_namespace.hpp>
#include "GeneratorContext.h"
#include "GenerateCSApiPass.h"

namespace Urho3D
{

void GenerateCSApiPass::Start()
{
    printer_ << "using System;";
    printer_ << "using CSharp;";
    printer_ << "";
}

bool GenerateCSApiPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    auto mapToPInvoke = [&](const cppast::cpp_function_parameter& param) {
        return generator->typeMapper_.MapToPInvoke(param.type(), EnsureNotKeyword(param.name()));
    };

    if (entity->kind_ == cppast::cpp_entity_kind::namespace_t)
    {
        if (entity->children_.empty())
            return false;

        if (info.event == info.container_entity_enter)
        {
            printer_ << fmt("namespace {{name}}", {{"name", entity->name_}});
            printer_.Indent();
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";
        }
        return true;
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::class_t)
    {
        if (info.event == info.container_entity_enter)
        {
            bool isStatic = false;
            std::vector<std::string> bases;
            if (entity->ast_ != nullptr)
                isStatic = IsStatic(*entity->ast_);
            else
                isStatic = true;

            if (!isStatic && entity->ast_->kind() == cppast::cpp_entity_kind::class_t)
            {
                const auto& cls = entity->Ast<cppast::cpp_class>();
                // Not necessary here, but makes it convenient allowing to not handle case where no bases exist.
                for (const auto& base : cls.bases())
                {
                    if (const auto* baseClass = GetEntity(base.type()))
                        bases.emplace_back(base.name());
                    else
                        URHO3D_LOGWARNINGF("Unknown base class: %s", cppast::to_string(base.type()).c_str());
                }
                bases.emplace_back("IDisposable");
            }

            auto vars = fmt({
                {"name", entity->name_},
                {"bases", str::join(bases, ", ")},
            });

            if (isStatic)
                printer_ << fmt("public static partial class {{name}}", vars);
            else
                printer_ << fmt("public partial class {{name}} : {{bases}}", vars);

            printer_.Indent();
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::enum_t)
    {
        if (info.event == info.container_entity_enter)
        {
            printer_ << "public enum " + entity->name_;
            printer_.Indent();
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }

    if (info.event == info.container_entity_exit)
        return true;

    if (entity->kind_ == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& ctor = entity->Ast<cppast::cpp_constructor>();
        MetaEntity* cls = entity->parent_.Get();

        bool hasBase = false;
        for (const auto& base : cls->Ast<cppast::cpp_class>().bases())
        {
            if (const auto* baseClass = GetEntity(base.type()))
            {
                hasBase = true;
                break;
            }
        }

        auto vars = fmt({
            {"class_name", cls->name_},
            {"parameter_list", ParameterList(ctor.parameters(),
                std::bind(&TypeMapper::ToCSType, &generator->typeMapper_, std::placeholders::_1), ".")},
            {"symbol_name", Sanitize(cls->symbolName_)},
            {"param_name_list", ParameterNameList(ctor.parameters(), mapToPInvoke)},
            {"has_base", hasBase},
            {"c_function_name", entity->cFunctionName_},
            {"access", entity->access_ == cppast::cpp_public ? "public" : "protected"}
        });

        // If class has a base class we call base constructor that does nothing. Class will be fully constructed here.
        printer_ << fmt("{{access}} {{class_name}}({{parameter_list}}){{#has_base}} : base(IntPtr.Zero){{/has_base}}", vars);
        printer_.Indent();
        {
            printer_ << fmt("instance_ = {{c_function_name}}({{param_name_list}});", vars);
            printer_ << fmt("InstanceCache.Add<{{class_name}}>(instance_, this);", vars);

            for (const auto& child : cls->children_)
            {
                if (child->kind_ == cppast::cpp_entity_kind::member_function_t)
                {
                    const auto& func = child->Ast<cppast::cpp_member_function>();
                    if (func.is_virtual())
                    {
                        auto vars = fmt({
                            {"class_name", cls->name_},
                            {"name", child->name_},
                            {"has_params", !func.parameters().empty()},
                            {"param_name_list", ParameterNameList(func.parameters(), [&](const cppast::cpp_function_parameter& param) {
                                // Avoid possible parameter name collision in enclosing scope.
                                return param.name() + "_";
                            })},
                            {"cs_param_name_list", ParameterNameList(func.parameters(), [&](const cppast::cpp_function_parameter& param) {
                                return generator->typeMapper_.MapToCS(param.type(), param.name() + "_");
                            })},
                            {"cs_param_type_list", ParameterTypeList(func.parameters(), [&](const cppast::cpp_type& type) -> std::string {
                                return fmt("typeof({{type}})", {{"type", generator->typeMapper_.ToCSType(type)}});
                            })},
                            {"return", !IsVoid(func.return_type()) ? "return " : ""},
                            {"source_class_name", Sanitize(cls->sourceName_)},
                            {"c_function_name", child->cFunctionName_},
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
        if (Count(ctor.parameters()) == 1)
        {
            if (!ctor.is_explicit() && Urho3D::GetTypeName((*ctor.parameters().begin()).type()) != cls->symbolName_)
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
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_member_function>();

        auto vars = fmt({
            {"name", entity->name_},
            {"return_type", generator->typeMapper_.ToCSType(func.return_type())},
            {"parameter_list", ParameterList(func.parameters(),
                std::bind(&TypeMapper::ToCSType, &generator->typeMapper_, std::placeholders::_1), ".")},
            {"c_function_name", entity->cFunctionName_},
            {"param_name_list", ParameterNameList(func.parameters(), mapToPInvoke)},
            {"has_params", !func.parameters().empty()},
            {"virtual", func.is_virtual() ? "virtual " : ""},
            {"access", entity->access_ == cppast::cpp_public ? "public" : "protected"}
        });
        printer_ << fmt("{{access}} {{virtual}}{{return_type}} {{name}}({{parameter_list}})", vars);
        printer_.Indent();
        {
            std::string call = fmt("{{c_function_name}}(instance_{{#has_params}}, {{/has_params}}{{param_name_list}})", vars);
            if (IsVoid(func.return_type()))
                printer_ << call + ";";
            else
                printer_ << "return " + generator->typeMapper_.MapToCS(func.return_type(), call) + ";";
        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_variable>();
        auto* ns = entity->parent_.Get();

        auto defaultValue = ExpandDefaultValue(ns->name_, entity->GetDefaultValue());
        bool isConstant = IsConst(var.type()) && !(entity->flags_ & HintReadOnly) && !defaultValue.empty();

        auto vars = fmt({
            {"cs_type", generator->typeMapper_.ToCSType(var.type())},
            {"name", entity->name_},
            {"ns_symbol", Sanitize(ns->symbolName_)},
            {"access", entity->access_ == cppast::cpp_public ? "public" : "protected"},
            {"const", entity->flags_ & HintReadOnly ? "readonly" :
                                         isConstant ? "const"    : "static"
            },
        });

        auto line = fmt("{{access}} {{const}} {{cs_type}} {{name}}", vars);
        if (isConstant || entity->flags_ & HintReadOnly)
        {
            line += " = " + defaultValue + ";";
            printer_ << line;
        }
        else
        {
            // A property with getters and setters
            printer_ << line;
            printer_.Indent();
            {
                // Getter
                String call = generator->typeMapper_.MapToCS(var.type(), fmt("get_{{ns_symbol}}_{{name}}()", vars));
                printer_ << fmt("get { return {{call}}; }", {{"call", call.CString()}});
                // Setter
                if (!IsConst(var.type()) && !(entity->flags_ & HintReadOnly))
                {
                    vars.set("value", generator->typeMapper_.MapToPInvoke(var.type(), "value"));
                    printer_ << fmt("set { set_{{ns_symbol}}_{{name}}({{value}}); }", vars);
                }
            }
            printer_.Dedent();
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_member_variable>();
        auto* ns = entity->parent_.Get();

        auto defaultValue = ExpandDefaultValue(ns->parent_->name_, entity->GetDefaultValue());
        bool isConstant = IsConst(var.type()) && !(entity->flags_ & HintReadOnly) && !defaultValue.empty();

        auto vars = fmt({
            {"cs_type", generator->typeMapper_.ToCSType(var.type())},
            {"name", entity->name_},
            {"ns_symbol", Sanitize(ns->symbolName_)},
            {"access", entity->access_ == cppast::cpp_public? "public " : "protected "},
            {"const", entity->flags_ & HintReadOnly ? "readonly " :
                                         isConstant ? "const "    : " "
            },
        });

        auto line = fmt("{{access}}{{const}}{{cs_type}} {{name}}", vars);
        if (isConstant)
            line += " = " + defaultValue + ";";
        else
        {
            // A property with getters and setters
            printer_ << line;
            printer_.Indent();
            {
                // Getter
                String call = generator->typeMapper_.MapToCS(var.type(), fmt("get_{{ns_symbol}}_{{name}}(instance_)",
                    vars));
                printer_ << fmt("get { return {{call}}; }", {{"call", call.CString()}});
                // Setter
                if (!IsConst(var.type()) && !(entity->flags_ & HintReadOnly))
                {
                    vars.set("value", generator->typeMapper_.MapToPInvoke(var.type(), "value"));
                    printer_ << fmt("set { set_{{ns_symbol}}_{{name}}(instance_, {{value}}); }", vars);
                }
            }
            printer_.Dedent();
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::enum_value_t)
    {
        auto line = entity->name_;
        auto defaultValue = entity->GetDefaultValue();
        if (!defaultValue.empty())
            line += " = " + defaultValue;
        printer_ << line + ",";
    }

    return true;
}

void GenerateCSApiPass::Stop()
{
    auto* generator = GetSubsystem<GeneratorContext>();
    String outputFile = generator->outputDirCs_ + "Urho3D.cs";
    File file(context_, outputFile, FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERRORF("Failed writing %s", outputFile.CString());
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

std::string GenerateCSApiPass::ExpandDefaultValue(const std::string& currentNamespace, const std::string& value)
{
    WeakPtr<MetaEntity> entity;
    if (generator->symbols_.Contains(value))
        return value;
    if (!currentNamespace.empty())
    {
        std::string fullName = currentNamespace + "::" + value;
        if (generator->symbols_.Contains(fullName))
            return str::replace_str(fullName, "::", ".");
    }
    return value;
}

}
