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

#include <fmt/format.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/File.h>
#include "GenerateCApiPass.h"


namespace Urho3D
{

void GenerateCApiPass::Start()
{
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

    auto toCType = [&](const cppast::cpp_type& type) { return ToCType(type); };
    auto toCppType = [&](const cppast::cpp_function_parameter& param) {
        return MapToCpp(param.type(), EnsureNotKeyword(param.name()));
    };

    if (entity->kind_ == cppast::cpp_entity_kind::class_t)
    {
        if (IsStatic(*entity->ast_))
            return true;

        // Destructor always exists even if it is not defined in the class
        String cFunctionName = GetUniqueName(Sanitize(entity->uniqueName_) + "_destructor");

        printer_ << fmt::format("URHO3D_EXPORT_API void {}({}* instance)", cFunctionName.CString(), entity->name_);
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
        printer_ << fmt::format("URHO3D_EXPORT_API {type} {name}({params})",
            fmt::arg("type", className + "*"), fmt::arg("name", entity->cFunctionName_),
            fmt::arg("params", ParameterList(func.parameters(), toCType)));
        printer_.Indent();
        {
            PrintDefaultValueCode(entity->children_);
            printer_ << "return " + MapToCNoCopy(entity->parent_->sourceName_,
                fmt::format("new {class}({params})", fmt::arg("class", className),
                    fmt::arg("params", ParameterNameList(func.parameters(), toCppType)))) + ";";
        }
        printer_.Dedent();
        printer_.WriteLine();
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_member_function>();

        auto isFinal = generator->final_.Contains(entity->parent_->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));

        auto name = entity->name_;
        auto cFunction = entity->cFunctionName_;
        auto paramNames = ParameterNameList(func.parameters(), toCppType);
        auto className = entity->access_ == cppast::cpp_public ? entity->parent_->symbolName_ : entity->parent_->sourceName_;

        printer_ << fmt::format("URHO3D_EXPORT_API {rtype} {cFunction}({className}* instance{psep}{params})",
            fmt::arg("rtype", ToCType(func.return_type())), FMT_CAPTURE(cFunction), FMT_CAPTURE(className),
            fmt::arg("psep", func.parameters().empty() ? "" : ", "),
            fmt::arg("params", ParameterList(func.parameters(), toCType)));
        printer_.Indent();
        {
            PrintDefaultValueCode(entity->children_);
            std::string call = "instance->";
            if (func.is_virtual())
                // Virtual methods always overriden in wrapper class so accessing them by simple name should not be
                // an issue.
                call += fmt::format("{}({})", entity->name_, paramNames);
            else if (entity->access_ == cppast::cpp_public)
                // Non-virtual public methods sometimes have issues being called. Use fully qualified name for
                // calling them.
                call += fmt::format("{}({})", entity->symbolName_, paramNames);
            else
                // Protected non-virtual methods are wrapped in public proxy methods.
                call += fmt::format("__public_{}({})", entity->name_, paramNames);

            if (!IsVoid(func.return_type()))
                call = "return " + MapToC(func.return_type(), call);

            printer_ << call + ";";
            printer_.Flush();
        }
        printer_.Dedent();
        printer_.WriteLine();

        if (func.is_virtual() && !isFinal)
        {
            printer_ << fmt::format("URHO3D_EXPORT_API void set_{name}_fn{cFunction}({className}* instance, void* fn)",
                fmt::arg("name", Sanitize(entity->parent_->sourceName_)), FMT_CAPTURE(cFunction),
                fmt::arg("className", entity->parent_->sourceName_));
            printer_.Indent();
            {
                printer_ << fmt::format("instance->fn{cFunction} = (decltype(instance->fn{cFunction}))fn;",
                    FMT_CAPTURE(cFunction));
            }
            printer_.Dedent();
            printer_.WriteLine();
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_function>();
        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));
        auto paramNames = ParameterNameList(func.parameters(), toCppType);

