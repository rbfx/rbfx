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
#include <fstream>
#include <spdlog/spdlog.h>
#include "GenerateClassWrappers.h"
#include "GeneratorContext.h"


namespace Urho3D
{

void GenerateClassWrappers::Start()
{
    printer_ << "#pragma once";
    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "#include <CSharp.h>";
    printer_ << "";
    printer_ << "";
    printer_ << "namespace Wrappers";
    printer_ << "{";
    printer_ << "";

    initPrinter_ << "#include <Urho3D/Urho3DAll.h>";
    initPrinter_ << "#include \"ClassWrappers.hpp\"";
    initPrinter_ << "";
    initPrinter_ << fmt::format("extern \"C\" void {}RegisterWrapperFactories(Context* context)",
                                generator->defaultNamespace_);
    initPrinter_.Indent();
    initPrinter_ << "auto* script = context->GetScripts();";
}

bool GenerateClassWrappers::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    if (entity->ast_ == nullptr)
        return true;

    if (entity->ast_->kind() != cppast::cpp_entity_kind::class_t)
        return true;

    if (entity->flags_ & HintProperty)
        return true;

    // Visit only once
    if (info.event == info.container_entity_exit)
        return true;

    // Class is not supposed to be inherited
    if (!generator->inheritable_.IsIncluded(entity->uniqueName_))
        return true;

    const auto& cls = entity->Ast<cppast::cpp_class>();
    if (!HasVirtual(cls) && !HasProtected(cls) && !IsAbstract(cls))
    {
        // Skip children for classes that do not have virtual or protected members
        return info.event != info.container_entity_enter;
    }

    printer_ << fmt::format("class URHO3D_EXPORT_API {} : public {}", entity->name_, entity->uniqueName_);
    printer_.Indent();

    // Urho3D-specific
    if (IsSubclassOf(cls, "Urho3D::Object"))
    {
        printer_ << fmt::format("URHO3D_OBJECT_STATIC(Wrappers::{}, {});", entity->name_, entity->uniqueName_);
        initPrinter_ << fmt::format("script->RegisterType<Wrappers::{}>();", entity->name_);
    }

    printer_.WriteLine("public:", false);
    printer_ << "void* gcHandle_ = nullptr;";
    if (IsSubclassOf(entity->Ast<cppast::cpp_class>(), "Urho3D::Object"))
    {
        printer_ << "Urho3D::TypeInfo* typeInfo_ = nullptr;";
        printer_ << "StringHash GetType() const override { return typeInfo_->GetType(); }";
        printer_ << "const Urho3D::String& GetTypeName() const override { return typeInfo_->GetTypeName(); }";
        printer_ << "const Urho3D::TypeInfo* GetTypeInfo() const override { return typeInfo_; }";
    }

    // Wrap constructors
    for (const auto& e : entity->children_)
    {
        if (e->kind_ == cppast::cpp_entity_kind::constructor_t)
        {
            const auto& ctor = e->Ast<cppast::cpp_constructor>();
            printer_ << fmt::format("{}({}) : {}({}) {{ }}", entity->name_, ParameterList(ctor.parameters()),
                entity->uniqueName_, ParameterNameList(ctor.parameters()));
        }
    }
    printer_ << fmt::format("virtual ~{}()", entity->name_);
    printer_.Indent();
    {
        printer_ << "if (gcHandle_ != nullptr)";
        printer_.Indent();
        {
            printer_ << "scriptSubsystem->FreeGCHandle(gcHandle_);";
            printer_ << "gcHandle_ = nullptr;";
        }
        printer_.Dedent();
        if (IsSubclassOf(entity->Ast<cppast::cpp_class>(), "Urho3D::Object"))
        {
            printer_ << "delete typeInfo_;";
            printer_ << "typeInfo_ = nullptr;";
        }
    }
    printer_.Dedent();

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

                auto name = child->name_;
                auto typeName = cppast::to_string(type);
                auto ref = wouldReturnByCopy ? "&" : "";

