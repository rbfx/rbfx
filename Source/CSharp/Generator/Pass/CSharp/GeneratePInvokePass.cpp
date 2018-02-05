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
#include "GeneratorContext.h"
#include "GeneratePInvokePass.h"

namespace Urho3D
{

void GeneratePInvokePass::Start()
{
    printer_ << "using System;";
    printer_ << "using System.Runtime.InteropServices;";
    printer_ << "";
    printer_ << "namespace Urho3D";
    printer_ << "{";
    printer_ << "";
}

bool GeneratePInvokePass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
    if (e.kind() == cppast::cpp_entity_kind::class_t)
    {
        const auto& cls = dynamic_cast<const cppast::cpp_class&>(e);

        if (info.event == info.container_entity_enter)
        {
            Vector<String> bases;
            for (const auto& base : cls.bases())
                bases.Push(base.name().c_str());

            printer_ << fmt("public partial class {{name}}{{#has_bases}} : {{bases}}{{/has_bases}}", {
                {"name", cls.name()},
                {"bases", String::Joined(bases, ", ").CString()},
                {"has_bases", !cls.bases().empty()},
            });
            printer_.Indent();
            if (bases.Empty())
                printer_ << "internal IntPtr instance_;";
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
        auto* generator = GetSubsystem<GeneratorContext>();
        auto typeMap = generator->GetTypeMap(var.type());
        printer_ << "[DllImport(\"Urho3DCSharp\", CallingConvention = CallingConvention.Cdecl)]";
        if (!typeMap.pInvokeAttribute.Empty())
            printer_ << "[return: MarshalAs(" + typeMap.pInvokeAttribute + ")]";

        printer_ << fmt("internal static extern {{c_type}} get_{{c_function_name}}(IntPtr cls);", {
            {"c_type", generator->GetCSType(var.type()).CString()},
            {"c_function_name", GetUserData(e)->cFunctionName.CString()},
        });
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
