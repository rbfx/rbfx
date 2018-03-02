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

bool GenerateClassWrappers::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->ast_ == nullptr)
        return true;

    if (entity->ast_->kind() != cppast::cpp_entity_kind::class_t)
        return true;

    // Visit only once
    if (info.event == info.container_entity_exit)
        return true;

    // Class is not supposed to be inherited
    if (generator->final_.Contains(entity->uniqueName_))
        return true;

    const auto& cls = entity->Ast<cppast::cpp_class>();
    if (!HasVirtual(cls) && !HasProtected(cls))
    {
        // Skip children for classes that do not have virtual or protected members
        return info.event != info.container_entity_enter;
    }

    printer_ << fmt("class URHO3D_EXPORT_API {{name}} : public {{symbol}}", {
        {"name", entity->name_},
        {"symbol", entity->uniqueName_},
    });
    printer_.Indent();

    // Urho3D-specific
    if (IsSubclassOf(cls, "Urho3D::Object"))
    {
        printer_ << fmt("URHO3D_OBJECT({{name}}, {{symbol_name}});", {{"name",        entity->name_},
                                                                      {"symbol_name", entity->uniqueName_}});
    }

    printer_.WriteLine("public:", false);
    // Wrap constructors
    for (const auto& e : entity->children_)
    {
        if (e->kind_ == cppast::cpp_entity_kind::constructor_t)
        {
            const auto& ctor = e->Ast<cppast::cpp_constructor>();
            printer_ << fmt("{{name}}({{parameter_list}}) : {{symbol_name}}({{parameter_name_list}}) { }",
                {{"name",                entity->name_},
                 {"symbol_name",         entity->uniqueName_},
                 {"parameter_list",      ParameterList(ctor.parameters())},
                 {"parameter_name_list", ParameterNameList(ctor.parameters())},});
        }
    }
    printer_ << fmt("virtual ~{{name}}() = default;", {{"name", entity->name_}});

    std::vector<std::string> wrappedList;
    auto implementWrapperClassMembers = [&](const MetaEntity* cls)
    {
        for (const auto& child : cls->children_)
        {
            if (child->kind_ == cppast::cpp_entity_kind::member_variable_t && child->access_ == cppast::cpp_protected)
            {
                // Getters and setters for protected class variables.
                const auto& var = child->Ast<cppast::cpp_member_variable>();
                const auto& type = var.type();

                // Avoid returning non-builtin complex types as by copy
                bool wouldReturnByCopy =
                    type.kind() != cppast::cpp_type_kind::pointer_t &&
                    type.kind() != cppast::cpp_type_kind::reference_t &&
                    type.kind() != cppast::cpp_type_kind::builtin_t;

                auto vars = fmt({
                    {"name", child->name_},
                    {"type", cppast::to_string(type)},
                    {"ref", wouldReturnByCopy ? "&" : ""}
                });

                printer_ << fmt("{{type}}{{ref}} __get_{{name}}() { return {{name}}; }", vars);
                printer_ << fmt("void __set_{{name}}({{type}} value) { {{name}} = value; }", vars);
            }
            else if (child->kind_ == cppast::cpp_entity_kind::member_function_t)
            {
                const auto& func = child->Ast<cppast::cpp_member_function>();

                auto methodId = func.name() + func.signature();
                if (std::find(wrappedList.begin(), wrappedList.end(), methodId) == wrappedList.end())
                {
                    wrappedList.emplace_back(methodId);
                    // Function pointer that virtual method will call
                    const auto& cls = child->parent_;
                    auto vars = fmt({
                        {"type",                cppast::to_string(func.return_type())},
                        {"name",                child->name_},
                        {"class_name",          entity->name_},
                        {"full_class_name",     entity->uniqueName_},
                        {"parameter_list",      ParameterList(func.parameters())},
                        {"parameter_name_list", ParameterNameList(func.parameters())},
                        {"return",              IsVoid(func.return_type()) ? "" : "return"},
                        {"const",               cppast::is_const(func.cv_qualifier()) ? "const " : ""},
                        {"has_params",          Count(func.parameters()) > 0},
                        {"symbol_name",         Sanitize(child->uniqueName_)}
                    });
                    if (func.is_virtual())
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
                    else if (child->access_ == cppast::cpp_protected) // Protected virtuals are not exposed, no point
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

    std::function<void(MetaEntity*)> implementBaseWrapperClassMembers = [&](MetaEntity* cls)
    {
        const auto& astCls = cls->Ast<cppast::cpp_class>();
        for (const auto& base : astCls.bases())
        {
            if (base.access_specifier() == cppast::cpp_private)
                continue;

            auto* parentCls = GetEntity(base.type());
            if (parentCls != nullptr)
            {
                auto* baseOverlay = static_cast<MetaEntity*>(parentCls->user_data());
                implementWrapperClassMembers(baseOverlay);
                implementBaseWrapperClassMembers(baseOverlay);
            }
            else
                URHO3D_LOGWARNINGF("Base class %s not found!", base.name().c_str());
        }
    };

    implementWrapperClassMembers(entity);
    implementBaseWrapperClassMembers(entity);

    printer_.Dedent("};");
    printer_ << "";
    entity->sourceName_ = "Wrappers::" + entity->name_;    // Wrap a wrapper class
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
