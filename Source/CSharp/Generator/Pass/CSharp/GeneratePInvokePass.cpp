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
            // Cache managed objects. API will always return same object for existing native object pointer.
            printer_ << fmt("internal static {{#has_bases}}new {{/has_bases}}WeakDictionary<IntPtr, {{name}}> cache_ = new WeakDictionary<IntPtr, {{name}}>();", vars);
            printer_ << "";
            if (bases.Empty())
            {
                printer_ << "internal IntPtr handle_;";
                printer_ << "protected volatile int disposed_;";
                printer_ << "";

                // Constructor that initializes form a handle
                printer_ << fmt("internal {{name}}(IntPtr handle)", vars);
                printer_.Indent();
                {
                    // Parent class may calls this constructor with null pointer when parent class constructor itself is
                    // creating instance.
                    printer_ << "if (handle != IntPtr.Zero)";
                    printer_.Indent();
                        printer_ << "handle_ = handle;";
                    printer_.Dedent();
                }
                printer_.Dedent();
                printer_ << "";
            }
            else
            {
                // Proxy constructor to one defined above
                printer_ << fmt("internal {{name}}(IntPtr handle) : base(handle) { }", vars);
                printer_ << "";
            }

            printer_ << fmt("public{{#has_bases}} new{{/has_bases}} void Dispose()", vars);
            printer_.Indent();
            {
                printer_ << "if (Interlocked.Increment(ref disposed_) == 1)";
                printer_.Indent();
                printer_ << "cache_.Remove(handle_);";
                printer_ << Sanitize(cls->symbolName_) + "_destructor(handle_);";
                printer_.Dedent();
                printer_ << "handle_ = IntPtr.Zero;";
            }
            printer_.Dedent();
            printer_ << "";

            printer_ << fmt("~{{name}}()", vars);
            printer_.Indent();
            {
                printer_ << "Dispose();";
            }
            printer_.Dedent();
            printer_ << "";

            // Destructor always exists even if it is not defined in the c++ class
            printer_ << dllImport;
            printer_ << fmt("internal static extern void {{symbol_name}}_destructor(IntPtr handle);", {
                {"symbol_name", Sanitize(cls->symbolName_).CString()}
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
        String csReturnType = typeMapper_->ToPInvokeTypeReturn(var->GetType(), false);
        auto vars = fmt({
            {"cs_return", csReturnType.CString()},
            {"cs_param", typeMapper_->ToPInvokeTypeParam(var->GetType()).CString()},
            {"c_function_name", decl->cFunctionName_.CString()},
            {"not_static", !decl->isStatic_},
        });
        if (csReturnType == "string")
        {
            // This is safe as member variables are always returned by reference from a getter.
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
        }
        printer_ << fmt("internal static extern {{cs_return}} get_{{c_function_name}}({{#not_static}}IntPtr cls{{/not_static}});", vars);
        printer_ << "";

        // Setter
        if (!var->isConstant_)
        {
            printer_ << dllImport;
            printer_
                << fmt("internal static extern void set_{{c_function_name}}({{#not_static}}IntPtr cls, {{/not_static}}{{cs_param}} value);", vars);
            printer_ << "";
        }
    }
    else if (decl->kind_ == Declaration::Kind::Constructor)
    {
        Function* ctor = dynamic_cast<Function*>(decl);

        printer_ << dllImport;
        auto csParams = ParameterList(ctor->GetParameters(), std::bind(&TypeMapper::ToPInvokeTypeParam, typeMapper_, std::placeholders::_1));
        auto vars = fmt({
            {"c_function_name", decl->cFunctionName_.CString()},
            {"cs_param_list", csParams.CString()}
        });
        printer_ << fmt("internal static extern IntPtr {{c_function_name}}({{cs_param_list}});", vars);
        printer_ << "";
    }
    else if (decl->kind_ == Declaration::Kind::Method)
    {
        Function* func = dynamic_cast<Function*>(decl);

        printer_ << dllImport;
        auto csParams = ParameterList(func->GetParameters(), std::bind(&TypeMapper::ToPInvokeTypeParam, typeMapper_, std::placeholders::_1));
        String csRetType = typeMapper_->ToPInvokeTypeReturn(func->GetReturnType(), true);
        auto vars = fmt({
            {"c_function_name", decl->cFunctionName_.CString()},
            {"cs_param_list", csParams.CString()},
            {"cs_return", csRetType.CString()},
            {"has_params", !func->GetParameters().empty()},
            {"ret_attribute", ""},
            {"class_name", func->parent_->name_.CString()},
            {"name", func->name_.CString()},
        });
        if (csRetType == "string")
            printer_ << "[return: MarshalAs(UnmanagedType.LPUTF8Str)]";
        printer_ << fmt("internal static extern {{cs_return}} {{c_function_name}}(IntPtr handle{{#has_params}}, {{cs_param_list}}{{/has_params}});", vars);
        printer_ << "";

        if (func->IsVirtual())
        {
            // API for setting callbacks of virtual methods
            printer_ << "[UnmanagedFunctionPointer(CallingConvention.Cdecl)]";
            printer_ << fmt("internal delegate {{ret_attribute}}{{cs_return}} {{name}}Delegate(IntPtr handle{{#has_params}}, {{cs_param_list}}{{/has_params}});", vars);
            printer_ << "";
            printer_ << dllImport;
            printer_ << fmt("internal static extern void set_{{class_name}}_fn{{name}}(IntPtr handle, {{name}}Delegate cb);", vars);
            printer_ << "";
        }
    }
    return true;
}

void GeneratePInvokePass::Stop()
{
    printer_ << "}";    // namespace Urho3D

    auto* generator = GetSubsystem<GeneratorContext>();
    String outputFile = generator->outputDir_ + "PInvoke.cs";
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
