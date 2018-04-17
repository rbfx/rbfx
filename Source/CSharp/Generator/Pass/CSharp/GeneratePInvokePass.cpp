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
#include <fstream>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_function_type.hpp>
#include "GeneratorContext.h"
#include "GeneratePInvokePass.h"
#include "ImplementInterfacesPass.h"


namespace Urho3D
{

void GeneratePInvokePass::Start()
{
    printer_ << "using System;";
    printer_ << "using System.Threading;";
    printer_ << "using System.Collections.Concurrent;";
    printer_ << "using System.Reflection;";
    printer_ << "using System.Runtime.CompilerServices;";
    printer_ << "using System.Runtime.InteropServices;";
    printer_ << "using Urho3D.CSharp;";
    printer_ << "";

    discoverInterfacesPass_ = generator->GetPass<DiscoverInterfacesPass>();
}

bool GeneratePInvokePass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    const char* dllImport = "[MethodImplAttribute(MethodImplOptions.InternalCall)]";

    // Generate C API for property getters and seters. Visitor will not visit these notes on it's own.
    if (entity->flags_ & HintProperty)
    {
        for (const auto& child : entity->children_)
            Visit(child.get(), info);
        return true;
    }

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
            if (entity->ast_ == nullptr /* stinks */ || IsStatic(*entity->ast_))
            {
                // A class will not have any methods. This is likely a dummy class for constant storage or something
                // similar.
                printer_ << fmt::format("public static partial class {}", entity->name_);
                printer_.Indent();
                return true;
            }

            bool hasBases = false;
            if (entity->ast_->kind() == cppast::cpp_entity_kind::class_t)
            {
                const auto& cls = entity->Ast<cppast::cpp_class>();

                for (const auto& base : cls.bases())
                {
                    if (GetEntity(base.type()) != nullptr)
                    {
                        hasBases = true;
                        break;
                    }
                }
            }
            auto newTag = hasBases ? "new " : " ";
            auto baseName = Sanitize(entity->uniqueName_);

            printer_ << fmt::format("public unsafe partial class {} : INativeObject", entity->name_);
            printer_.Indent();
            printer_ << fmt::format("internal {}(IntPtr instance, bool ownsInstance) : base(instance, ownsInstance)", entity->name_);
            printer_.Indent();
            {
                if (entity->symbolName_ == "Urho3D::RefCounted")
                {
                    // If instance is null then managed side is initiating object construction and will call AddRef
                    // after SetupInstance().
                    printer_ << "if (instance != IntPtr.Zero)";
                    printer_.Indent("");
                    {
                        printer_ << "AddRef();";
                    }
                    printer_.Dedent("");
                }
            }
            printer_.Dedent();
            printer_ << "";

            printer_ << fmt::format("public override void Dispose()");
            printer_.Indent();
            {
                printer_ << "if (Interlocked.Increment(ref DisposedCounter) == 1)";
                printer_.Indent();
                printer_ << "InstanceCache.Remove(NativeInstance);";
                printer_ << baseName + "_destructor(NativeInstance, OwnsNativeInstance);";
                printer_.Dedent();
                printer_ << "NativeInstance = IntPtr.Zero;";
            }
            printer_.Dedent();
            printer_ << "";

            // Helpers for marshalling type between public and pinvoke APIs
            printer_ << fmt::format("internal {}static {} __FromPInvoke(IntPtr source, bool owns)", newTag, entity->name_);
            printer_.Indent();
            {
                printer_ << "if (source == IntPtr.Zero)";
                printer_.Indent("");
                {
                    printer_ << "return null;";
                }
                printer_.Dedent("");

                printer_ << "return InstanceCache.GetOrAdd(source, ptr =>";
                printer_.Indent();
                {
                    printer_ << "var type = InstanceCache.GetNativeType(GetNativeTypeId(ptr));";
                    printer_ << "if (type == null)";
                    printer_.Indent("");
                    {
                        printer_ << fmt::format("return new {className}(ptr, owns);", fmt::arg("className", entity->name_));
                    }
                    printer_.Dedent("");
                    printer_ << "else";
                    printer_.Indent("");
                    {
                        printer_ << fmt::format("return ({className})Activator.CreateInstance(type, BindingFlags.NonPublic | BindingFlags.Instance, null, new object[]{{ptr, owns}}, null);",
                            fmt::arg("className", entity->name_));
                    }
                    printer_.Dedent("");
                }
                printer_.Dedent("});");
            }
            printer_.Dedent();
            printer_ << "";

