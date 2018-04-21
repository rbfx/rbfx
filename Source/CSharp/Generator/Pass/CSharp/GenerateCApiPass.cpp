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
#include <cppast/cpp_template.hpp>
#include "GenerateCApiPass.h"
#include "Pass/CSharp/ImplementInterfacesPass.h"


namespace Urho3D
{

void GenerateCApiPass::Start()
{
    printer_ << "#include <mono/metadata/assembly.h>";
    printer_ << "#include <mono/metadata/loader.h>";
    printer_ << "#include <mono/metadata/object.h>";
    printer_ << "#include <mono/metadata/appdomain.h>";
    printer_ << "#include <mono/metadata/class.h>";
    printer_ << "#include <Urho3D/Urho3DAll.h>";
    printer_ << "#include \"CSharp.h\"";
    printer_ << "#include \"ClassWrappers.hpp\"";
    printer_ << "#include \"PODTypes.hpp\"";
    printer_ << "";
    printer_ << "#undef TRANSPARENT";
    printer_ << "#undef near";
    printer_ << "#undef far";
    printer_ << "";
    printer_ << "extern \"C\"";
    printer_ << "{";
    printer_ << "";

    printer_ << fmt::format("void {}RegisterWrapperFactories(Urho3D::Context* context);", generator->defaultNamespace_);

    // Declare extra mono call initializers
    for (const auto& initializer : generator->extraMonoCallInitializers_)
        printer_ << fmt::format("void {}();", initializer);

    printerInternalCalls_ << fmt::format("URHO3D_EXPORT_API void {}RegisterMonoInternalCalls()",
        generator->defaultNamespace_);
    printerInternalCalls_.Indent();
}

bool GenerateCApiPass::Visit(MetaEntity* entity, cppast::visitor_info info)
{
    // Visit entities just once
    if (info.event == info.container_entity_exit || entity->ast_ == nullptr || entity->name_.empty())
        return true;

    auto toCType = [&](const cppast::cpp_type& type) { return ToCType(type); };
    auto toCppType = [&](const cppast::cpp_function_parameter& param) {
        if (IsComplexOutputType(param.type()))
        {
            auto name = param.name() + "Out";
            if (param.type().kind() == cppast::cpp_type_kind::pointer_t)
                name = "&" + name;
            return name;
        }
        return MapToCpp(param.type(), EnsureNotKeyword(param.name()));
    };

    if (entity->kind_ == cppast::cpp_entity_kind::class_t)
    {
        if (IsStatic(*entity->ast_))
            return true;

        const auto& cls = entity->Ast<cppast::cpp_class>();
        auto baseName = Sanitize(entity->uniqueName_);

        // Method for getting type id.
        printer_ << fmt::format("std::uintptr_t {}_typeid()", baseName);
        printer_.Indent();
        {
            printer_ << fmt::format("return GetTypeID<{}>();", entity->symbolName_);
        }
        printer_.Dedent();
        printer_ << "";

        printer_ << fmt::format("std::uintptr_t {}_instance_typeid({}* instance)", baseName, entity->sourceSymbolName_);
        printer_.Indent();
        {
            printer_ << fmt::format("return GetTypeID(instance);");
        }
        printer_.Dedent();
        printer_ << "";

        RegisterMonoInternalCall(entity, baseName + "_typeid");
        RegisterMonoInternalCall(entity, baseName + "_instance_typeid");

        if (!IsExported(cls))
            return true;

        // Destructor always exists even if it is not defined in the class
        printer_ << fmt::format("void {}_destructor({}* instance, bool owner)", baseName, entity->sourceSymbolName_);
        printer_.Indent();
        {
            // Using sourceName_ with wrapper classes causes weird build errors.
            std::string className = entity->symbolName_;
            if (IsSubclassOf(cls, "Urho3D::RefCounted"))
            {
                // RefCounted is not thread-safe therefore extra care has to be taken here.

                // When managed object is releasing a reference and this is last reference we trust that deletion is
                // free to happen on any thread (like finalizers thread) or that user explicitly invoked object disposal
                // on appropriate thread.
                // If managed object is releasing a reference on main thread then we trust this is safe to delete object
                // as well. Engine still may hold reference to an object but is mostly single-threaded therefore this
                // should be safe.
                printer_ << "if (instance->Refs() == 1 || Thread::IsMainThread())";
                printer_.Indent("");
                {
                    printer_ << "instance->ReleaseRef();";
                }
                printer_.Dedent("");
                printer_ << "else";
                printer_.Indent("");
                {
                    // This is not a last ref and managed object is being disposed most likely by a finalizer. Schedule
                    // reference releasing to be done on main thread. This should be safe in most cases. For example if
                    // engine is holding a reference then it is most likely interacted with on main thread.
                    printer_ << "Urho3D::scriptSubsystem->QueueReleaseRef(instance);";
                }
                printer_.Dedent("");
            }
            else
            {
                // Non-RefCounted objects are deleted wherever destruction is invoked. User is trusted to make a
                // decision on which thread object deletion should happen.
                // Object is deleted only if managed object owns native instance. There are cases when managed object
                // gets to interact with objects whose lifetime is managed externally and they are not RefCounted.
                printer_ << "if (owner)";
                printer_.Indent("");
                {
                    printer_ << "delete instance;";
                }
                printer_.Dedent("");
            }
        }
        printer_.Dedent();
        printer_ << "";

        RegisterMonoInternalCall(entity, baseName + "_destructor");

        // Method for pinning managed class instance to native class. Ensures that managed class is nog GC'ed before
        // native class is freed. It is important only for classes that can be inherited.
        bool isInheritable = generator->inheritable_.IsIncluded(entity->symbolName_);
        bool isRefCounted = IsSubclassOf(cls, "Urho3D::RefCounted");
        if (isInheritable || isRefCounted)
        {
            printer_ << fmt::format("void {}_setup({}* instance, void* gcHandle, const char* typeName)", baseName, entity->sourceSymbolName_);
            printer_.Indent();
            {
                const auto& cls = entity->Ast<cppast::cpp_class>();
                if (isRefCounted)
                {
                    printer_ << "assert(instance->HasDeleter() == false);";
                    printer_ << "instance->SetDeleter([](RefCounted* instance_, void* gcHandle_) {";
                    printer_.Indent("");
                    {
                        printer_ << "scriptSubsystem->FreeGCHandle(gcHandle_);";
                        printer_ << "delete instance_;";
                    }
                    printer_.Dedent("}, gcHandle);");
                }
                if (isInheritable)
                {
                    if (isRefCounted)
                        // Ensure that different GC handles are stored in wrapepr class and refcounted deleter user data
                        printer_ << "gcHandle = scriptSubsystem->CloneGCHandle(gcHandle);";
                    printer_ << "instance->gcHandle_ = gcHandle;";
                    if (IsSubclassOf(cls, "Urho3D::Object"))
                        printer_ << fmt::format("instance->typeInfo_ = new Urho3D::TypeInfo(typeName, {}::GetTypeInfoStatic());", entity->sourceSymbolName_);
                }
            }
            printer_.Dedent();
            printer_ << "";

            RegisterMonoInternalCall(entity, baseName + "_setup");
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::constructor_t)
    {
        const auto& cls = entity->GetParent()->Ast<cppast::cpp_class>();
        const auto& func = entity->Ast<cppast::cpp_constructor>();
        if (!IsExported(cls))
        {
            entity->Remove();
            return true;
        }

        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));
        std::string className = entity->GetParent()->sourceSymbolName_;
        printer_ << "// " + entity->uniqueName_;
        printer_ << fmt::format("{type} {name}({params})",
            fmt::arg("type", className + "*"), fmt::arg("name", entity->cFunctionName_),
            fmt::arg("params", ParameterList(func.parameters(), toCType)));
        printer_.Indent();
        {
            // Do not AddRef to RefCounted objects here because we may end up having several managed classes pointing to
            // same native instance, therefore wrapper classes AddRef instead.
            PrintParameterHandlingCodePre(entity->children_);
            printer_ << "auto* returnValue = " +
                fmt::format("new {class}({params})", fmt::arg("class", className),
                    fmt::arg("params", ParameterNameList(func.parameters(), toCppType))) + ";";
            PrintParameterHandlingCodePost(entity->children_);
            printer_ << "return returnValue;";
        }
        printer_.Dedent();
        printer_ << "";

        RegisterMonoInternalCall(entity->parent_.lock().get(), entity->cFunctionName_);
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_member_function>();
        const auto& cls = entity->GetParent()->Ast<cppast::cpp_class>();
        if (!IsExported(cls))
        {
            entity->Remove();
            return true;
        }

        auto isFinal = !generator->inheritable_.IsIncluded(entity->GetParent()->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));

