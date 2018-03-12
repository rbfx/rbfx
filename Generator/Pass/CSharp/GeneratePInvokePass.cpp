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
#include "GeneratePInvokePass.h"

namespace Urho3D
{

void GeneratePInvokePass::Start()
{
    printer_ << "using System;";
    printer_ << "using System.Threading;";
    printer_ << "using System.Collections.Concurrent;";
    printer_ << "using System.Runtime.InteropServices;";
    printer_ << "using CSharp;";
    printer_ << "";
}

bool GeneratePInvokePass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    const char* dllImport = "[DllImport(\"Urho3DCSharp\", CallingConvention = CallingConvention.Cdecl)]";

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

            printer_ << fmt::format("public unsafe partial class {} : IDisposable", entity->name_);
            printer_.Indent();
            printer_ << fmt::format("internal {}(IntPtr instance) : base(instance) {{ }}", entity->name_);
            printer_ << "";

            printer_ << fmt::format("public {}void Dispose()", newTag);
            printer_.Indent();
            {
                printer_ << "if (Interlocked.Increment(ref disposed_) == 1)";
                printer_.Indent();
                printer_ << baseName + "_destructor(instance_);";
                printer_ << fmt::format("InstanceCache.Remove<{}>(instance_, this);", entity->name_);
                printer_.Dedent();
                printer_ << "instance_ = IntPtr.Zero;";
            }
            printer_.Dedent();
            printer_ << "";

            printer_ << fmt::format("~{}()", entity->name_);
            printer_.Indent();
            {
                printer_ << "Dispose();";
            }
            printer_.Dedent();
            printer_ << "";

            // Helpers for marshalling type between public and pinvoke APIs
            printer_ << fmt::format("internal {}static {} __FromPInvoke(IntPtr source)", newTag, entity->name_);
            printer_.Indent();
            {
                printer_ << fmt::format("return InstanceCache.GetOrAdd<{className}>(source, ptr => new {className}(ptr));",
                    fmt::arg("className", entity->name_));
            }
            printer_.Dedent();
            printer_ << "";

            printer_ << fmt::format("internal static IntPtr __ToPInvoke({} source)", entity->name_);
            printer_.Indent();
            {
                printer_ << "if (source == null)";
                printer_.Indent();
                {
                    printer_ << "return IntPtr.Zero;";
                }
                printer_.Dedent();
                printer_ << "return source.instance_;";
            }
            printer_.Dedent();
            printer_ << "";

            // Destructor always exists even if it is not defined in the c++ class
            printer_ << dllImport;
            printer_ << fmt::format("internal static extern void {}_destructor(IntPtr instance);", baseName);
            printer_ << "";

            // Method for pinning managed object to native instance
            if (generator->inheritable_.IsIncluded(entity->uniqueName_))
            {
                printer_ << dllImport;
                printer_ << fmt::format("internal static extern void {}_setup(IntPtr instance, IntPtr gcHandle, "
                    "[param: MarshalAs(UnmanagedType.LPUTF8Str)]string typeName);", baseName);
                printer_ << "";
            }
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

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if ((IsConst(var.type()) || entity->flags_ & HintReadOnly) && !entity->GetDefaultValue().empty())
            return true;

        // Getter
        printer_ << dllImport;
        auto csParam = ToPInvokeTypeParam(var.type());
        auto csReturnType = ToPInvokeTypeReturn(var.type());
        if (csReturnType == "string")
        {
            // This is safe as member variables are always returned by copy from a getter.
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
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

        auto isFinal = !generator->inheritable_.IsIncluded(entity->parent_->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if (IsConst(var.type()) && !entity->GetDefaultValue().empty())
            return true;

        // Getter
        printer_ << dllImport;
        auto csReturnType = ToPInvokeTypeReturn(var.type());
        auto csParam = ToPInvokeTypeParam(var.type());
        if (csReturnType == "string")
        {
            // This is safe as member variables are always returned by copy from a getter.
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
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
            std::bind(&GeneratePInvokePass::ToPInvokeTypeParam, this, std::placeholders::_1));
        printer_ << fmt::format("internal static extern IntPtr {}({});", entity->cFunctionName_, csParams);
        printer_ << "";
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        auto isFinal = !generator->inheritable_.IsIncluded(entity->parent_->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        const auto& func = entity->Ast<cppast::cpp_member_function>();

        printer_ << dllImport;
        auto csParams = ParameterList(func.parameters(),
            std::bind(&GeneratePInvokePass::ToPInvokeTypeParam, this, std::placeholders::_1));
        auto rtype = ToPInvokeTypeReturn(func.return_type());
        auto cFunction = entity->cFunctionName_;
        auto className = entity->parent_->name_;
        auto sourceClassName = Sanitize(entity->parent_->sourceSymbolName_);
        auto uniqueName = Sanitize(entity->uniqueName_);
        auto pc = func.parameters().empty() ? "" : ", ";

        if (rtype == "string")
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
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
            printer_ << fmt::format("internal static extern void set_{sourceClassName}_fn{cFunction}(IntPtr instance, {className}{cFunction}Delegate cb);",
                FMT_CAPTURE(sourceClassName), FMT_CAPTURE(cFunction), FMT_CAPTURE(className));
            printer_ << "";
        }
    }
    return true;
}

void GeneratePInvokePass::Stop()
{
    String outputFile = generator->outputDirCs_ + "PInvoke.cs";
    File file(context_, outputFile, FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERRORF("Failed writing %s", outputFile.CString());
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

std::string GeneratePInvokePass::ToPInvokeTypeReturn(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(type, "IntPtr");
    return result;
}

std::string GeneratePInvokePass::ToPInvokeTypeParam(const cppast::cpp_type& type)
{
    std::string result = ToPInvokeType(type, "IntPtr");
    if (result == "string")
        return "[param: MarshalAs(UnmanagedType.LPUTF8Str)]" + result;
    return result;
}

}