        printer_ << fmt::format("URHO3D_EXPORT_API {} {}({})",
            ToCType(func.return_type()), entity->cFunctionName_, ParameterList(func.parameters(), toCType));
        printer_.Indent();
        {
            PrintDefaultValueCode(entity->children_);
            std::string call;
            if (entity->access_ == cppast::cpp_public)
                // Non-virtual public methods sometimes have issues being called. Use fully qualified name for
                // calling them.
                call = fmt::format("{}({})", entity->symbolName_, paramNames);
            else
                // Protected non-virtual methods are wrapped in public proxy methods.
                call = fmt::format("__public_{}({})", entity->name_, paramNames);

            if (!IsVoid(func.return_type()))
            {
                printer_.Write("return ");
                call = MapToC(func.return_type(), call);
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
        auto rtype = ToCType(var.type());
        auto cFunction = entity->cFunctionName_;
        auto namespaceName = ns->sourceName_;
        auto name = entity->name_;
        auto value = MapToCpp(var.type(), "value");

        // Getter
        printer_.Write(fmt::format("URHO3D_EXPORT_API {rtype} get_{cFunction}()", FMT_CAPTURE(rtype),
            FMT_CAPTURE(cFunction)));
        printer_.Indent();

        std::string expr = fmt::format("{namespaceName}::{name}", FMT_CAPTURE(namespaceName), FMT_CAPTURE(name));

        // Variables are non-temporary therefore they do not need copying.
        printer_ << "return " + MapToC(var.type(), expr) + ";";

        printer_.Dedent();
        printer_.WriteLine();

        // Setter
        if (!IsConst(var.type()))
        {
            printer_.Write(fmt::format("URHO3D_EXPORT_API void set_{cFunction}(", FMT_CAPTURE(cFunction)));
            printer_.Write(fmt::format("{rtype} value)", FMT_CAPTURE(rtype)));
            printer_.Indent();

            printer_.Write(fmt::format("{namespaceName}::{name} = {value};", FMT_CAPTURE(namespaceName),
                FMT_CAPTURE(name), FMT_CAPTURE(value)));

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

        auto isFinal = generator->final_.Contains(entity->parent_->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        entity->cFunctionName_ = Sanitize(ns->symbolName_ + "_" + entity->name_);
        auto cType = ToCType(var.type());
        auto cFunction = entity->cFunctionName_;
        auto namespaceName = ns->sourceName_;
        auto name = entity->name_;

        // Getter
        printer_.Write(fmt::format("URHO3D_EXPORT_API {cType} get_{cFunction}({namespaceName}* instance)",
            FMT_CAPTURE(cType), FMT_CAPTURE(cFunction), FMT_CAPTURE(namespaceName)));
        printer_.Indent();
        {
            std::string expr = "instance->";
            if (entity->access_ != cppast::cpp_public)
                expr += fmt::format("__get_{}()", name);
            else
                expr += name;

            // Variables are non-temporary therefore they do not need copying.
            auto mapped = MapToC(var.type(), expr);
            printer_ << fmt::format("return {mapped};", FMT_CAPTURE(mapped));
        }
        printer_.Dedent();
        printer_.WriteLine();

        // Setter
        if (!IsConst(var.type()))
        {
            printer_.Write(fmt::format("URHO3D_EXPORT_API void set_{cFunction}(", FMT_CAPTURE(cFunction)));
            printer_.Write(fmt::format("{namespaceName}* instance, {cType} value)",
                FMT_CAPTURE(namespaceName), FMT_CAPTURE(cType)));
            printer_.Indent();

            auto value = MapToCpp(var.type(), "value");
            if (entity->access_ != cppast::cpp_public)
                printer_.Write(fmt::format("instance->__set_{name}({value});", FMT_CAPTURE(name), FMT_CAPTURE(value)));
            else
                printer_.Write(fmt::format("instance->{name} = {value};", FMT_CAPTURE(name), FMT_CAPTURE(value)));

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

std::string GenerateCApiPass::MapToCNoCopy(const std::string& type, const std::string& expression)
{
    const auto* map = generator->GetTypeMap(type);
    std::string result = expression;

    if (map)
        result = fmt::format(map->cppToCTemplate_.c_str(), fmt::arg("value", result));
    else if (generator->symbols_.Contains(type))
        result = fmt::format("script->TakeOwnership<{type}>({result})", FMT_CAPTURE(result), FMT_CAPTURE(type));

    return result;
}

std::string GenerateCApiPass::MapToCpp(const cppast::cpp_type& type, const std::string& expression)
{
    const auto* map = generator->GetTypeMap(type);
    std::string result = expression;

    if (map)
        result = fmt::format(map->cToCppTemplate_.c_str(), fmt::arg("value", result));
    else if (IsComplexValueType(type))
    {
        if (type.kind() != cppast::cpp_type_kind::pointer_t)
            result = "*" + result;
    }

    return result;
}

std::string GenerateCApiPass::MapToC(const cppast::cpp_type& type, const std::string& expression)
{
    const auto* map = generator->GetTypeMap(type);
    std::string result = expression;

    if (map)
        result = fmt::format(map->cppToCTemplate_.c_str(), fmt::arg("value", result));
    else if (IsComplexValueType(type))
        result = fmt::format("script->AddRef<{type}>({result})", FMT_CAPTURE(result),
            fmt::arg("type", Urho3D::GetTypeName(type)));

    return result;
}

std::string GenerateCApiPass::ToCType(const cppast::cpp_type& type)
{
    if (const auto* map = generator->GetTypeMap(type))
        return map->cType_;

    std::string typeName = cppast::to_string(type);

    if (IsEnumType(type))
        return typeName;

    if (IsComplexValueType(type))
        // A value type is turned into pointer.
        return Urho3D::GetTypeName(type) + "*";

    // Builtin type
    return typeName;
}

void GenerateCApiPass::PrintDefaultValueCode(const std::vector<SharedPtr<MetaEntity>>& parameters)
{
    for (const auto& param : parameters)
    {
        auto value = param->GetDefaultValue();
        if (value.empty())
            continue;

        const auto& cppType = param->Ast<cppast::cpp_function_parameter>().type();
        auto* typeMap = generator->GetTypeMap(cppType);
        if (typeMap != nullptr && typeMap->csType_ == "string")
        {
            printer_ << fmt::format("if ({} == nullptr)", param->name_);
            printer_.Indent();
            {
                printer_ << fmt::format("{} = \"\";", param->name_);
            }
            printer_.Dedent();
        }
        else if (IsComplexValueType(cppType) && value != "nullptr")
        {
            printer_ << fmt::format("if ({} == nullptr)", param->name_);
            printer_.Indent();
            {
                auto typeName = Urho3D::GetTypeName(cppType);
                std::string ref;
                if (cppType.kind() == cppast::cpp_type_kind::reference_t)
                    ref = "&";
                printer_ << fmt::format("{} = {}const_cast<{}{}>({});", param->name_, ref, typeName, ref, value);
            }
            printer_.Dedent();
        }
    }
}

}