        auto cFunction = entity->cFunctionName_;
        auto paramNames = ParameterNameList(func.parameters(), toCppType);
        auto className = entity->GetFirstParentOfKind(cppast::cpp_entity_kind::class_t)->sourceSymbolName_;

        printer_ << "// " + entity->uniqueName_;
        printer_ << fmt::format("{rtype} {cFunction}({className}* instance{psep}{params})",
            fmt::arg("rtype", ToCType(func.return_type(), true)), FMT_CAPTURE(cFunction), FMT_CAPTURE(className),
            fmt::arg("psep", func.parameters().empty() ? "" : ", "),
            fmt::arg("params", ParameterList(func.parameters(), toCType)));
        printer_.Indent();
        {
            PrintParameterHandlingCodePre(entity->children_);
            std::string call = "instance->";
            if (func.is_virtual())
                // Virtual methods always overriden in wrapper class so accessing them by simple name should not be
                // an issue.
                call += fmt::format("{}({})", entity->sourceName_, paramNames);
            else if (entity->access_ == cppast::cpp_public)
                // Non-virtual public methods sometimes have issues being called. Use fully qualified name for
                // calling them.
                call += fmt::format("{}({})", entity->sourceSymbolName_, paramNames);
            else
                // Protected non-virtual methods are wrapped in public proxy methods.
                call += fmt::format("__public_{}({})", entity->sourceName_, paramNames);

            if (!IsVoid(func.return_type()))
                call = GetAutoType(func.return_type()) + " returnValue = " + MapToC(func.return_type(), call);

            printer_ << call + ";";
            PrintParameterHandlingCodePost(entity->children_);
            if (!IsVoid(func.return_type()))
                printer_ << "return returnValue;";

            printer_.Flush();
        }
        printer_.Dedent();
        printer_ << "";

