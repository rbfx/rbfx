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
#include "GenerateCSApiPass.h"

namespace Urho3D
{

void GenerateCSApiPass::Start()
{
    printer_ << "using System;";
    printer_ << "";
    printer_ << "namespace Urho3D";
    printer_ << "{";
    printer_ << "";
}

bool GenerateCSApiPass::Visit(const cppast::cpp_entity& e, cppast::visitor_info info)
{
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
        }
        else if (info.event == info.container_entity_exit)
        {
            printer_.Dedent();
            printer_ << "";
        }
    }
    else if (e.kind() == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& ctor = dynamic_cast<const cppast::cpp_constructor&>(e);
        const auto& cls = dynamic_cast<const cppast::cpp_class&>(e.parent().value());
        auto toCSParam = [&](const cppast::cpp_type& type) {
            const auto& map = generator->GetTypeMap(type);
            return map.csType;
        };
        auto getClassInstances = [&](const cppast::cpp_function_parameter& param) {
            if (generator->IsKnownType(param.type()))
                return param.name() + ".instance_";
            return param.name();
        };

        auto vars = fmt({
            {"class_name", e.parent().value().name()},
            {"cs_parameter_list", ParameterList(ctor.parameters(), toCSParam).CString()},
            {"symbol_name", Sanitize(GetSymbolName(e.parent().value())).CString()},
            {"param_name_list", ParameterNameList(ctor.parameters(), getClassInstances).CString()},
            {"has_base", !cls.bases().empty()}
        });

        // If class has a base class we call base constructor that does nothing. Class will be fully constructed here.
        printer_ << fmt("public {{class_name}}({{cs_parameter_list}}){{#has_base}} : base(IntPtr.Zero){{/has_base}}", vars);
        printer_.Indent();
        {
            printer_ << fmt("instance_ = construct_{{symbol_name}}({{param_name_list}});", vars);
        }
        printer_.Dedent();
    }

    return true;
}

void GenerateCSApiPass::Stop()
{
    printer_ << "}";    // namespace Urho3D

    auto* generator = GetSubsystem<GeneratorContext>();
    String outputFile = generator->GetOutputDir() + "Urho3D.cs";
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
