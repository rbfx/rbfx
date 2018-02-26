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
#include <Urho3D/IO/Log.h>
#include "GenerateClassWrappers.h"
#include "GeneratorContext.h"
#include "Declarations/Class.hpp"
#include "Declarations/Variable.hpp"


namespace Urho3D
{

void GenerateClassWrappers::Start()
{
    printer_ << "#pragma once";
    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "";
    printer_ << "namespace Wrappers";
    printer_ << "{";
}

bool GenerateClassWrappers::Visit(Declaration* decl, Event event)
{
    if (decl->kind_ != Declaration::Kind::Class)
        return true;

    Class* cls = static_cast<Class*>(decl);
    if (!cls->HasVirtual() && !cls->HasProtected())
    {
        // Skip children for classes that do not have virtual or protected members
        return event != Event::ENTER;
    }

    // Visit only once
    if (event != Event::ENTER)
        return true;

    auto* generator = GetSubsystem<GeneratorContext>();
    printer_ << fmt("class URHO3D_EXPORT_API {{name}} : public {{symbol}}", {
        {"name", cls->name_},
        {"symbol", cls->symbolName_},
    });
    printer_.Indent();

    // Urho3D-specific
    if (cls->IsSubclassOf("Urho3D::Object"))
    {
        printer_ << fmt("URHO3D_OBJECT({{name}}, {{symbol_name}});", {{"name",        cls->name_},
                                                                      {"symbol_name", cls->symbolName_}});
    }

    printer_.WriteLine("public:", false);
    // Wrap constructors
    for (const auto& e : cls->children_)
    {
        if (e->kind_ == Declaration::Kind::Constructor)
        {
            const auto* ctor = dynamic_cast<const Function*>(e.Get());
            printer_ << fmt("{{name}}({{parameter_list}}) : {{symbol_name}}({{parameter_name_list}}) { }",
                {{"name",                cls->name_},
                 {"symbol_name",         cls->symbolName_},
                 {"parameter_list",      ParameterList(ctor->GetParameters())},
                 {"parameter_name_list", ParameterNameList(ctor->GetParameters())},});
        }
    }
    printer_ << fmt("virtual ~{{name}}() = default;", {{"name", cls->name_}});

    Vector<String> wrappedList;
    auto implementWrapperClassMembers = [&](const Class* cls)
    {
        for (const auto& child : cls->children_)
        {
            if (child->kind_ == Declaration::Kind::Variable && !child->isPublic_)
            {
                // Getters and setters for protected class variables.
                Variable* var = dynamic_cast<Variable*>(child.Get());
                const auto& type = var->GetType();

                // Avoid returning non-builtin complex types as by copy
                bool wouldReturnByCopy =
                    type.kind() != cppast::cpp_type_kind::pointer_t &&
                    type.kind() != cppast::cpp_type_kind::reference_t &&
                    type.kind() != cppast::cpp_type_kind::builtin_t;

                auto vars = fmt({
                    {"name", var->name_},
                    {"type", cppast::to_string(type)},
                    {"ref", wouldReturnByCopy ? "&" : ""}
                });

                printer_ << fmt("{{type}}{{ref}} __get_{{name}}() { return {{name}}; }", vars);
                printer_ << fmt("void __set_{{name}}({{type}} value) { {{name}} = value; }", vars);
            }
            else if (child->kind_ == Declaration::Kind::Method)
            {
                Function* func = dynamic_cast<Function*>(child.Get());

                String signature = func->name_ + dynamic_cast<const cppast::cpp_member_function*>(func->source_)->signature().c_str();
                if (!wrappedList.Contains(signature))
                {
                    wrappedList.Push(signature);
                    // Function pointer that virtual method will call
                    Class* cls = dynamic_cast<Class*>(func->parent_.Get());
                    auto vars = fmt({
                        {"type",                cppast::to_string(func->GetReturnType())},
                        {"name",                func->name_},
                        {"class_name",          cls->name_},
                        {"full_class_name",     cls->symbolName_},
                        {"parameter_list",      ParameterList(func->GetParameters())},
                        {"parameter_name_list", ParameterNameList(func->GetParameters())},
                        {"return",              IsVoid(func->GetReturnType()) ? "" : "return"},
                        {"const",               func->isConstant_ ? "const " : ""},
                        {"has_params",          !func->GetParameters().empty()},
                        {"symbol_name",         Sanitize(func->symbolName_)}
                    });
                    if (func->IsVirtual())
                    {
                        printer_ << fmt("{{type}}(*fn{{symbol_name}})({{class_name}} {{const}}*{{#has_params}}, {{/has_params}}{{parameter_list}}) = nullptr;", vars);
                        // Virtual method that calls said pointer
                        printer_ << fmt("{{type}} {{name}}({{parameter_list}}) {{const}}override", vars);
                        printer_.Indent();
                        {
                            printer_ << fmt("if (fn{{symbol_name}} == nullptr)", vars);
                            printer_.Indent();
                            {
                                printer_ << fmt("{{full_class_name}}::{{name}}({{parameter_name_list}});", vars);
                            }
                            printer_.Dedent();
                            printer_ << "else";
                            printer_.Indent();
                            {
                                printer_ << fmt("{{return}}(fn{{symbol_name}})(this{{#has_params}}, {{/has_params}}{{parameter_name_list}});", vars);
                            }
                            printer_.Dedent();

                        }
                        printer_.Dedent();
                    }
                    else if (!child->isPublic_) // Protected virtuals are not exposed, no point
                    {
                        printer_ << fmt("{{type}} __public_{{name}}({{parameter_list}})", vars);
                        printer_.Indent();
                        printer_ << fmt("{{name}}({{parameter_name_list}});", vars);
                        printer_.Dedent();
                    }
                }
            }
        }
    };

    std::function<void(Class*)> implementBaseWrapperClassMembers = [&](Class* cls)
    {
        const auto* astCls = dynamic_cast<const cppast::cpp_class*>(cls->source_);
        for (const auto& base : astCls->bases())
        {
            if (base.access_specifier() == cppast::cpp_private)
                continue;

            auto* parentCls = dynamic_cast<Class*>(generator->symbols_.Get(Urho3D::GetTypeName(base.type())));
            if (parentCls != nullptr)
            {
                implementWrapperClassMembers(parentCls);
                implementBaseWrapperClassMembers(parentCls);
            }
            else
                URHO3D_LOGWARNINGF("Base class %s not found!", base.name().c_str());
        }
    };

    implementWrapperClassMembers(cls);
    implementBaseWrapperClassMembers(cls);

    printer_.Dedent("};");
    printer_ << "";
    cls->sourceName_ = "Wrappers::" + cls->name_;    // Wrap a wrapper class
    return true;
}

void GenerateClassWrappers::Stop()
{
    printer_ << "}";    // namespace Wrappers

    File file(context_, GetSubsystem<GeneratorContext>()->outputDirCpp_ + "ClassWrappers.hpp", FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERROR("Failed saving ClassWrappers.hpp");
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

}