        RegisterMonoInternalCall(entity->parent_.lock().get(), entity->cFunctionName_);

        if (func.is_virtual() && !isFinal)
        {
            auto virtCFunction = fmt::format("set_fn{cFunction}", FMT_CAPTURE(cFunction));
            printer_ << fmt::format("void {virtCFunction}({className}* instance, void* fn)",
                FMT_CAPTURE(virtCFunction), FMT_CAPTURE(className));
            printer_.Indent();
            {
                printer_ << fmt::format("instance->fn{cFunction} = (decltype(instance->fn{cFunction}))fn;",
                    FMT_CAPTURE(cFunction));
            }
            printer_.Dedent();
            printer_ << "";

            RegisterMonoInternalCall(entity->parent_.lock().get(), virtCFunction);
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::function_t)
    {
        const auto& func = entity->Ast<cppast::cpp_function>();
        entity->cFunctionName_ = GetUniqueName(Sanitize(entity->uniqueName_));
        auto paramNames = ParameterNameList(func.parameters(), toCppType);

        printer_ << "// " + entity->uniqueName_;
        printer_ << fmt::format("{} {}({})",
            ToCType(func.return_type(), true), entity->cFunctionName_, ParameterList(func.parameters(), toCType));
        printer_.Indent();
        {
            PrintParameterHandlingCodePre(entity->children_);
            std::string call;
            if (entity->access_ == cppast::cpp_public)
                // Non-virtual public methods sometimes have issues being called. Use fully qualified name for
                // calling them.
                call = fmt::format("{}({})", entity->sourceSymbolName_, paramNames);
            else
                // Protected non-virtual methods are wrapped in public proxy methods.
                call = fmt::format("__public_{}({})", entity->name_, paramNames);

            if (!IsVoid(func.return_type()))
            {
                printer_.Write(GetAutoType(func.return_type()) + " returnValue = ");
                call = MapToC(func.return_type(), call);
            }

            printer_.Write(call + ";");
            PrintParameterHandlingCodePost(entity->children_);

            if (!IsVoid(func.return_type()))
                printer_ << "return returnValue;";
        }
        printer_.Dedent();
        printer_ << "";

        RegisterMonoInternalCall(entity->parent_.lock().get(), entity->cFunctionName_);
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_variable>();
        auto* ns = entity->GetParent();

        // Constants with values get converted to native c# constants in GenerateCSharpApiPass
        if ((IsConst(var.type()) || entity->flags_ & HintReadOnly) && !entity->GetDefaultValue().empty())
            return true;

        entity->cFunctionName_ = Sanitize(ns->symbolName_ + "_" + entity->name_);
        auto rtype = ToCType(var.type(), true);
        auto cFunction = entity->cFunctionName_;
        auto namespaceName = ns->sourceSymbolName_;
        auto name = entity->name_;
        auto value = MapToCpp(var.type(), "value");

        // Getter
        printer_ << "// " + entity->uniqueName_;
        printer_.Write(fmt::format("{rtype} get_{cFunction}()", FMT_CAPTURE(rtype),
            FMT_CAPTURE(cFunction)));
        printer_.Indent();

        std::string expr = fmt::format("{namespaceName}::{name}", FMT_CAPTURE(namespaceName), FMT_CAPTURE(name));

        // Variables are non-temporary therefore they do not need copying.
        printer_ << "return " + MapToC(var.type(), expr) + ";";

        printer_.Dedent();
        printer_ << "";

        RegisterMonoInternalCall(entity->parent_.lock().get(), "get_" + cFunction);

        // Setter
        if (!IsConst(var.type()))
        {
            printer_.Write(fmt::format("void set_{cFunction}(", FMT_CAPTURE(cFunction)));
            printer_.Write(fmt::format("{rtype} value)", FMT_CAPTURE(rtype)));
            printer_.Indent();

            printer_.Write(fmt::format("{namespaceName}::{name} = {value};", FMT_CAPTURE(namespaceName),
                FMT_CAPTURE(name), FMT_CAPTURE(value)));

            printer_.Dedent();
            printer_ << "";

            RegisterMonoInternalCall(entity->parent_.lock().get(), "set_" + cFunction);
        }
    }
    else if (entity->kind_ == cppast::cpp_entity_kind::member_variable_t)
    {
        const auto& var = entity->Ast<cppast::cpp_member_variable>();
        auto* ns = entity->GetParent();

        // Constants with values get converted to native c# constants in GenerateCSharpApiPass
        if ((IsConst(var.type()) || entity->flags_ & HintReadOnly) && !entity->GetDefaultValue().empty())
            return true;

        auto isFinal = !generator->inheritable_.IsIncluded(entity->GetParent()->symbolName_);
        if (isFinal && entity->access_ != cppast::cpp_public)
            return true;

        entity->cFunctionName_ = Sanitize(ns->symbolName_ + "_" + entity->name_);
        auto cType = ToCType(var.type(), true);
        auto cFunction = entity->cFunctionName_;
        auto namespaceName = ns->sourceSymbolName_;
        auto name = entity->name_;

        // Getter
        printer_ << "// " + entity->uniqueName_;
        printer_.Write(fmt::format("{cType} get_{cFunction}({namespaceName}* instance)",
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
        printer_ << "";

        RegisterMonoInternalCall(entity->parent_.lock().get(), "get_" + cFunction);

        // Setter
        if (!IsConst(var.type()))
        {
            printer_.Write(fmt::format("void set_{cFunction}(", FMT_CAPTURE(cFunction)));
            printer_.Write(fmt::format("{namespaceName}* instance, {cType} value)",
                FMT_CAPTURE(namespaceName), FMT_CAPTURE(cType)));
            printer_.Indent();

            auto value = MapToCpp(var.type(), "value");
            if (entity->access_ != cppast::cpp_public)
                printer_.Write(fmt::format("instance->__set_{name}({value});", FMT_CAPTURE(name), FMT_CAPTURE(value)));
            else
                printer_.Write(fmt::format("instance->{name} = {value};", FMT_CAPTURE(name), FMT_CAPTURE(value)));

            printer_.Dedent();
            printer_ << "";

            RegisterMonoInternalCall(entity->parent_.lock().get(), "set_" + cFunction);
        }
    }

    return true;
}

void GenerateCApiPass::Stop()
{
    // Generate calls that obtain object offsets in case of multiple inheritance
    auto* pass = generator->GetPass<DiscoverInterfacesPass>();
    for (const auto& pair: pass->inheritedBy_)
    {
        auto* inherited = generator->GetSymbol(pair.first);
        if (inherited == nullptr)
            continue;

        for (const auto& inheritorName : pair.second)
        {
            auto inheritor = generator->GetSymbol(inheritorName);
            if (inheritor == nullptr)
                continue;

            auto cFunction = fmt::format("{}_{}_offset", Sanitize(inheritor->symbolName_),
                Sanitize(inherited->symbolName_));
            printer_ << fmt::format("int {cFunction}()", FMT_CAPTURE(cFunction));
            printer_.Indent();
            {
                printer_ << fmt::format("return GetBaseClassOffset<{}, {}>();", inheritor->symbolName_,
                                        inherited->symbolName_);
            }
            printer_.Dedent();
            printer_ << "";

            RegisterMonoInternalCall(inherited, cFunction);
        }
    }

    // Call extra initialization functions
    for (const auto& initializer : generator->extraMonoCallInitializers_)
        printerInternalCalls_ << initializer + "();";

    printerInternalCalls_.Dedent();
    printer_ << printerInternalCalls_.Get();
    printer_ << "";

    printer_ << fmt::format("URHO3D_EXPORT_API void {}RegisterCSharp(Urho3D::Context* context)", generator->defaultNamespace_);
    printer_.Indent();
    {
        printer_ << "if (context->GetScripts() == nullptr)";
        printer_.Indent("");
        {
            printer_ << "context->RegisterSubsystem(new ScriptSubsystem(context));";
        }
        printer_.Dedent("");

        printer_ << fmt::format("{}RegisterWrapperFactories(context);", generator->defaultNamespace_);
        // Put other wrapper late initialization code here.
    }
    printer_.Dedent();

    printer_ << "";
    printer_ << "}";    // Close extern "C"


    std::ofstream fp(generator->outputDirCpp_ + "CApi.cpp");
    if (!fp.is_open())
    {
        spdlog::get("console")->error("Failed saving CApi.cpp");
        return;
    }
    fp << printer_.Get();
}

std::string GenerateCApiPass::GetUniqueName(const std::string& baseName)
{
    unsigned index = 0;
    std::string newName;
    for (newName = Sanitize(baseName); std::find(usedNames_.begin(), usedNames_.end(), newName) != usedNames_.end(); index++);
    usedNames_.emplace_back(baseName + std::to_string(index));
    return newName;
}

std::string GenerateCApiPass::MapToCpp(const cppast::cpp_type& type, const std::string& expression)
{
    const auto* map = generator->GetTypeMap(type, false);
    std::string result = expression;

    if (map)
        return fmt::format(map->cToCppTemplate_.c_str(), fmt::arg("value", result));
    else if (type.kind() == cppast::cpp_type_kind::template_instantiation_t)
        return fmt::format("{type}({expr})", fmt::arg("type", Urho3D::GetTypeName(type)), fmt::arg("expr", result));

    // TODO: This is getting out of hand, simplify this. Maybe implement dereferencing in c++?
    if (!IsEnumType(type) && ((IsValueType(type) && IsComplexType(type)) || IsReference(type)))
        result = "*" + result;
    return result;
}

std::string GenerateCApiPass::MapToC(const cppast::cpp_type& type, const std::string& expression)
{
    const auto* map = generator->GetTypeMap(type);
    std::string result = expression;

    if (map)
        result = fmt::format(map->cppToCTemplate_.c_str(), fmt::arg("value", result));
    else if (IsComplexType(type))
    {
        auto typeName = GetTemplateSubtype(type);
        if (typeName.empty())
            typeName = Urho3D::GetTypeName(type);
        result = fmt::format("CSharpObjConverter::ToCSharp<{type}>({result})", FMT_CAPTURE(result), fmt::arg("type", typeName));
    }

    return result;
}

std::string GenerateCApiPass::ToCType(const cppast::cpp_type& type, bool disallowReferences)
{
    std::function<std::string(const cppast::cpp_type&)> toCType = [&](const cppast::cpp_type& t) {
        switch (t.kind())
        {
        case cppast::cpp_type_kind::builtin_t:
        case cppast::cpp_type_kind::user_defined_t:
            return cppast::to_string(t);
        case cppast::cpp_type_kind::cv_qualified_t:
        {
            const auto& cv = dynamic_cast<const cppast::cpp_cv_qualified_type&>(t);
            return (cppast::is_volatile(cv.cv_qualifier()) ? "volatile " : "") + toCType(cv.type());
        }
        case cppast::cpp_type_kind::pointer_t:
            return toCType(dynamic_cast<const cppast::cpp_pointer_type&>(t).pointee()) + "*";
        case cppast::cpp_type_kind::reference_t:
            return toCType(dynamic_cast<const cppast::cpp_reference_type&>(t).referee()) + "*";
        case cppast::cpp_type_kind::template_instantiation_t:
        {
            const auto& tpl = dynamic_cast<const cppast::cpp_template_instantiation_type&>(t);
            auto tplName = tpl.primary_template().name();
            if (tplName == "SharedPtr" || tplName == "WeakPtr")
                return tpl.unexposed_arguments() + "*";
            assert(false);
        }
        default:
            assert(false);
        }
        return std::string();
    };

    std::string typeName;
    if (const auto* map = generator->GetTypeMap(type))
    {
        typeName = map->cType_;
        if (IsOutType(type) && !disallowReferences)
            // Typemaps map to blittable types therefore we should return them as values.
            typeName += "*";
    }
    else
    {
        typeName = toCType(type);
        if (IsValueType(type) && IsComplexType(type))
            // Value types turned into pointers
            typeName += "*";
    }
    return typeName;
}

void GenerateCApiPass::PrintParameterHandlingCodePre(const std::vector<std::shared_ptr<MetaEntity>>& parameters)
{
    for (const auto& param : parameters)
    {
        auto value = param->GetNativeDefaultValue();
        if (!value.empty())
        {
            // Some default values need extra care
            const auto& cppType = param->Ast<cppast::cpp_function_parameter>().type();
            auto* typeMap = generator->GetTypeMap(GetBaseType(cppType));
            if (typeMap != nullptr && typeMap->csType_ == "string")
            {
                printer_ << fmt::format("if ({} == nullptr)", param->name_);
                printer_.Indent();
                {
                    printer_ << fmt::format("{} = mono_string_empty(mono_domain_get());", param->name_);
                }
                printer_.Dedent();
            }
            else if (typeMap == nullptr && IsComplexType(cppType) && value != "nullptr")
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
        const auto& type = param->Ast<cppast::cpp_function_parameter>().type();
        if (IsComplexOutputType(type))
            // Typemapped output types need to be mapped back and forth
            printer_ << "auto " + param->name_ + "Out = " + MapToCpp(type, "*" + param->name_) + ";";
    }
}

void GenerateCApiPass::PrintParameterHandlingCodePost(const std::vector<std::shared_ptr<MetaEntity>>& parameters)
{
    for (const auto& param : parameters)
    {
        const auto& type = param->Ast<cppast::cpp_function_parameter>().type();
        if (IsComplexOutputType(type))
            printer_ << "*" + param->name_ + " = " + MapToC(type, param->name_ + "Out") + ";";
    }
}

std::string GenerateCApiPass::GetAutoType(const cppast::cpp_type& type)
{
    const auto& nonCvType = cppast::remove_cv(type);
    if (auto* map = generator->GetTypeMap(nonCvType))
    {
        if (map->csType_ == "string")
            return "auto*";
        else if (map->isValueType_)
            return "auto&&";
        else
            return "auto*";
    }
    else if (nonCvType.kind() == cppast::cpp_type_kind::builtin_t || IsEnumType(nonCvType))
        return "auto&&";
    else
        return "auto*";
}

std::string GenerateCApiPass::GetMonoInternalCallClassName(MetaEntity* cls)
{
    std::vector<std::string> parts;
    // Gather parts of symbol name
    while (cls)
    {
        if (!cls->name_.empty())
            parts.emplace_back(cls->name_);
        cls = cls->parent_.lock().get();
    }

    // Process parts by inserting . separator after namespaces and :: after anything else.
    std::string result;
    std::string currentSymbol;

    for (auto it = parts.rbegin(); it != parts.rend(); it++)
    {
        result += *it;
        currentSymbol += *it;
        auto* entity = generator->GetSymbol(currentSymbol);
        if (entity == nullptr)
            continue;

        if (it + 1 != parts.rend())  // not last
        {
            if (entity->kind_ == cppast::cpp_entity_kind::namespace_t)
                result += ".";
            else
                result += "::";
            currentSymbol += "::";
        }
    }

    return result;
}

void GenerateCApiPass::RegisterMonoInternalCall(MetaEntity* cls, const std::string& function)
{
    printerInternalCalls_ << fmt::format("MONO_INTERNAL_CALL({}, {});", GetMonoInternalCallClassName(cls), function);
}

}
