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
#include "GenerateCApiPass.h"


namespace Urho3D
{

void GenerateCApiPass::Start()
{
    generator_ = GetSubsystem<GeneratorContext>();
    typeMapper_ = &generator_->typeMapper_;

    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "#include \"CSharp.h\"";
    printer_ << "#include \"ClassWrappers.hpp\"";
    printer_ << "";
    printer_ << "using namespace Urho3D;";
    printer_ << "";
    printer_ << "extern \"C\"";
    printer_ << "{";
    printer_ << "";
}

bool GenerateCApiPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    // Visit entities just once
    if (info.event == info.container_entity_exit || entity->ast_ == nullptr || entity->name_.empty())
        return true;

    auto toCType = [&](const cppast::cpp_type& type) { return typeMapper_->ToCType(type); };
    auto toCppType = [&](const cppast::cpp_function_parameter& param) {
        return typeMapper_->MapToCpp(param.type(), EnsureNotKeyword(param.name()));
    };

    if (entity->kind_ == cppast::cpp_entity_kind::class_t)
    {
        if (IsStatic(*entity->ast_))
            return true;

        // Destructor always exists even if it is not defined in the class
        String cFunctionName = GetUniqueName(Sanitize(entity->uniqueName_) + "_destructor");

        printer_ << fmt("URHO3D_EXPORT_API void {{c_function_name}}({{class_name}}* instance)", {
            {"c_function_name",     cFunctionName.CString()},
            {"class_name",          entity->name_},
        });
        printer_.Indent();
        {
            printer_ << "script->ReleaseRef(instance);";
        }
        printer_.Dedent();
        printer_ << "";
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& func = entity->Ast<cppast::cpp_constructor>();
        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));
        std::string className = entity->parent_->sourceName_;
        auto vars = fmt({
            {"c_function_name",     entity->cFunctionName_},
            {"parameter_name_list", ParameterNameList(func.parameters(), toCppType)},
            {"class_name",          className},
            {"c_parameter_list",    ParameterList(func.parameters(), toCType)},
            {"c_return_type",       className + "*"},
        });
        printer_ << fmt("URHO3D_EXPORT_API {{c_return_type}} {{c_function_name}}({{c_parameter_list}})", vars);
        printer_.Indent();
        {
            printer_ << "return " + typeMapper_->MapToCNoCopy(entity->parent_->sourceName_,
                fmt("new {{class_name}}({{parameter_name_list}})", vars)) + ";";
        }
        printer_.Dedent();
        printer_.WriteLine();
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_member_function>();
        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));

        auto vars = fmt({
            {"name",                entity->name_},
            {"c_function_name",     entity->cFunctionName_},
            {"parameter_name_list", ParameterNameList(func.parameters(), toCppType)},
            {"base_symbol_name",    entity->symbolName_},
            {"class_name",          entity->parent_->sourceName_},
            {"class_name_sanitized", Sanitize(entity->parent_->sourceName_)},
            {"c_parameter_list",    ParameterList(func.parameters(), toCType)},
            {"c_return_type",       typeMapper_->ToCType(func.return_type())},
            {"psep",                func.parameters().empty() ? "" : ", "}
        });

        printer_ << fmt("URHO3D_EXPORT_API {{c_return_type}} {{c_function_name}}({{class_name}}* instance{{psep}}{{c_parameter_list}})", vars);
        printer_.Indent();
        {
            std::string call = "instance->";
            if (func.is_virtual())
                // Virtual methods always overriden in wrapper class so accessing them by simple name should not be
                // an issue.
                call += fmt("{{name}}({{parameter_name_list}})", vars);
            else if (entity->access_ == cppast::cpp_public)
                // Non-virtual public methods sometimes have issues being called. Use fully qualified name for
                // calling them.
                call += fmt("{{base_symbol_name}}({{parameter_name_list}})", vars);
            else
                // Protected non-virtual methods are wrapped in public proxy methods.
                call += fmt("__public_{{name}}({{parameter_name_list}})", vars);

            if (!IsVoid(func.return_type()))
                call = "return " + typeMapper_->MapToC(func.return_type(), call);

            printer_ << call + ";";
            printer_.Flush();
        }
        printer_.Dedent();
        printer_.WriteLine();

        if (func.is_virtual())
        {
            printer_ << fmt("URHO3D_EXPORT_API void set_{{class_name_sanitized}}_fn{{c_function_name}}({{class_name}}* instance, void* fn)",
                vars);
            printer_.Indent();
            {
                printer_ << fmt("instance->fn{{c_function_name}} = (decltype(instance->fn{{c_function_name}}))fn;", vars);
            }
            printer_.Dedent();
            printer_.WriteLine();
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_function>();
        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));

        auto vars = fmt({
            {"name",                entity->name_},
            {"c_function_name",     entity->cFunctionName_},
            {"parameter_name_list", ParameterNameList(func.parameters(), toCppType)},
            {"base_symbol_name",    entity->symbolName_},
            {"class_name",          entity->parent_->sourceName_},
            {"c_parameter_list",    ParameterList(func.parameters(), toCType)},
            {"c_return_type",       typeMapper_->ToCType(func.return_type())},
        });

        printer_ << fmt("URHO3D_EXPORT_API {{c_return_type}} {{c_function_name}}({{c_parameter_list}})", vars);
        printer_.Indent();
        {
            std::string call;
            if (entity->access_ == cppast::cpp_public)
                // Non-virtual public methods sometimes have issues being called. Use fully qualified name for
                // calling them.
                call = fmt("{{base_symbol_name}}({{parameter_name_list}})", vars);
            else
                // Protected non-virtual methods are wrapped in public proxy methods.
                call = fmt("__public_{{name}}({{parameter_name_list}})", vars);

            if (!IsVoid(func.return_type()))
            {
                printer_.Write("return ");
                call = typeMapper_->MapToC(func.return_type(), call);
            }

            printer_.Write(call);
            printer_.Write(";");
            printer_.Flush();
        }
        printer_.Dedent();
        printer_.WriteLine();
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_variable>();
        auto* ns = entity->parent_.Get();

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if ((IsConst(var.type()) || entity->flags_ & HintReadOnly) && !entity->GetDefaultValue().empty())
            return true;

        entity->cFunctionName_ = Sanitize(ns->symbolName_ + "_" + entity->name_);
        auto vars = fmt({{"c_type",          typeMapper_->ToCType(var.type())},
                         {"c_function_name", entity->cFunctionName_},
                         {"namespace_name",  ns->sourceName_},
                         {"name",            entity->name_},
                         {"value",           typeMapper_->MapToCpp(var.type(), "value")}
        });
        // Getter
        printer_.Write(fmt("URHO3D_EXPORT_API {{c_type}} get_{{c_function_name}}(", vars));
        printer_.Write(")");
        printer_.Indent();

        std::string expr = fmt("{{namespace_name}}::{{name}}", vars);

        // Variables are non-temporary therefore they do not need copying.
        printer_ << "return " + typeMapper_->MapToC(var.type(), expr) + ";";

        printer_.Dedent();
        printer_.WriteLine();

        // Setter
        if (!IsConst(var.type()))
        {
            printer_.Write(fmt("URHO3D_EXPORT_API void set_{{c_function_name}}(", vars));
            printer_.Write(fmt("{{c_type}} value)", vars));
            printer_.Indent();

            printer_.Write(fmt("{{namespace_name}}::{{name}} = {{value}};", vars));

            printer_.Dedent();
            printer_.WriteLine();
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_member_variable>();
        auto* ns = entity->parent_.Get();

        // Constants with values get converted to native c# constants in GenerateCSApiPass
        if ((IsConst(var.type()) || entity->flags_ & HintReadOnly) && !entity->GetDefaultValue().empty())
            return true;

        entity->cFunctionName_ = Sanitize(ns->symbolName_ + "_" + entity->name_);
        auto vars = fmt({{"c_type", typeMapper_->ToCType(var.type())},
                         {"c_function_name", entity->cFunctionName_},
                         {"namespace_name",  ns->sourceName_},
                         {"name",            entity->name_},
        });
        // Getter
        printer_.Write(fmt("URHO3D_EXPORT_API {{c_type}} get_{{c_function_name}}({{namespace_name}}* instance)", vars));
        printer_.Indent();
        {
            std::string expr = "instance->";
            if (entity->access_ != cppast::cpp_public)
                expr += fmt("__get_{{name}}()", vars);
            else
                expr += fmt("{{name}}", vars);

            // Variables are non-temporary therefore they do not need copying.
            printer_ << fmt("return {{mapped}};", {{"mapped", typeMapper_->MapToC(var.type(), expr)}});
        }
        printer_.Dedent();
        printer_.WriteLine();

        // Setter
        if (!IsConst(var.type()))
        {
            printer_.Write(fmt("URHO3D_EXPORT_API void set_{{c_function_name}}(", vars));
            printer_.Write(fmt("{{namespace_name}}* instance, {{c_type}} value)", vars));
            printer_.Indent();

            vars.set("value", typeMapper_->MapToCpp(var.type(), "value"));

            if (entity->access_ != cppast::cpp_public)
                printer_.Write(fmt("instance->__set_{{name}}({{value}});", vars));
            else
                printer_.Write(fmt("instance->{{name}} = {{value}};", vars));

            printer_.Dedent();
            printer_.WriteLine();
        }
    }

    return true;
}

void GenerateCApiPass::Stop()
{
    printer_ << "}";    // Close extern "C"

    File file(context_, GetSubsystem<GeneratorContext>()->outputDirCpp_ + "CApi.cpp", FILE_WRITE);
    if (!file.IsOpen())
    {
        URHO3D_LOGERROR("Failed saving CApi.cpp");
        return;
    }
    file.WriteLine(printer_.Get());
    file.Close();
}

std::string GenerateCApiPass::GetUniqueName(const std::string& baseName)
{
    unsigned index = 0;
    std::string newName;
    for (newName = Sanitize(baseName); usedNames_.Contains(newName); index++)
        newName = baseName + std::to_string(index);

    usedNames_.Push(newName);
    return newName;
}

}
