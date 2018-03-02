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
            if (entity->ast_ == nullptr /* stinks */ || IsStatic(*entity->ast_))
            {
                // A class will not have any methods. This is likely a dummy class for constant storage or something
                // similar.
                printer_ << fmt("public static partial class {{class_name}}", {{"class_name", entity->name_}});
                printer_.Indent();
                return true;
            }

            bool hasBases = false;
            if (entity->ast_->kind() == cppast::cpp_entity_kind::class_t)
            {
                const auto& cls = entity->Ast<cppast::cpp_class>();

                for (const auto& base : cls.bases())
                {
                    if (const auto* baseClass = GetEntity(base.type()))
                    {
                        hasBases = true;
                        break;
                    }
                }
            }
            auto vars = fmt({
                {"class_name", entity->name_},
                {"has_bases", hasBases},
            });

            printer_ << fmt("public partial class {{class_name}} : IDisposable", vars);
            printer_.Indent();
            if (!hasBases)
            {
                printer_ << "internal IntPtr instance_;";
                printer_ << "protected volatile int disposed_;";
                printer_ << "";

                // Constructor that initializes form a instance
                printer_ << fmt("internal {{class_name}}(IntPtr instance)", vars);
                printer_.Indent();
                {
                    // Parent class may calls this constructor with null pointer when parent class constructor itself is
                    // creating instance.
                    printer_ << "if (instance != IntPtr.Zero)";
                    printer_.Indent();
                    {
                        printer_ << "instance_ = instance;";
                    }
                    printer_.Dedent();
                }
                printer_.Dedent();
                printer_ << "";
            }
            else
            {
                // Proxy constructor to one defined above
                printer_ << fmt("internal {{class_name}}(IntPtr instance) : base(instance) { }", vars);
                printer_ << "";
            }

            printer_ << fmt("public{{#has_bases}} new{{/has_bases}} void Dispose()", vars);
            printer_.Indent();
            {
                printer_ << "if (Interlocked.Increment(ref disposed_) == 1)";
                printer_.Indent();
                printer_ << fmt("InstanceCache.Remove<{{class_name}}>(instance_);", vars);
                if (entity->uniqueName_ == "Urho3D::Context")
                {
                    // When context is disposing we are still likely holding on to some objects. This causes a crash
                    // upon application exit, because references we are holding on will be disposed after Context no
                    // longer exists. If such references inherit from Object they will try to access Context and crash.
                    // In order to avoid this crash we explicitly dispose of all remaining native references just before
                    // destroying Context instance. It is critical that native objects are not interacted with after
                    // Context is destroyed. Creating and destroying multiple contexts will make things go boom as well.
                    // It is very likely that same thing will happen when trying to have multiple Context instances in
                    // the same process. This usage is very atypical, so do not do it.
                    printer_ << "InstanceCache.Dispose();";
                }
                printer_ << Sanitize(entity->uniqueName_) + "_destructor(instance_);";
                printer_.Dedent();
                printer_ << "instance_ = IntPtr.Zero;";
            }
            printer_.Dedent();
            printer_ << "";

            printer_ << fmt("~{{class_name}}()", vars);
            printer_.Indent();
            {
                printer_ << "Dispose();";
            }
            printer_.Dedent();
            printer_ << "";

            // Helpers for marshalling type between public and pinvoke APIs
            printer_ << fmt("internal{{#has_bases}} new{{/has_bases}} static {{class_name}} __FromPInvoke(IntPtr source)", vars);
            printer_.Indent();
            {
                printer_ << fmt("return InstanceCache.GetOrAdd<{{class_name}}>(source, ptr => new {{class_name}}(ptr));", vars);
            }
            printer_.Dedent();
            printer_ << "";

            printer_ << fmt("internal static IntPtr __ToPInvoke({{class_name}} source)", vars);
            printer_.Indent();
            {
                printer_ << fmt("return source.instance_;", vars);
            }
            printer_.Dedent();
            printer_ << "";

            // Destructor always exists even if it is not defined in the c++ class
            printer_ << dllImport;
            printer_ << fmt("internal static extern void {{symbol_name}}_destructor(IntPtr instance);", {
                {"symbol_name", Sanitize(entity->uniqueName_)}
            });
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

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if ((IsConst(var.type()) || entity->flags_ & HintReadOnly) && !entity->GetDefaultValue().empty())
            return true;

        // Getter
        printer_ << dllImport;
        String csReturnType = generator->typeMapper_.ToPInvokeTypeReturn(var.type());
        auto vars = fmt({
            {"cs_return", csReturnType.CString()},
            {"cs_param", generator->typeMapper_.ToPInvokeTypeParam(var.type())},
            {"c_function_name", entity->cFunctionName_},
        });
        if (csReturnType == "string")
        {
            // This is safe as member variables are always returned by copy from a getter.
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
        }
        printer_ << fmt("internal static extern {{cs_return}} get_{{c_function_name}}();", vars);
        printer_ << "";

        // Setter
        if (!IsConst(var.type()))
        {
            printer_ << dllImport;
            printer_
                << fmt("internal static extern void set_{{c_function_name}}({{cs_param}} value);", vars);
            printer_ << "";
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_member_variable>();

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if (IsConst(var.type()) && !entity->GetDefaultValue().empty())
            return true;

        // Getter
        printer_ << dllImport;
        String csReturnType = generator->typeMapper_.ToPInvokeTypeReturn(var.type());
        auto vars = fmt({
            {"cs_return", csReturnType.CString()},
            {"cs_param", generator->typeMapper_.ToPInvokeTypeParam(var.type())},
            {"c_function_name", entity->cFunctionName_},
        });
        if (csReturnType == "string")
        {
            // This is safe as member variables are always returned by copy from a getter.
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
        }
        printer_ << fmt("internal static extern {{cs_return}} get_{{c_function_name}}(IntPtr instance);", vars);
        printer_ << "";

        // Setter
        if (!IsConst(var.type()))
        {
            printer_ << dllImport;
            printer_
                << fmt("internal static extern void set_{{c_function_name}}(IntPtr instance, {{cs_param}} value);", vars);
            printer_ << "";
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& ctor = entity->Ast<cppast::cpp_constructor>();

        printer_ << dllImport;
        auto csParams = ParameterList(ctor.parameters(), std::bind(&TypeMapper::ToPInvokeTypeParam, &generator->typeMapper_, std::placeholders::_1));
        auto vars = fmt({
            {"c_function_name", entity->cFunctionName_},
            {"cs_param_list", csParams}
        });
        printer_ << fmt("internal static extern IntPtr {{c_function_name}}({{cs_param_list}});", vars);
        printer_ << "";
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_member_function>();

        printer_ << dllImport;
        auto csParams = ParameterList(func.parameters(),
            std::bind(&TypeMapper::ToPInvokeTypeParam, &generator->typeMapper_, std::placeholders::_1), nullptr);
        String csRetType = generator->typeMapper_.ToPInvokeTypeReturn(func.return_type());
        auto vars = fmt({
            {"c_function_name", entity->cFunctionName_},
            {"cs_param_list", csParams},
            {"cs_return", csRetType.CString()},
            {"has_params", !func.parameters().empty()},
            {"ret_attribute", ""},
            {"class_name", entity->parent_->name_},
            {"source_class_name", Sanitize(entity->parent_->sourceName_)},
            {"name", entity->name_},
        });
        if (csRetType == "string")
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
        printer_ << fmt("internal static extern {{cs_return}} {{c_function_name}}(IntPtr instance{{#has_params}}, {{cs_param_list}}{{/has_params}});", vars);
        printer_ << "";

        if (func.is_virtual())
        {
            // API for setting callbacks of virtual methods
            printer_ << "[UnmanagedFunctionPointer(CallingConvention.Cdecl)]";
            printer_ << fmt("internal delegate {{ret_attribute}}{{cs_return}} {{class_name}}{{name}}Delegate(IntPtr instance{{#has_params}}, {{cs_param_list}}{{/has_params}});", vars);
            printer_ << "";
            printer_ << dllImport;
            printer_ << fmt("internal static extern void set_{{source_class_name}}_fn{{c_function_name}}(IntPtr instance, {{class_name}}{{name}}Delegate cb);", vars);
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

}
