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

#include <fmt/format.h>
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
        return MapToPInvoke(param.type(), EnsureNotKeyword(param.name()));
    };

    if (entity->kind_ == cppast::cpp_entity_kind::namespace_t)
    {
        if (entity->children_.empty())
            return false;

        if (info.event == info.container_entity_enter)
        {
            printer_ << fmt::format("namespace {}", entity->name_);
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

            if (isStatic)
                printer_ << fmt::format("public static partial class {}", entity->name_);
            else
                printer_ << fmt::format("public unsafe partial class {} : {}", entity->name_, str::join(bases, ", "));

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
            if (GetEntity(base.type()) != nullptr)
            {
                hasBase = true;
                break;
            }
        }

        auto className = cls->name_;
        auto baseCtor = hasBase ? " : base(IntPtr.Zero)" : "";
        auto paramNameList = ParameterNameList(ctor.parameters(), mapToPInvoke);
        auto cFunctionName = entity->cFunctionName_;

        // If class has a base class we call base constructor that does nothing. Class will be fully constructed here.
        printer_.Write(fmt::format("{access} {className}(", fmt::arg("access",
            entity->access_ == cppast::cpp_public ? "public" : "protected"), FMT_CAPTURE(className)));

        PrintCSParameterList(entity->children_);
        printer_.Write(fmt::format("){baseCtor}", FMT_CAPTURE(baseCtor)));

        printer_.Indent();
        {
            printer_ << fmt::format("instance_ = {cFunctionName}({paramNameList});",
                FMT_CAPTURE(cFunctionName), FMT_CAPTURE(paramNameList));
            printer_ << fmt::format("InstanceCache.Add<{className}>(instance_, this);", FMT_CAPTURE(className));

            if (!generator->final_.Contains(entity->parent_->symbolName_))
            {
                for (const auto& child : cls->children_)
                {
                    if (child->kind_ == cppast::cpp_entity_kind::member_function_t)
                    {
                        const auto& func = child->Ast<cppast::cpp_member_function>();
                        if (func.is_virtual())
                        {
                            auto name = child->name_;
                            auto pc = func.parameters().empty() ? "" : ", ";
                            auto paramTypeList = ParameterTypeList(func.parameters(),
                                [&](const cppast::cpp_type& type) -> std::string
                                {
                                    return fmt::format("typeof({})", ToCSType(type));
                                });
                            auto paramNameListCs = ParameterNameList(func.parameters(),
                                [&](const cppast::cpp_function_parameter& param)
                                {
                                    return MapToCS(param.type(), param.name() + "_");
                                });
                            auto paramNameList = ParameterNameList(func.parameters(),
                                [&](const cppast::cpp_function_parameter& param)
                                {
                                    // Avoid possible parameter name collision in enclosing scope.
                                    return param.name() + "_";
                                });

                            // Optimization: do not route c++ virtual method calls through .NET if user does not override
                            // such method in a managed class.
                            printer_ << fmt::format("if (GetType().HasOverride(nameof({name}){pc}{paramTypeList}))",
                                FMT_CAPTURE(name), FMT_CAPTURE(pc), FMT_CAPTURE(paramTypeList));
                            printer_.Indent();
                            {
                                printer_ << fmt::format("set_{sourceClass}_fn{cFunction}(instance_, "
                                        "(instance__{pc}{paramNameList}) =>",
                                    fmt::arg("sourceClass", Sanitize(cls->sourceName_)),
                                    fmt::arg("cFunction", child->cFunctionName_), FMT_CAPTURE(pc),
                                    FMT_CAPTURE(paramNameList));
                                printer_.Indent();
                                {
                                    auto expr = fmt::format("{name}({paramNameListCs})", FMT_CAPTURE(name),
                                        FMT_CAPTURE(paramNameListCs));
                                    if (!IsVoid(func.return_type()))
                                    {
                                        expr = MapToPInvoke(func.return_type(), expr);
                                        printer_.Write(fmt::format("return {}", expr));
                                    }
                                    else
                                        printer_.Write(expr);
                                    printer_.Write(";");
                                }
                                printer_.Dedent("});");
                            }
                            printer_.Dedent();
                        }
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
                printer_.Write(fmt::format("public static implicit operator {className}(", FMT_CAPTURE(className)));
                PrintCSParameterList(entity->children_);
                printer_.Write(")");

                printer_.Indent();
                {
                    printer_ << fmt::format("return new {className}({paramNameList});",
                        FMT_CAPTURE(className), FMT_CAPTURE(paramNameList));
                }
                printer_.Dedent();
                printer_ << "";
            }
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        auto isFinal = generator->final_.Contains(entity->parent_->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        const auto& func = entity->Ast<cppast::cpp_member_function>();

        auto rtype = ToCSType(func.return_type());
        auto pc = func.parameters().empty() ? "" : ", ";
        auto paramNameList = ParameterNameList(func.parameters(), mapToPInvoke);

        // Function start
        printer_.Write(fmt::format("{access} {virtual}{rtype} {name}(",
            fmt::arg("access", entity->access_ == cppast::cpp_public ? "public" : "protected"),
            fmt::arg("virtual", func.is_virtual() ? "virtual " : ""), FMT_CAPTURE(rtype),
            fmt::arg("name", entity->name_)));

        // Parameter list
        PrintCSParameterList(entity->children_);
        printer_.Write(")");

        // Body
        printer_.Indent();
        {
            std::string call = fmt::format("{cFunction}(instance_{pc}{paramNameList})",
                fmt::arg("cFunction", entity->cFunctionName_), FMT_CAPTURE(pc), FMT_CAPTURE(paramNameList));

            if (IsVoid(func.return_type()))
                printer_ << call + ";";
            else
                printer_ << "return " + MapToCS(func.return_type(), call) + ";";
        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_variable>();
        auto* ns = entity->parent_.Get();

        auto defaultValue = GetConvertedDefaultValue(entity, true);
        bool isConstant = IsConst(var.type()) && !(entity->flags_ & HintReadOnly) && !defaultValue.empty();
        auto csType = ToCSType(var.type());
        auto name = entity->name_;
        auto constant = entity->flags_ & HintReadOnly ? "static readonly" :
                                           isConstant ? "const"           : "static";
        auto access = entity->access_ == cppast::cpp_public ? "public" : "protected";

        auto line = fmt::format("{access} {constant} {csType} {name}",
            FMT_CAPTURE(access), FMT_CAPTURE(constant), FMT_CAPTURE(csType), FMT_CAPTURE(name));

        if (isConstant || entity->flags_ & HintReadOnly)
        {
            line += " = " + defaultValue + ";";
            printer_ << line;
        }
        else
        {
            // A property with getters and setters
            auto nsSymbol = Sanitize(ns->symbolName_);
            printer_ << line;
            printer_.Indent();
            {
                // Getter
                auto call = MapToCS(var.type(),
                    fmt::format("get_{nsSymbol}_{name}()", FMT_CAPTURE(nsSymbol), FMT_CAPTURE(name)));
                printer_ << fmt::format("get {{ return {call}; }}", fmt::arg("call", call));
                // Setter
                if (!IsConst(var.type()) && !(entity->flags_ & HintReadOnly))
                {
                    auto value = MapToPInvoke(var.type(), "value");
                    printer_ << fmt::format("set {{ set_{nsSymbol}_{name}({value}); }}",
                        FMT_CAPTURE(nsSymbol), FMT_CAPTURE(name), FMT_CAPTURE(value));
                }
            }
            printer_.Dedent();
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
    {
        auto isFinal = generator->final_.Contains(entity->parent_->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        const auto& var = entity->Ast<cppast::cpp_member_variable>();
        auto* ns = entity->parent_.Get();

        auto defaultValue = GetConvertedDefaultValue(entity, true);
        bool isConstant = IsConst(var.type()) && !(entity->flags_ & HintReadOnly) && !defaultValue.empty();
        auto csType = ToCSType(var.type());

        auto name = entity->name_;
        auto nsSymbol = Sanitize(ns->symbolName_);
        auto constant = entity->flags_ & HintReadOnly ? "readonly" :
                                           isConstant ? "const"    : "";

        auto line = fmt::format("{access} {constant} {csType} {name}",
            fmt::arg("access", entity->access_ == cppast::cpp_public? "public" : "protected"),
            FMT_CAPTURE(constant), FMT_CAPTURE(csType), FMT_CAPTURE(name));

        if (isConstant)
            line += " = " + defaultValue + ";";
        else
        {
            // A property with getters and setters
            printer_ << line;
            printer_.Indent();
            {
                // Getter
                auto call = MapToCS(var.type(), fmt::format("get_{nsSymbol}_{name}(instance_)",
                    FMT_CAPTURE(nsSymbol), FMT_CAPTURE(name)));
                printer_ << fmt::format("get {{ return {}; }}", call);
                // Setter
                if (!IsConst(var.type()) && !(entity->flags_ & HintReadOnly))
                {
                    auto value = MapToPInvoke(var.type(), "value");
                    printer_ << fmt::format("set {{ set_{nsSymbol}_{name}(instance_, {value}); }}",
                        FMT_CAPTURE(nsSymbol), FMT_CAPTURE(name), FMT_CAPTURE(value));
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

std::string GenerateCSApiPass::MapToCS(const cppast::cpp_type& type, const std::string& expression)
{
    if (const auto* map = generator->GetTypeMap(type))
        return fmt::format(map->pInvokeToCSTemplate_.c_str(), fmt::arg("value", expression));
    else if (IsComplexValueType(type))
    {
        std::string returnType = "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
        return fmt::format("{}.__FromPInvoke({})", returnType, expression);
    }
    return expression;
}

std::string GenerateCSApiPass::ToCSType(const cppast::cpp_type& type)
{
    std::string result;
    if (const auto* map = generator->GetTypeMap(type))
        result = map->csType_;
    else if (GetEntity(type) != nullptr)
        return "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
    else
        result = ToPInvokeType(type, "IntPtr");
    return result;
}

std::string GenerateCSApiPass::MapToPInvoke(const cppast::cpp_type& type, const std::string& expression)
{
    if (const auto* map = generator->GetTypeMap(type))
        return fmt::format(map->csToPInvokeTemplate_.c_str(), fmt::arg("value", expression));
    else if (IsComplexValueType(type))
    {
        std::string returnType = "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
        return fmt::format("{}.__ToPInvoke({})", returnType, expression);
    }
    return expression;
}

void GenerateCSApiPass::PrintCSParameterList(const std::vector<SharedPtr<MetaEntity>>& parameters)
{
    for (const auto& param : parameters)
    {
        const auto& cppType = param->Ast<cppast::cpp_function_parameter>().type();
        printer_.Write(fmt::format("{} {}", ToCSType(cppType), EnsureNotKeyword(param->name_)));

        auto defaultValue = GetConvertedDefaultValue(param, false);
        if (!defaultValue.empty())
            printer_.Write("=" + defaultValue);

        if (param != parameters.back())
            printer_.Write(", ");
    }
}

std::string GenerateCSApiPass::GetConvertedDefaultValue(MetaEntity* param, bool allowComplex)
{
    const cppast::cpp_type* cppType;

    switch (param->kind_)
    {
    case cppast::cpp_entity_kind::function_parameter_t:
        cppType = &param->Ast<cppast::cpp_function_parameter>().type();
        break;
    case cppast::cpp_entity_kind::member_variable_t:
        cppType = &param->Ast<cppast::cpp_member_variable>().type();
        break;
    case cppast::cpp_entity_kind::variable_t:
        cppType = &param->Ast<cppast::cpp_variable>().type();
        break;
    default:
        assert(false);
    }

    auto value = param->GetDefaultValue();
    if (!value.empty())
    {
        auto* typeMap = generator->GetTypeMap(*cppType);
        WeakPtr<MetaEntity> entity;
        if (typeMap != nullptr && typeMap->csType_ == "string")
        {
            // String literals
            if (value == "String::EMPTY")  // TODO: move to json?
                value = "\"\"";
        }
        else if ((!allowComplex && IsComplexValueType(*cppType)) || value == "nullptr")
        {
            // C# may only have default values constructed by default constructor. Because of this such default
            // values are replaced with null. Function body will construct actual default value if parameter is
            // null.
            value = "null";
        }
        else if (generator->symbols_.TryGetValue("Urho3D::" + value, entity))
            value = entity->symbolName_;
        else if (generator->enumValues_.TryGetValue(value, entity))
            value = entity->symbolName_;

        str::replace_str(value, "::", ".");
    }
    return value;
}

}