            // Offsets for multiple inheritance
            auto it = discoverInterfacesPass_->inheritedBy_.find(entity->symbolName_);
            if (it != discoverInterfacesPass_->inheritedBy_.end())
            {
                for (const auto& inheritorName : it->second)
                {
                    auto* inheritor = generator->GetSymbol(inheritorName);
                    if (inheritor == nullptr)
                        continue;

                    auto* base = generator->GetSymbol(it->first);
                    if (base == nullptr)
                        continue;

                    auto baseSym = Sanitize(it->first);
                    auto derivedSym = Sanitize(inheritorName);

                    printer_ << dllImport;
                    printer_ << fmt::format("internal static extern int {derivedSym}_{baseSym}_offset();",
                        FMT_CAPTURE(derivedSym), FMT_CAPTURE(baseSym));
                    printer_ << fmt::format("static int {derivedSym}_offset = {derivedSym}_{baseSym}_offset();",
                        FMT_CAPTURE(derivedSym), FMT_CAPTURE(baseSym));
                    printer_ << "";
                }
            }
            //////////////////////////////////

            printer_ << fmt::format("internal static IntPtr __ToPInvoke({} source)",
                ((entity->flags_ & HintInterface) ? "I" : "") + entity->name_);
            printer_.Indent();
            {
                printer_ << "if (source == null)";
                printer_.Indent("");
                {
                    printer_ << "return IntPtr.Zero;";
                }
                printer_.Dedent("");

                // Offsets for multiple inheritance
                if (it != discoverInterfacesPass_->inheritedBy_.end())
                {
                    for (const auto& inheritorName : it->second)
                    {
                        auto* inheritor = generator->GetSymbol(inheritorName);
                        if (inheritor == nullptr)
                            continue;

                        auto* base = generator->GetSymbol(it->first);
                        if (base == nullptr)
                            continue;

                        auto baseSym = Sanitize(base->symbolName_);
                        auto derivedSym = Sanitize(inheritorName);
                        auto derivedName = inheritorName;
                        str::replace_str(derivedName, "::", ".");

                        printer_ << fmt::format("if (source is {derivedName})", FMT_CAPTURE(derivedName));
                        printer_.Indent();
                        {
                            printer_ << fmt::format("return source.NativeInstance + {derivedSym}_offset;",
                                FMT_CAPTURE(derivedSym));
                        }
                        printer_.Dedent();
                    }
                }
                ///////////////////////////////////

                printer_ << "return source.NativeInstance;";
            }
            printer_.Dedent();
            printer_ << "";

            // Destructor always exists even if it is not defined in the c++ class
            printer_ << dllImport;
            printer_ << fmt::format("internal static extern void {}_destructor(IntPtr instance, bool owner);", baseName);
            printer_ << "";

            // Method for pinning managed object to native instance
            if (generator->inheritable_.IsIncluded(entity->uniqueName_) ||
                IsSubclassOf(entity->Ast<cppast::cpp_class>(), "Urho3D::RefCounted"))
            {
                printer_ << dllImport;
                printer_ << fmt::format("internal static extern void {}_setup(IntPtr instance, IntPtr gcHandle, "
                    "string typeName);", baseName);
                printer_ << "";
            }

            // Method for getting type id.
            printer_ << dllImport;
            printer_ << fmt::format("private static extern IntPtr {}_typeid();", baseName);
            printer_ << fmt::format("internal static IntPtr GetNativeTypeId() {{ return {}_typeid(); }}", baseName);
            printer_ << "";

