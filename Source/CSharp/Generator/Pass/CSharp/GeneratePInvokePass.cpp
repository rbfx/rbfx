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
#include <ThirdParty/cppast/include/cppast/cpp_member_variable.hpp>
#include <CppTypeInfo.h>
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
    printer_ << "";
    printer_ << "namespace Urho3D";
    printer_ << "{";
    printer_ << "";
}

bool GeneratePInvokePass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    const char* dllImport = "[DllImport(\"Urho3DCSharp\", CallingConvention = CallingConvention.Cdecl)]";
    auto data = GetUserData(e);
    auto* generator = GetSubsystem<GeneratorContext>();

    if (e.kind() == cppast::cpp_entity_kind::class_t)
    {
        const auto& cls = dynamic_cast<const cppast::cpp_class&>(e);

        if (info.event == info.container_entity_enter)
        {
            Vector<String> bases;
            for (const auto& base : cls.bases())
                bases.Push(base.name().c_str());

            auto vars = fmt({
                {"name", cls.name()},
                {"bases", String::Joined(bases, ", ").CString()},
                {"has_bases", !cls.bases().empty()},
            });

            printer_ << fmt("public partial class {{name}} : {{#has_bases}}{{bases}}, {{/has_bases}}IDisposable", vars);
            printer_.Indent();
            // Cache managed objects. API will always return same object for existing native object pointer.
            printer_ << fmt("internal static {{#has_bases}}new {{/has_bases}}ConcurrentDictionary<IntPtr, {{name}}> cache_ = new ConcurrentDictionary<IntPtr, {{name}}>();", vars);
            printer_ << "";
            if (bases.Empty())
            {
                printer_ << "internal IntPtr instance_;";
                printer_ << "protected volatile int disposed_;";
                printer_ << "";

                // Constructor that initializes form instance value
                printer_ << fmt("internal {{name}}(IntPtr instance)", vars);
                printer_.Indent();
                {
                    // Parent class may calls this constructor with null pointer when parent class constructor itself is
                    // creating instance.
                    printer_ << "if (instance != IntPtr.Zero)";
                    printer_.Indent();
                    {
                        printer_ << "instance_ = instance;";
                        if (generator->IsSubclassOf(cls, "Urho3D::RefCounted"))
                            printer_ << "Urho3D__RefCounted__AddRef(instance);";
                    }
                    printer_.Dedent();
                }
                printer_.Dedent();
                printer_ << "";
            }
            else
            {
                // Proxy constructor to one defined above
                printer_ << fmt("internal {{name}}(IntPtr instance) : base(instance) { }", vars);
                printer_ << "";
            }

            printer_ << fmt("public{{#has_bases}} new{{/has_bases}} void Dispose()", vars);
            printer_.Indent();
            {
                printer_ << "if (Interlocked.Increment(ref disposed_) == 1)";
                printer_.Indent();
                printer_ << "var self = this;";
                printer_ << "cache_.TryRemove(instance_, out self);";
                if (generator->IsSubclassOf(cls, "Urho3D::RefCounted"))
                    printer_ << "Urho3D__RefCounted__ReleaseRef(instance_);";
                else
                    printer_ << "destruct_" + Sanitize(GetSymbolName(cls)) + "(instance_);";
                printer_.Dedent();
                printer_ << "instance_ = IntPtr.Zero;";
            }
            printer_.Dedent();
            printer_ << "";

            // Destructor always exists even if it is not defined in the c++ class
            printer_ << dllImport;
            printer_ << fmt("internal static extern void destruct_{{symbol_name}}(IntPtr instance);", {
                {"symbol_name", Sanitize(GetSymbolName(cls)).CString()}
            });
            printer_ << "";
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = dynamic_cast<const cppast::cpp_member_variable&>(e);
        auto typeMap = generator->GetTypeMap(var.type());

        // Getter
        printer_ << dllImport;
        if (!typeMap.pInvokeAttribute.Empty())
            // This is safe as member variables are always returned by reference from a getter.
            printer_ << "[return: MarshalAs(" + typeMap.pInvokeAttribute + ")]";

        auto vars = fmt({
            {"cs_type", typeMap.GetPInvokeType().CString()},
            {"cs_ret_type", typeMap.GetPInvokeType(true).CString()},
            {"c_function_name", data->cFunctionName.CString()},
        });
        printer_ << fmt("internal static extern {{cs_ret_type}} get_{{c_function_name}}(IntPtr cls);", vars);
        printer_ << "";

        // Setter
        printer_ << dllImport;
        printer_ << fmt("internal static extern void set_{{c_function_name}}(IntPtr cls, {{cs_type}} value);", vars);
        printer_ << "";
    }
    else if (e.kind() == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& ctor = dynamic_cast<const cppast::cpp_constructor&>(e);

        printer_ << dllImport;
        auto csParams = ParameterList(ctor.parameters(), [&](const cppast::cpp_type& type) {
            return generator->GetTypeMap(type).GetPInvokeType();
        });
        auto vars = fmt({
            {"c_function_name", data->cFunctionName.CString()},
            {"cs_param_list", csParams.CString()}
        });
        printer_ << fmt("internal static extern IntPtr {{c_function_name}}({{cs_param_list}});", vars);
        printer_ << "";
    }
    else if (e.kind() == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);
        printer_ << dllImport;
        auto csParams = ParameterList(func.parameters(), [&](const cppast::cpp_type& type) {
            return generator->GetTypeMap(type).GetPInvokeType();
        });

        String csRetType = "IntPtr";
        if (!IsVoid(func.return_type()))
            csRetType = generator->GetTypeMap(func.return_type()).GetPInvokeType(true);

        auto vars = fmt({
            {"c_function_name", data->cFunctionName.CString()},
            {"cs_param_list", csParams.CString()},
            {"cs_return", csRetType.CString()},
            {"has_params", !func.parameters().empty()}
        });
        printer_ << fmt("internal static extern {{cs_return}} {{c_function_name}}(IntPtr instance{{#has_params}}, {{cs_param_list}}{{/has_params}});", vars);
        printer_ << "";

    }
    return true;
}

void GeneratePInvokePass::Stop()
{
    printer_ << "}";    // namespace Urho3D

    auto* generator = GetSubsystem<GeneratorContext>();
    String outputFile = generator->GetOutputDir() + "PInvoke.cs";
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
