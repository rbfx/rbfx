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
#include "GeneratePInvokePass.h"

namespace Urho3D
{

void GeneratePInvokePass::Start()
{
    generator_ = GetSubsystem<GeneratorContext>();
    typeMapper_ = &generator_->typeMapper_;

    printer_ << "using System;";
    printer_ << "using System.Threading;";
    printer_ << "using System.Collections.Concurrent;";
    printer_ << "using System.Runtime.InteropServices;";
    printer_ << "using CSharp;";
    printer_ << "";
    printer_ << "namespace Urho3D";
    printer_ << "{";
    printer_ << "";
}

bool GeneratePInvokePass::Visit(Declaration* decl, Event event)
{
    const char* dllImport = "[DllImport(\"Urho3DCSharp\", CallingConvention = CallingConvention.Cdecl)]";

    if (decl->kind_ == Declaration::Kind::Class)
    {
        Class* cls = dynamic_cast<Class*>(decl);

        if (event == Event::ENTER)
        {
            if (cls->isStatic_)
            {
                // A class will not have any methods. This is likely a dummy class for constant storage or something
                // similar.
                printer_ << fmt("public static partial class {{class_name}}", {{"class_name", cls->name_}});
                printer_.Indent();
                return true;
            }
            Vector<String> bases;
            for (const auto& base : cls->bases_)
                bases.Push(base->name_);

            auto vars = fmt({
                {"class_name", cls->name_},
                {"bases", String::Joined(bases, ", ").CString()},
                {"has_bases", !cls->bases_.Empty()},
            });

            printer_ << fmt("public partial class {{class_name}} : {{#has_bases}}{{bases}}, {{/has_bases}}IDisposable", vars);
            printer_.Indent();
            if (bases.Empty())
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
                        printer_ << "instance_ = instance;";
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
                if (cls->symbolName_ == "Urho3D::Context")
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
                printer_ << Sanitize(cls->symbolName_) + "_destructor(instance_);";
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
                {"symbol_name", Sanitize(cls->symbolName_)}
            });
            printer_ << "";
        }
        else if (event == Event::EXIT)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }
    else if (decl->kind_ == Declaration::Kind::Variable)
    {
        Variable* var = dynamic_cast<Variable*>(decl);

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if (var->isConstant_ && (!var->defaultValue_.Empty() || var->parent_->kind_ == Declaration::Kind::Enum))
            return true;

        // Getter
        printer_ << dllImport;
        String csReturnType = typeMapper_->ToPInvokeTypeReturn(var->GetType());
        auto vars = fmt({
            {"cs_return", csReturnType.CString()},
            {"cs_param", typeMapper_->ToPInvokeTypeParam(var->GetType())},
            {"c_function_name", decl->cFunctionName_},
            {"not_static", !decl->isStatic_},
        });
        if (csReturnType == "string")
        {
            // This is safe as member variables are always returned by copy from a getter.
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
        }
        printer_ << fmt("internal static extern {{cs_return}} get_{{c_function_name}}({{#not_static}}IntPtr instance{{/not_static}});", vars);
        printer_ << "";

        // Setter
        if (!var->isConstant_)
        {
            printer_ << dllImport;
            printer_
                << fmt("internal static extern void set_{{c_function_name}}({{#not_static}}IntPtr instance, {{/not_static}}{{cs_param}} value);", vars);
            printer_ << "";
        }
    }
    else if (decl->kind_ == Declaration::Kind::Constructor)
    {
        Function* ctor = dynamic_cast<Function*>(decl);

        printer_ << dllImport;
        auto csParams = ParameterList(ctor->parameters_, std::bind(&TypeMapper::ToPInvokeTypeParam, typeMapper_, std::placeholders::_1));
        auto vars = fmt({
            {"c_function_name", decl->cFunctionName_},
            {"cs_param_list", csParams}
        });
        printer_ << fmt("internal static extern IntPtr {{c_function_name}}({{cs_param_list}});", vars);
        printer_ << "";
    }
    else if (decl->kind_ == Declaration::Kind::Method)
    {
        Function* func = dynamic_cast<Function*>(decl);

        printer_ << dllImport;
        auto csParams = ParameterList(func->parameters_,
            std::bind(&TypeMapper::ToPInvokeTypeParam, typeMapper_, std::placeholders::_1), nullptr);
        String csRetType = typeMapper_->ToPInvokeTypeReturn(*func->returnType_);
        auto vars = fmt({
            {"c_function_name", decl->cFunctionName_},
            {"cs_param_list", csParams},
            {"cs_return", csRetType.CString()},
            {"has_params", !func->parameters_.empty()},
            {"ret_attribute", ""},
            {"class_name", func->parent_->name_},
            {"source_class_name", Sanitize(func->parent_->sourceName_)},
            {"name", func->name_},
        });
        if (csRetType == "string")
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
        printer_ << fmt("internal static extern {{cs_return}} {{c_function_name}}(IntPtr instance{{#has_params}}, {{cs_param_list}}{{/has_params}});", vars);
        printer_ << "";

        if (func->IsVirtual())
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
    printer_ << "}";    // namespace Urho3D

    auto* generator = GetSubsystem<GeneratorContext>();
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