                printer_ << fmt::format("{typeName}{ref} __get_{name}() {{ return {name}; }}",
                    FMT_CAPTURE(name), FMT_CAPTURE(typeName), FMT_CAPTURE(ref));
                printer_ << fmt::format("void __set_{name}({typeName} value) {{ {name} = value; }}",
                    FMT_CAPTURE(name), FMT_CAPTURE(typeName), FMT_CAPTURE(ref));
            }
            else if (child->kind_ == cppast::cpp_entity_kind::member_function_t)
            {
                const auto& func = child->Ast<cppast::cpp_member_function>();

                auto methodId = func.name() + func.signature();
                if (std::find(wrappedList.begin(), wrappedList.end(), methodId) == wrappedList.end())
                {
                    wrappedList.emplace_back(methodId);
                    // Function pointer that virtual method will call
                    const auto& cls = child->GetParent();
                    auto typeName = cppast::to_string(func.return_type());
                    auto name = child->sourceName_;
                    auto parameterList = ParameterList(func.parameters());
                    auto parameterNameList = ParameterNameList(func.parameters());
                    auto constModifier = cppast::is_const(func.cv_qualifier()) ? "const " : "";
                    auto pc = Count(func.parameters()) > 0 ? ", " : "";
                    auto cFunctionName = Sanitize(child->uniqueName_);
                    auto fullClassName = entity->uniqueName_;
                    auto className = entity->name_;

                    if (func.is_virtual())
                    {
                        printer_ << fmt::format("{typeName}(*fn{cFunctionName})(void* gcHandle{pc}{parameterList}) = nullptr;",
                            FMT_CAPTURE(typeName), FMT_CAPTURE(cFunctionName), FMT_CAPTURE(className),
                            FMT_CAPTURE(constModifier), FMT_CAPTURE(pc), FMT_CAPTURE(parameterList));
                        // Virtual method that calls said pointer
                        printer_ << fmt::format("{typeName} {name}({parameterList}) {constModifier}override",
                            FMT_CAPTURE(typeName), FMT_CAPTURE(name), FMT_CAPTURE(parameterList),
                            FMT_CAPTURE(constModifier));
                        printer_.Indent();
                        {
                            if (!cppast::is_pure(child->Ast<cppast::cpp_member_function>().virtual_info()))
                            {
                                printer_ << fmt::format("if (fn{cFunctionName} == nullptr)", FMT_CAPTURE(cFunctionName));
                                printer_.Indent();
                                {
                                    printer_ << (IsVoid(func.return_type()) ? "" : "return ") +
                                        fmt::format("{fullClassName}::{name}({parameterNameList});",
                                            FMT_CAPTURE(fullClassName), FMT_CAPTURE(name), FMT_CAPTURE(parameterNameList));
                                }
                                printer_.Dedent();
                                printer_ << "else";
                            }
                            printer_.Indent();
                            {
                                printer_ << (IsVoid(func.return_type()) ? "" : "return ") +
                                    fmt::format("(fn{cFunctionName})(gcHandle_{pc}{parameterNameList});",
                                        FMT_CAPTURE(cFunctionName), FMT_CAPTURE(pc), FMT_CAPTURE(parameterNameList));
                            }
                            printer_.Dedent();

                        }
                        printer_.Dedent();
                    }
                    else if (child->access_ == cppast::cpp_protected) // Protected virtuals are not exposed, no point
                    {
                        printer_ << fmt::format("{typeName} __public_{name}({parameterList})",
                            FMT_CAPTURE(typeName), FMT_CAPTURE(name), FMT_CAPTURE(parameterList));
                        printer_.Indent();
                        printer_ << (IsVoid(func.return_type()) ? "" : "return ") +
                            fmt::format("{name}({parameterNameList});", FMT_CAPTURE(name),
                                FMT_CAPTURE(parameterNameList));
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
                spdlog::get("console")->warn("Base class {} not found!", base.name());
        }
    };

    implementWrapperClassMembers(entity);
    implementBaseWrapperClassMembers(entity);

    printer_.Dedent("};");
    printer_ << "";
    entity->sourceSymbolName_ = "Wrappers::" + entity->name_;    // Wrap a wrapper class
    return true;
}

void GenerateClassWrappers::Stop()
{
    initPrinter_.Dedent();
    printer_ << "}";    // namespace Wrappers

    // Save ClassWrappers.hpp
    {
        std::ofstream fp(generator->outputDirCpp_ + "ClassWrappers.hpp");
        if (!fp.is_open())
        {
            spdlog::get("console")->error("Failed saving ClassWrappers.hpp");
            return;
        }
        fp << printer_.Get();
    }

    // Save RegisterFactories.cpp
    {
        std::ofstream fp(generator->outputDirCpp_ + "RegisterFactories.cpp");
        if (!fp.is_open())
        {
            spdlog::get("console")->error("Failed saving RegisterFactories.cpp");
            return;
        }
        fp << initPrinter_.Get();
    }
}

}
