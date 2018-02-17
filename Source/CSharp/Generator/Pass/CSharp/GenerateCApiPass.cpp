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

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/File.h>
#include <Declarations/Namespace.hpp>
#include <Declarations/Class.hpp>
#include <Declarations/Variable.hpp>
#include "GenerateCApiPass.h"


namespace Urho3D
{

void GenerateCApiPass::Start()
{
    generator_ = GetSubsystem<GeneratorContext>();
    typeMapper_ = &generator_->typeMapper_;

    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "#include \"ClassWrappers.hpp\"";
    printer_ << "";
    printer_ << "using namespace Urho3D;";
    printer_ << "";
    printer_ << "extern \"C\"";
    printer_ << "{";
    printer_ << "";
}

bool GenerateCApiPass::Visit(Declaration* decl, Event event)
{
    // Visit entities just once
    if (event == Event::EXIT || decl->source_ == nullptr)
        return true;

    auto toCType = [&](const cppast::cpp_type& type) { return typeMapper_->ToCType(type); };
    auto toCppType = [&](const cppast::cpp_function_parameter& param) {
        return typeMapper_->MapToCpp(param.type(), EnsureNotKeyword(param.name()));
    };

    if (decl->kind_ == Declaration::Kind::Class)
    {
        auto* cls = dynamic_cast<Class*>(decl);

        // Destructor always exists even if it is not defined in the class
        String cFunctionName = GetUniqueName(Sanitize(cls->symbolName_) + "_destructor");

        printer_ << fmt("URHO3D_EXPORT_API void {{c_function_name}}({{class_name}}* cls)", {
            {"c_function_name",     cFunctionName.CString()},
            {"class_name",          cls->name_.CString()},
        });
        printer_.Indent();
        {
            printer_ << "delete cls;";
        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (decl->kind_ == Declaration::Kind::Constructor || decl->kind_ == Declaration::Kind::Method)
    {
        Function* func = dynamic_cast<Function*>(decl);
        func->cFunctionName_ = GetUniqueName(Sanitize(func->symbolName_));

        String cParameterList = ParameterList(func->GetParameters(), toCType);
        auto vars = fmt({
            {"name",                func->name_.CString()},
            {"c_function_name",     func->cFunctionName_.CString()},
            {"parameter_name_list", ParameterNameList(func->GetParameters(), toCppType).CString()},
        });

        if (!func->isStatic_)
        {
            // If function wraps a class method then insert first parameter - a class pointer.
            if (func->kind_ != Declaration::Kind::Constructor)
            {
                if (!cParameterList.Empty())
                    cParameterList = ", " + cParameterList;
                cParameterList = func->parent_->sourceName_ + "* cls" + cParameterList;
            }
            vars.set("class_name", func->parent_->sourceName_.CString());
            vars.set("class_name_sanitized", Sanitize(func->parent_->sourceName_).CString());
        }
        vars.set("c_parameter_list", cParameterList.CString());

        // Constructor wrapper returns a pointer, but Declaration has a void return type for constructors.
        if (func->kind_ == Declaration::Kind::Constructor)
            vars.set("c_return_type", (func->name_ + "*").CString());
        else
            vars.set("c_return_type", typeMapper_->ToCType(func->GetReturnType()).CString());

        printer_ << fmt("URHO3D_EXPORT_API {{c_return_type}} {{c_function_name}}({{c_parameter_list}})", vars);
        printer_.Indent();
        {
            String call;
            if (func->kind_ == Declaration::Kind::Constructor)
                call = fmt("new {{class_name}}({{parameter_name_list}})", vars);
            else
            {
                call = fmt("{{name}}({{parameter_name_list}})", vars);

                if (!func->isStatic_)
                {
                    if (!func->isPublic_ && !func->IsVirtual())
                        call = "__public_" + call;
                    call = "cls->" + call;
                }
            }

            if (!IsVoid(func->GetReturnType()) || func->kind_ == Declaration::Kind::Constructor)
            {
                printer_.Write("return ");
                call = typeMapper_->MapToC(func->GetReturnType(), call, true);
            }

            printer_.Write(call);
            printer_.Write(";");
            printer_.Flush();
        }
        printer_.Dedent();
        printer_.WriteLine();

        if (func->IsVirtual())
        {
            printer_ << fmt("URHO3D_EXPORT_API void set_{{class_name_sanitized}}_fn{{name}}({{class_name}}* cls, void* fn)",
                vars);
            printer_.Indent();
            {
                printer_ << fmt("cls->fn{{name}} = (decltype(cls->fn{{name}}))fn;", vars);
            }
            printer_.Dedent();
            printer_.WriteLine();
        }
    }
    else if (decl->kind_ == Declaration::Kind::Variable)
    {
        auto* var = dynamic_cast<Variable*>(decl);
        auto* ns = dynamic_cast<Namespace*>(var->parent_.Get());

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if (var->isConstant_ && (!var->defaultValue_.Empty() || ns->kind_ == Declaration::Kind::Enum))
            return true;

        var->cFunctionName_ = Sanitize(ns->symbolName_ + "_" + var->name_);
        auto vars = fmt({{"c_type", typeMapper_->ToCType(var->GetType()).CString()},
                         {"c_function_name", var->cFunctionName_.CString()},
                         {"namespace_name",  ns->sourceName_.CString()},
                         {"name",            var->name_.CString()},
                         {"type",            GetConversionType(var->GetType()).CString()}
        });
        // Getter
        printer_.Write(fmt("URHO3D_EXPORT_API {{c_type}} get_{{c_function_name}}(", vars));
        if (!var->isStatic_)
            printer_.Write(fmt("{{namespace_name}}* cls", vars));
        printer_.Write(")");
        printer_.Indent();

        String expr;
        if (!var->isStatic_)
            expr += "cls->";
        else
            expr += ns->sourceName_ + "::";

        if (!decl->isPublic_)
            expr += fmt("__get_{{name}}()", vars).c_str();
        else
            expr += fmt("{{name}}", vars).c_str();

        // Variables are non-temporary therefore they do not need copying.
        printer_ << fmt("return {{mapped}};", {{"mapped", typeMapper_->MapToC(var->GetType(), expr, false).CString()}});

        printer_.Dedent();
        printer_.WriteLine();

        // Setter
        if (!var->isConstant_)
        {
            printer_.Write(fmt("URHO3D_EXPORT_API void set_{{c_function_name}}(", vars));
            if (!var->isStatic_)
                printer_.Write(fmt("{{namespace_name}}* cls, ", vars));
            printer_.Write(fmt("{{c_type}} value)", vars));
            printer_.Indent();

            vars.set("value", typeMapper_->MapToCpp(var->GetType(), "value").CString());
            if (!var->isStatic_)
                printer_.Write("cls->");
            else
                expr += ns->sourceName_ + "::";

            if (!decl->isPublic_)
                printer_.Write(fmt("__set_{{name}}({{value}});", vars));
            else
                printer_.Write(fmt("{{name}} = {{value}};", vars));

            printer_.Dedent();
            printer_.WriteLine();
        }
    }

    return true;
}

void GenerateCApiPass::Stop()
{
    printer_ << "}";    // Close extern "C"

    File file(context_, GetSubsystem<GeneratorContext>()->outputDir_ + "CApi.cpp", FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERROR("Failed saving CApi.cpp");
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

String GenerateCApiPass::GetUniqueName(const String& baseName)
{
    unsigned index = 0;
    String newName;
    for (newName = Sanitize(baseName); usedNames_.Contains(newName); index++)
        newName = baseName + String(index);

    usedNames_.Push(newName);
    return newName;
}

}
