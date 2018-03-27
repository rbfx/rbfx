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
#include <cppast/cpp_template.hpp>
#include <cppast/cpp_type.hpp>
#include "GeneratorContext.h"
#include "GenerateCSApiPass.h"
#include "Pass/CSharp/GeneratePInvokePass.h"

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
        std::string expr;
        if (IsComplexOutputType(param.type()))
            return "ref " + param.name() + "Out";
        else
            expr = EnsureNotKeyword(param.name());

        if (!IsOutType(param.type()))
        {
            if (auto* map = generator->GetTypeMap(param.type(), false))
            {
                if (map->isValueType_ && map->csType_ != "string")
                {
                    auto defaultValue = metaParam->GetDefaultValue();
                    defaultValue = ConvertDefaultValueToCS(defaultValue, param.type(), true);
                    if (!defaultValue.empty())
                        expr += fmt::format(".GetValueOrDefault({})", defaultValue);
                }
            }
        }

        expr = MapToPInvoke(param.type(), expr);

        if (IsOutType(param.type()))
            expr = "ref " + expr;

        return expr;
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
            std::vector<std::string> baseInterfaces{"INativeObject"};
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
                    printer_ << "InstanceCache.Add(this);";

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
                                        return fmt::format("typeof({})", ToCSType(param.type(), true));
                                    });
                                    auto paramNameListCs = MapParameterList(child->children_, [&](MetaEntity* metaParam)
                                    {
                                        const auto& param = metaParam->Ast<cppast::cpp_function_parameter>();
                                        std::string result;
                                        if (IsComplexOutputType(param.type()))
                                            result = param.name() + "Out";
                                        else
                                            result = MapToCS(param.type(), param.name() + "_");
                                        if (IsOutType(param.type()))
                                            result = "ref " + result;
                                        return result;
                                    });
                                    auto paramNameList = MapParameterList(child->children_, [&](MetaEntity* metaParam)
                                    {
                                        const auto& param = metaParam->Ast<cppast::cpp_function_parameter>();
                                        // Types in lambda declaration are required in case of ref parameters.
                                        auto type = GeneratePInvokePass::ToPInvokeType(param.type());
                                        auto name = param.name();
                                        // Avoid possible parameter name collision in enclosing scope by appending _.
                                        return fmt::format("{type} {name}_", FMT_CAPTURE(type), FMT_CAPTURE(name));
                                    });

                                    // Optimization: do not route c++ virtual method calls through .NET if user does not override
                                    // such method in a managed class.
                                    printer_
                                        << fmt::format("if (GetType().HasOverride(nameof({name}){pc}{paramTypeList}))",
                                            FMT_CAPTURE(name), FMT_CAPTURE(pc), FMT_CAPTURE(paramTypeList));
                                    printer_.Indent();
                                    {
                                        printer_ << fmt::format("set_{sourceClass}_fn{cFunction}(instance, "
                                                "(IntPtr gcHandle_{pc}{paramNameList}) =>",
                                            fmt::arg("sourceClass", Sanitize(entity->sourceSymbolName_)),
                                            fmt::arg("cFunction", child->cFunctionName_), FMT_CAPTURE(pc),
                                            FMT_CAPTURE(paramNameList));
                                        printer_.Indent();
                                        {
                                            // ref parameters typemapped to C# types
                                            for (const auto& param : child->children_)
                                            {
                                                const auto& type = param->Ast<cppast::cpp_function_parameter>().type();
                                                if (IsComplexOutputType(type))
                                                    printer_ << "var " + param->name_ + "Out = " + MapToCS(type, param->name_ + "_") + ";";
                                            }

                                            auto expr = fmt::format(
                                                "(({className})GCHandle.FromIntPtr(gcHandle_).Target).{name}({paramNameListCs})",
                                                FMT_CAPTURE(className), FMT_CAPTURE(name),
                                                FMT_CAPTURE(paramNameListCs));
                                            if (!IsVoid(func.return_type()))
                                            {
                                                expr = MapToPInvoke(func.return_type(), expr);
                                                printer_ << fmt::format("var returnValue = {}", expr) + ";";
                                            }
                                            else
                                                printer_ << expr + ";";

                                            // ref parameters typemapped back to pInvoke
                                            for (const auto& param : child->children_)
                                            {
                                                const auto& type = param->Ast<cppast::cpp_function_parameter>().type();
                                                if (IsComplexOutputType(type))
                                                    printer_ << param->name_ + "_" + " = " + MapToPInvoke(type, param->name_ + "Out") + ";";
                                            }

                                            if (!IsVoid(func.return_type()))
                                                printer_ << "return returnValue;";
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
            PrintParameterHandlingCodePre(entity->children_);
            printer_ << fmt::format("var instance = {cFunctionName}({paramNameList});",
                FMT_CAPTURE(cFunctionName), FMT_CAPTURE(paramNameList));
            printer_ << "SetupInstance(instance);";
            PrintParameterHandlingCodePost(entity->children_);
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

        if (func.name() == "FreeScratchBuffer")
            int a = 2;

        auto rtype = ToCSType(func.return_type(), true);
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
            if (entity->symbolName_.find(entity->parent_->symbolName_) == 0)
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
            call = MapToCS(func.return_type(), call);

            if (!IsVoid(func.return_type()))
                call = "var returnValue = " + call;

            PrintParameterHandlingCodePre(entity->children_);

            printer_ << call + ";";

            PrintParameterHandlingCodePost(entity->children_);

            if (!IsVoid(func.return_type()))
                printer_ << "return returnValue;";
        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_function>();
        auto rtype = ToCSType(func.return_type(), true);
        auto csParams = FormatCSParameterList(entity->children_);

        printer_ << fmt::format("{access} static {rtype} {name}({csParams})",
            fmt::arg("access", entity->access_ == cppast::cpp_public ? "public" : "protected"), FMT_CAPTURE(rtype),
            fmt::arg("name", entity->name_), FMT_CAPTURE(csParams));

        auto paramNameList = MapParameterList(entity->children_, mapToPInvoke);

        // Body
        printer_.Indent();
        {
            std::string call = fmt::format("{cFunction}({paramNameList})",
                fmt::arg("cFunction", entity->cFunctionName_), FMT_CAPTURE(paramNameList));
            call = MapToCS(func.return_type(), call);

            if (!IsVoid(func.return_type()))
                call = "var returnValue = " + call;

            PrintParameterHandlingCodePre(entity->children_);

            printer_ << call + ";";

            PrintParameterHandlingCodePost(entity->children_);

            if (!IsVoid(func.return_type()))
                printer_ << "return returnValue;";
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
        auto csType = ToCSType(var.type(), true);
        auto name = entity->name_;
        auto sourceName = entity->sourceName_;
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
                    fmt::format("get_{nsSymbol}_{sourceName}()", FMT_CAPTURE(nsSymbol), FMT_CAPTURE(sourceName)));
                printer_ << fmt::format("get {{ return {call}; }}", fmt::arg("call", call));
                // Setter
                if (!IsConst(var.type()) && !(entity->flags_ & HintReadOnly))
                {
                    auto value = MapToPInvoke(var.type(), "value");
                    printer_ << fmt::format("set {{ set_{nsSymbol}_{sourceName}({value}); }}",
                        FMT_CAPTURE(nsSymbol), FMT_CAPTURE(sourceName), FMT_CAPTURE(value));
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
            auto csType = ToCSType(getterFunc.return_type(), true);

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
            auto csType = ToCSType(var.type(), true);

            auto name = entity->name_;
            auto sourceName = entity->sourceName_;
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
                        fmt::format("get_{nsSymbol}_{sourceName}(instance_)", FMT_CAPTURE(nsSymbol),
                            FMT_CAPTURE(sourceName)));
                    printer_ << fmt::format("get {{ return {}; }}", call);
                    // Setter
                    if (!IsConst(var.type()) && !(entity->flags_ & HintReadOnly))
                    {
                        auto value = MapToPInvoke(var.type(), "value");
                        printer_ << fmt::format("set {{ set_{nsSymbol}_{sourceName}(instance_, {value}); }}",
                            FMT_CAPTURE(nsSymbol), FMT_CAPTURE(sourceName), FMT_CAPTURE(value));
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
    String outputFile = generator->outputDirCs_ + "CSharp.cs";
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
    if (IsVoid(type))
        return expression;


    if (const auto* map = generator->GetTypeMap(type, false))
        return fmt::format(map->pInvokeToCSTemplate_.c_str(), fmt::arg("value", expression));
    else if (IsComplexType(type))
        return fmt::format("{}.__FromPInvoke({})", ToCSType(type), expression);

    return expression;
}

std::string GenerateCSApiPass::ToCSType(const cppast::cpp_type& type, bool disallowReferences)
{
    bool isRef = false;

    std::function<std::string(const cppast::cpp_type&)> toCSType = [&](const cppast::cpp_type& t) -> std::string {
        switch (t.kind())
        {
        case cppast::cpp_type_kind::builtin_t:
            return Urho3D::PrimitiveToPInvokeType(dynamic_cast<const cppast::cpp_builtin_type&>(t).builtin_type_kind());
        case cppast::cpp_type_kind::user_defined_t:
            return cppast::to_string(t);
        case cppast::cpp_type_kind::cv_qualified_t:
            return toCSType(dynamic_cast<const cppast::cpp_cv_qualified_type&>(t).type());
        case cppast::cpp_type_kind::pointer_t:
        case cppast::cpp_type_kind::reference_t:
        {
            const auto& pointee = cppast::remove_cv(
                t.kind() == cppast::cpp_type_kind::pointer_t ?
                dynamic_cast<const cppast::cpp_pointer_type&>(t).pointee() :
                dynamic_cast<const cppast::cpp_reference_type&>(t).referee());

            if (pointee.kind() == cppast::cpp_type_kind::builtin_t)
            {
                const auto& builtin = dynamic_cast<const cppast::cpp_builtin_type&>(pointee);
                if (builtin.builtin_type_kind() == cppast::cpp_builtin_type_kind::cpp_char)
                    return "string";
                if (t.kind() == cppast::cpp_type_kind::pointer_t)
                    return "IntPtr";
                isRef = true;
                return toCSType(pointee);
            }
            else if (pointee.kind() == cppast::cpp_type_kind::user_defined_t)
                return toCSType(pointee);
            else
            {
                isRef = true;
                return toCSType(pointee);
            }
        }
        case cppast::cpp_type_kind::template_instantiation_t:
        {
            const auto& tpl = dynamic_cast<const cppast::cpp_template_instantiation_type&>(t);
            auto tplName = tpl.primary_template().name();
            if (tplName == "SharedPtr" || tplName == "WeakPtr")
                return tpl.unexposed_arguments();
            assert(false);
        }
        default:
            assert(false);
        }
    };

    std::string typeName;
    if (auto* map = generator->GetTypeMap(type))
    {
        typeName = map->csType_;
        if (IsOutType(type))
        {
            isRef = true;
            typeName = typeName;
        }
    }
    else
        typeName = toCSType(type);

    if (!disallowReferences && isRef)
        typeName = "ref " + typeName;

    str::replace_str(typeName, "::", ".");
    return typeName;
}

std::string GenerateCSApiPass::MapToPInvoke(const cppast::cpp_type& type, const std::string& expression)
{
    if (const auto* map = generator->GetTypeMap(type, false))
        return fmt::format(map->csToPInvokeTemplate_.c_str(), fmt::arg("value", expression));
    else if (IsComplexType(type))
        return fmt::format("{}.__ToPInvoke({})", ToCSType(type, true), expression);

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
        if (IsOutType(cppType))
            defaultValue.clear();
        else
        {
            if (auto* map = generator->GetTypeMap(cppType, false))
            {
                // Value types are made nullable in order to allow default values.
                if (map->isValueType_ && !defaultValue.empty() && map->csType_ != "string")
                    csType += "?";
            }
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
    if (auto* map = generator->GetTypeMap(type, false))
    {
        if (map->csType_ == "string")
        {
            // String literals
            if (value == "String::EMPTY")  // TODO: move to json?
                value = "\"\"";
            return value;
        }
        else if (map->isValueType_ && !allowComplex)
        {
            // Value type parameters are turned to nullables when they have default values.
            return "null";
        }
    }

    if (!allowComplex && IsComplexType(type))
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

void GenerateCSApiPass::PrintParameterHandlingCodePre(const std::vector<SharedPtr<MetaEntity>>& parameters)
{
    for (const auto& param : parameters)
    {
        const auto& type = param->Ast<cppast::cpp_function_parameter>().type();
        if (IsComplexOutputType(type))
            printer_ << "var " + param->name_ + "Out = " + MapToPInvoke(type, param->name_) + ";";
    }
}

void GenerateCSApiPass::PrintParameterHandlingCodePost(const std::vector<SharedPtr<MetaEntity>>& parameters)
{
    for (const auto& param : parameters)
    {
        const auto& type = param->Ast<cppast::cpp_function_parameter>().type();
        if (IsComplexOutputType(type))
            printer_ << param->name_ + " = " + MapToCS(type, param->name_ + "Out") + ";";
    }
}

}
