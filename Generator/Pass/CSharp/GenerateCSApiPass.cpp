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
    printer_ << "using System.Diagnostics;";
    printer_ << "using System.Runtime.InteropServices;";
    printer_ << "using CSharp;";
    printer_ << "";
}

bool GenerateCSApiPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    auto mapToPInvoke = [&](MetaEntity* metaParam) {
        const auto& param = metaParam->Ast<cppast::cpp_function_parameter>();
        auto expr = EnsureNotKeyword(param.name());
        if (auto* map = generator->GetTypeMap(param.type()))
        {
            if (map->isValueType)
            {
                auto defaultValue = metaParam->GetDefaultValue();
                defaultValue = ConvertDefaultValueToCS(defaultValue, param.type(), true);
                if (!defaultValue.empty())
                    expr += fmt::format(".GetValueOrDefault({})", defaultValue);
            }
        }
        return MapToPInvoke(param.type(), expr);
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
            std::vector<std::string> baseInterfaces;
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
                    WeakPtr<MetaEntity> baseEntity;
                    if (generator->symbols_.TryGetValue(Urho3D::GetTypeName(base.type()), baseEntity))
                    {
                        std::string name;
                        if (baseEntity->flags_ & HintInterface)
                        {
                            baseInterfaces.emplace_back("I" + base.name());
                            if (!bases.empty())
                                name = "I";
                        }
                        name += base.name();
                        bases.emplace_back(name);
                    }
                    else
                        URHO3D_LOGWARNINGF("Unknown base class: %s", cppast::to_string(base.type()).c_str());
                }

                // Root object for native objects.
                if (bases.empty())
                    bases.emplace_back("NativeObject");

                // If this class was used in multiple inheritance and is marked as interface - it implements it's
                // own interface. It's own interface will be implementing other interfaces of the class so no point
                // listing them here again.
                if (entity->flags_ & HintInterface)
                {
                    bases.resize(1);
                    bases.emplace_back("I" + entity->name_);
                }

                bases.emplace_back("IDisposable");
            }

            if (isStatic)
                printer_ << fmt::format("public static partial class {}", entity->name_);
            else
            {
                printer_ << fmt::format("public unsafe partial class {} : {}", entity->name_, str::join(bases, ", "));

                if (entity->flags_ & HintInterface)
                {
                    auto interfaces = str::join(baseInterfaces, ", ");
                    if (!interfaces.empty())
                        interfaces = " : " + interfaces;
                    interface_.indent_ = 0;
                    interface_ << fmt::format("public unsafe interface I{}{}", entity->name_, interfaces);
                    interface_.indent_ = printer_.indent_;
                    interface_.Indent();
                }
            }

            printer_.Indent();

            if (!isStatic)
            {
                printer_ << "internal override void SetupInstance(IntPtr instance)";
                printer_.Indent();
                {
                    auto className = entity->name_;
                    printer_ << fmt::format("Debug.Assert(instance != IntPtr.Zero);");
                    printer_ << "instance_ = instance;";
                    if (generator->inheritable_.IsIncluded(entity->uniqueName_))
                        printer_ << fmt::format("{}_setup(instance, GCHandle.ToIntPtr(GCHandle.Alloc(this)), GetType().Name);",
                            Sanitize(entity->uniqueName_));
                    printer_ << fmt::format("InstanceCache.Add<{className}>(instance, this);", FMT_CAPTURE(className));

                    if (generator->inheritable_.IsIncluded(entity->symbolName_))
                    {
                        for (const auto& child : entity->children_)
                        {
                            if (child->kind_ == cppast::cpp_entity_kind::member_function_t)
                            {
                                const auto& func = child->Ast<cppast::cpp_member_function>();
                                if (func.is_virtual())
                                {
                                    auto name = child->name_;
                                    auto pc = func.parameters().empty() ? "" : ", ";
                                    auto paramTypeList = MapParameterList(child->children_, [&](MetaEntity* metaParam)
                                    {
                                        const auto& param = metaParam->Ast<cppast::cpp_function_parameter>();
                                        return fmt::format("typeof({})", ToCSType(param.type()));
                                    });
                                    auto paramNameListCs = MapParameterList(child->children_, [&](MetaEntity* metaParam)
                                    {
                                        const auto& param = metaParam->Ast<cppast::cpp_function_parameter>();
                                        return MapToCS(param.type(), param.name() + "_");
                                    });
                                    auto paramNameList = MapParameterList(child->children_, [&](MetaEntity* metaParam)
                                    {
                                        const auto& param = metaParam->Ast<cppast::cpp_function_parameter>();
                                        // Avoid possible parameter name collision in enclosing scope.
                                        return param.name() + "_";
                                    });

                                    // Optimization: do not route c++ virtual method calls through .NET if user does not override
                                    // such method in a managed class.
                                    printer_
                                        << fmt::format("if (GetType().HasOverride(nameof({name}){pc}{paramTypeList}))",
                                            FMT_CAPTURE(name), FMT_CAPTURE(pc), FMT_CAPTURE(paramTypeList));
                                    printer_.Indent();
                                    {
                                        printer_ << fmt::format("set_{sourceClass}_fn{cFunction}(instance, "
                                                "(gcHandle_{pc}{paramNameList}) =>",
                                            fmt::arg("sourceClass", Sanitize(entity->sourceSymbolName_)),
                                            fmt::arg("cFunction", child->cFunctionName_), FMT_CAPTURE(pc),
                                            FMT_CAPTURE(paramNameList));
                                        printer_.Indent();
                                        {
                                            auto expr = fmt::format(
                                                "(({className})GCHandle.FromIntPtr(gcHandle_).Target).{name}({paramNameListCs})",
                                                FMT_CAPTURE(className), FMT_CAPTURE(name),
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
            }
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";

            if (entity->flags_ & HintInterface)
            {
                interface_.Dedent();
                interface_ << "";
                printer_ << interface_.Get();
            }
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
        auto paramNameList = MapParameterList(entity->children_, mapToPInvoke);
        auto cFunctionName = entity->cFunctionName_;

        // If class has a base class we call base constructor that does nothing. Class will be fully constructed here.
        printer_ << fmt::format("{access} {className}({csParams}){baseCtor}", fmt::arg("access",
            entity->access_ == cppast::cpp_public ? "public" : "protected"), FMT_CAPTURE(className),
            FMT_CAPTURE(baseCtor), fmt::arg("csParams", FormatCSParameterList(entity->children_)));

        printer_.Indent();
        {
            printer_ << fmt::format("var instance = {cFunctionName}({paramNameList});",
                FMT_CAPTURE(cFunctionName), FMT_CAPTURE(paramNameList));
            printer_ << "SetupInstance(instance);";
        }
        printer_.Dedent();
        printer_ << "";

        // Implicit constructors with one parameter get conversion operators generated for them.
        if (Count(ctor.parameters()) == 1)
        {
            if (!ctor.is_explicit() && Urho3D::GetTypeName((*ctor.parameters().begin()).type()) != cls->symbolName_)
            {
                printer_ << fmt::format("public static implicit operator {className}({csParams})", FMT_CAPTURE(className),
                    fmt::arg("csParams", FormatCSParameterList(entity->children_)));

                printer_.Indent();
                {
                    auto paramNameList = MapParameterList(entity->children_, [&](MetaEntity* metaParam) {
                        return metaParam->name_;
                    });
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
        auto isFinal = !generator->inheritable_.IsIncluded(entity->parent_->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        const auto& func = entity->Ast<cppast::cpp_member_function>();

        auto rtype = ToCSType(func.return_type());
        auto pc = func.parameters().empty() ? "" : ", ";

        auto csParams = FormatCSParameterList(entity->children_);
        printer_ << fmt::format("{access} {virtual}{rtype} {name}({csParams})",
            fmt::arg("access", entity->access_ == cppast::cpp_public ? "public" : "protected"),
            fmt::arg("virtual", /*!isFinal &&*/ func.is_virtual() ? "virtual " : ""), FMT_CAPTURE(rtype),
            fmt::arg("name", entity->name_), FMT_CAPTURE(csParams));

        if (entity->access_ == cppast::cpp_public && entity->parent_->flags_ & HintInterface)
        {
            // Implement interface methods that come directly from interfaced class. If interfaced class inherits other
            // interfaces then these interfaces will be inherited instead.
            if (entity->sourceSymbolName_.find(entity->parent_->symbolName_) == 0)
            {
                interface_ << fmt::format("{rtype} {name}({csParams});", FMT_CAPTURE(rtype),
                    fmt::arg("name", entity->name_), FMT_CAPTURE(csParams));
            }
        }

        auto paramNameList = MapParameterList(entity->children_, mapToPInvoke);

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

        auto defaultValue = ConvertDefaultValueToCS(entity->GetDefaultValue(), var.type(), true);
        auto access = entity->access_ == cppast::cpp_public ? "public" : "protected";
        auto csType = ToCSType(var.type());
        auto name = entity->name_;
        std::string constant;
        if (defaultValue.empty())
            // No default value means we will have to generate a property with a getter.
            constant = "static";
        else if (entity->flags_ & HintReadOnly)
            // Explicitly requested to be readonly
            constant = "static readonly";
        else if (IsConst(var.type()))
        {
            if (GetBaseType(var.type()).kind() == cppast::cpp_type_kind::builtin_t)
                // Builtin type constants with default value can be "const". It also implies "static".
                constant = "const";
            else
                // Complex constant types with default values must be readonly.
                constant = "static readonly";
        }
        else
            // Anything else is simply static.
            constant = "static";

        auto line = fmt::format("{access} {constant} {csType} {name}",
            FMT_CAPTURE(access), FMT_CAPTURE(constant), FMT_CAPTURE(csType), FMT_CAPTURE(name));

        if (constant != "static")
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
        if (entity->flags_ & HintProperty)
        {
            MetaEntity* getter = nullptr;
            MetaEntity* setter = nullptr;
            for (const auto& child : entity->children_)
            {
                if (child->name_ == "set")
                    setter = child;
                else
                    getter = child;
            }
            assert(getter != nullptr);

            const auto& getterFunc = getter->Ast<cppast::cpp_member_function>();
            auto csType = ToCSType(getterFunc.return_type());

            auto access = entity->access_ == cppast::cpp_public ? "public" : "protected";
            printer_ << fmt::format("{access} {csType} {name}", FMT_CAPTURE(access), FMT_CAPTURE(csType),
                fmt::arg("name", entity->name_));
            printer_.Indent();
            {
                auto call = MapToCS(getterFunc.return_type(), fmt::format("{cFunction}(instance_)",
                    fmt::arg("cFunction", getter->cFunctionName_)));
                printer_ << fmt::format("get {{ return {call}; }}", FMT_CAPTURE(call));

                if (setter != nullptr)
                {
                    auto value = MapToPInvoke(getterFunc.return_type(), "value");
                    printer_ << fmt::format("set {{ {cFunction}(instance_, {value}); }}",
                        fmt::arg("cFunction", setter->cFunctionName_), FMT_CAPTURE(value));
                }
            }
            printer_.Dedent();
            printer_ << "";
        }
        else
        {
            auto isFinal = !generator->inheritable_.IsIncluded(entity->parent_->symbolName_);
            if (isFinal && entity->access_ != cppast::cpp_public)
                return true;

            const auto& var = entity->Ast<cppast::cpp_member_variable>();
            auto* ns = entity->parent_.Get();

            auto defaultValue = ConvertDefaultValueToCS(entity->GetDefaultValue(), var.type(), true);
            bool isConstant = IsConst(var.type()) && !(entity->flags_ & HintReadOnly) && !defaultValue.empty();
            auto csType = ToCSType(var.type());

            auto name = entity->name_;
            auto nsSymbol = Sanitize(ns->symbolName_);
            auto constant = entity->flags_ & HintReadOnly ? "readonly" : isConstant ? "const" : "";

            auto line = fmt::format("{access} {constant} {csType} {name}",
                fmt::arg("access", entity->access_ == cppast::cpp_public ? "public" : "protected"),
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
                    auto call = MapToCS(var.type(),
                        fmt::format("get_{nsSymbol}_{name}(instance_)", FMT_CAPTURE(nsSymbol), FMT_CAPTURE(name)));
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
        auto typeName = GetTemplateSubtype(type);
        if (typeName.empty())
            typeName = Urho3D::GetTypeName(type);

        typeName = "global::" + str::replace_str(typeName, "::", ".");
        return fmt::format("{}.__FromPInvoke({})", typeName, expression);
    }
    return expression;
}

std::string GenerateCSApiPass::ToCSType(const cppast::cpp_type& type)
{
    std::string result;
    if (const auto* map = generator->GetTypeMap(type))
        return map->csType_;

    auto typeName = GetTemplateSubtype(type);
    if (typeName.empty() && GetEntity(type) != nullptr)
        typeName = Urho3D::GetTypeName(type);

    if (!typeName.empty())
        return "global::" + str::replace_str(typeName, "::", ".");

    return ToPInvokeType(type, "IntPtr");
}

std::string GenerateCSApiPass::MapToPInvoke(const cppast::cpp_type& type, const std::string& expression)
{
    if (const auto* map = generator->GetTypeMap(type))
        return fmt::format(map->csToPInvokeTemplate_.c_str(), fmt::arg("value", expression));
    else if (IsComplexValueType(type))
    {
        auto typeName = GetTemplateSubtype(type);
        if (typeName.empty())
            typeName = Urho3D::GetTypeName(type);

        typeName = "global::" + str::replace_str(typeName, "::", ".");
        return fmt::format("{}.__ToPInvoke({})", typeName, expression);
    }
    return expression;
}

std::string GenerateCSApiPass::FormatCSParameterList(const std::vector<SharedPtr<MetaEntity>>& parameters)
{
    std::string result;
    for (const auto& param : parameters)
    {
        const auto& cppType = param->Ast<cppast::cpp_function_parameter>().type();
        auto csType = ToCSType(cppType);
        auto defaultValue = param->GetDefaultValue();
        if (auto* map = generator->GetTypeMap(cppType))
        {
            // Value types are made nullable in order to allow default values.
            if (map->isValueType && !defaultValue.empty())
                csType += "?";
        }
        result += fmt::format("{} {}", csType, EnsureNotKeyword(param->name_));

        if (!defaultValue.empty())
            result += "=" + ConvertDefaultValueToCS(defaultValue, cppType, false);

        if (param != parameters.back())
            result += ", ";
    }
    return result;
}

std::string GenerateCSApiPass::ConvertDefaultValueToCS(std::string value, const cppast::cpp_type& type,
    bool allowComplex)
{
    if (value.empty())
        return value;

    if (value == "nullptr")
        return "null";

    WeakPtr<MetaEntity> entity;
    if (auto* map = generator->GetTypeMap(type))
    {
        if (map->csType_ == "string")
        {
            // String literals
            if (value == "String::EMPTY")  // TODO: move to json?
                value = "\"\"";
        }
        else if (map->isValueType && !allowComplex)
        {
            // Value type parameters are turned to nullables when they have default values.
            return "null";
        }
    }

    if (!allowComplex && IsComplexValueType(type))
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

    return value;
}

}