            printer_ << dllImport;
            printer_ << fmt::format("private static extern IntPtr {}_instance_typeid(IntPtr instance);", baseName);
            printer_ << fmt::format("internal static IntPtr GetNativeTypeId(IntPtr instance) {{ return {}_instance_typeid(instance); }}", baseName);
            printer_ << "";
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }

    if (info.event == info.container_entity_exit)
        return true;

    if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_variable>();

        // Constants with values get converted to native c# constants in GenerateCSharpApiPass
        if ((IsConst(var.type()) || entity->flags_ & HintReadOnly) && !entity->GetDefaultValue().empty())
            return true;

        // Getter
        printer_ << dllImport;
        auto csParam = ToPInvokeTypeParam(var.type());
        auto csReturnType = ToPInvokeTypeReturn(var.type());
        if (auto* map = generator->GetTypeMap(var.type()))
        {
            if (!map->marshalAttribute_.empty())
                printer_ << fmt::format("[return: {}]", map->marshalAttribute_);
        }
        printer_ << fmt::format("internal static extern {} get_{}();", csReturnType, entity->cFunctionName_);
        printer_ << "";

        // Setter
        if (!IsConst(var.type()))
        {
            printer_ << dllImport;
            printer_ << fmt::format("internal static extern void set_{}({} value);", entity->cFunctionName_, csParam);
            printer_ << "";
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_member_variable>();

        auto isFinal = !generator->inheritable_.IsIncluded(entity->GetParent()->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        // Constants with values get converted to native c# constants in GenerateCSharpApiPass
        if (IsConst(var.type()) && !entity->GetDefaultValue().empty())
            return true;

        // Getter
        printer_ << dllImport;
        auto csReturnType = ToPInvokeTypeReturn(var.type());
        auto csParam = ToPInvokeTypeParam(var.type());
        if (auto* map = generator->GetTypeMap(var.type()))
        {
            if (!map->marshalAttribute_.empty())
                printer_ << fmt::format("[return: {}]", map->marshalAttribute_);
        }
        printer_ << fmt::format("internal static extern {} get_{}(IntPtr instance);",
            csReturnType, entity->cFunctionName_);
        printer_ << "";

        // Setter
        if (!IsConst(var.type()))
        {
            printer_ << dllImport;
            printer_ << fmt::format("internal static extern void set_{}(IntPtr instance, {} value);",
                entity->cFunctionName_, csParam);
            printer_ << "";
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& ctor = entity->Ast<cppast::cpp_constructor>();
        printer_ << dllImport;
        auto csParams = ParameterList(ctor.parameters(),
            std::bind(&GeneratePInvokePass::ToPInvokeTypeParam, std::placeholders::_1));
        printer_ << fmt::format("internal static extern IntPtr {}({});", entity->cFunctionName_, csParams);
        printer_ << "";
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        auto isFinal = !generator->inheritable_.IsIncluded(entity->GetParent()->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        const auto& func = entity->Ast<cppast::cpp_member_function>();

        printer_ << dllImport;
        auto csParams = ParameterList(func.parameters(),
            std::bind(&GeneratePInvokePass::ToPInvokeTypeParam, std::placeholders::_1));
        auto rtype = ToPInvokeTypeReturn(func.return_type());
        auto cFunction = entity->cFunctionName_;
        auto className = entity->GetParent()->name_;
        auto sourceClassName = Sanitize(entity->GetParent()->sourceSymbolName_);
        auto uniqueName = Sanitize(entity->uniqueName_);
        auto pc = func.parameters().empty() ? "" : ", ";

        if (auto* map = generator->GetTypeMap(func.return_type()))
        {
            if (!map->marshalAttribute_.empty())
                printer_ << fmt::format("[return: {}]", map->marshalAttribute_);
        }
        printer_ << fmt::format("internal static extern {rtype} {cFunction}(IntPtr instance{pc}{csParams});",
            FMT_CAPTURE(rtype), FMT_CAPTURE(cFunction), FMT_CAPTURE(pc), FMT_CAPTURE(csParams));
        printer_ << "";

        if (func.is_virtual())
        {
            // API for setting callbacks of virtual methods
            printer_ << "[UnmanagedFunctionPointer(CallingConvention.Cdecl)]";
            printer_ << fmt::format("internal delegate {rtype} {className}{cFunction}Delegate(IntPtr instance{pc}{csParams});",
                FMT_CAPTURE(rtype), FMT_CAPTURE(className), FMT_CAPTURE(cFunction), FMT_CAPTURE(pc), FMT_CAPTURE(csParams));
            printer_ << "";
            printer_ << dllImport;
            printer_ << fmt::format("internal static extern void set_fn{cFunction}(IntPtr instance, IntPtr cb);",
                FMT_CAPTURE(cFunction), FMT_CAPTURE(className));
            printer_ << "";
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_function>();

        printer_ << dllImport;
        auto csParams = ParameterList(func.parameters(),
            std::bind(&GeneratePInvokePass::ToPInvokeTypeParam, std::placeholders::_1));
        auto rtype = ToPInvokeTypeReturn(func.return_type());
        auto cFunction = entity->cFunctionName_;

        if (auto* map = generator->GetTypeMap(func.return_type()))
        {
            if (!map->marshalAttribute_.empty())
                printer_ << fmt::format("[return: {}]", map->marshalAttribute_);
        }
        printer_ << fmt::format("internal static extern {rtype} {cFunction}({csParams});",
            FMT_CAPTURE(rtype), FMT_CAPTURE(cFunction), FMT_CAPTURE(csParams));
        printer_ << "";
    }

    return true;
}

void GeneratePInvokePass::Stop()
{
    auto outputFile = generator->outputDirCs_ + "PInvoke.cs";
    std::ofstream fp(outputFile);
    if (!fp.is_open())
    {
        spdlog::get("console")->error("Failed writing {}", outputFile);
        return;
    }
    fp << printer_.Get();
}

std::string GeneratePInvokePass::ToPInvokeTypeReturn(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(type, true);
    return result;
}

std::string GeneratePInvokePass::ToPInvokeTypeParam(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(type);

    if (auto* map = generator->GetTypeMap(type))
    {
        if (!map->marshalAttribute_.empty())
            result = fmt::format("[param: {}]", map->marshalAttribute_) + result;
    }

    return result;
}

std::string GeneratePInvokePass::ToPInvokeType(const cppast::cpp_type& type, bool disallowReferences)
{
    std::function<std::string(const cppast::cpp_type&)> toPInvokeType = [&](const cppast::cpp_type& t) -> std::string {
        switch (t.kind())
        {
        case cppast::cpp_type_kind::builtin_t:
            return Urho3D::PrimitiveToPInvokeType(dynamic_cast<const cppast::cpp_builtin_type&>(t).builtin_type_kind());
        case cppast::cpp_type_kind::user_defined_t:
            if (IsEnumType(t))
                return cppast::to_string(t);
            // In case this is complex object returned by value always treat it as a pointer.
            return "IntPtr";
        case cppast::cpp_type_kind::cv_qualified_t:
            return toPInvokeType(dynamic_cast<const cppast::cpp_cv_qualified_type&>(t).type());
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
                auto typeName = toPInvokeType(pointee);
                if (!disallowReferences)
                    typeName = "ref " + typeName;
                return typeName;
            }
            else if (pointee.kind() == cppast::cpp_type_kind::user_defined_t && !IsEnumType(t))
                return "IntPtr";

            return "IntPtr";
        }
        case cppast::cpp_type_kind::template_instantiation_t:
        {
            const auto& tpl = dynamic_cast<const cppast::cpp_template_instantiation_type&>(t);
            auto tplName = tpl.primary_template().name();
            if (tplName == "SharedPtr" || tplName == "WeakPtr")
                return "IntPtr";
            assert(false);
        }
        default:
            assert(false);
        }
        return "";
    };

    std::string typeName;
    if (auto* map = generator->GetTypeMap(type))
    {
        typeName = map->pInvokeType_;
        if (!disallowReferences && IsOutType(type))
            typeName = "ref " + typeName;
    }
    else
        typeName = toPInvokeType(type);

    str::replace_str(typeName, "::", ".");
    return typeName;
}

}
