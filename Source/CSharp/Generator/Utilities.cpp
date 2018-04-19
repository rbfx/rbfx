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

#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_template.hpp>
#include <fmt/format.h>
#include <tinydir.h>
#if _WIN32
#   include <direct.h>
#endif
#include "Utilities.h"
#include "GeneratorContext.h"


namespace Urho3D
{

std::regex WildcardToRegex(const std::string& wildcard)
{
    const std::string wildcardCharacter("@@WILDCARD_STAR@@");

    // Wildcard is converted to regex
    std::string regex = wildcard;

    // * is regex character. Make sure our regex will not interfere with wildcard values
    str::replace_str(regex, "*", wildcardCharacter);

    // Escape regex characters except for *
    const char* special = "\\.^$|()[]{}+?";
    for (char c = *special; c != 0; c = *(++special))
        str::replace_str(regex, std::string(1, c), fmt::format("\\{}", c));

    // Replace wildcard characters
    str::replace_str(regex, wildcardCharacter + wildcardCharacter, ".*");
    str::replace_str(regex, wildcardCharacter, "[^/]*");
    regex = "^" + regex + "$";

    return std::regex(regex);
}

std::string GetScopeName(const cppast::cpp_entity& e)
{
    std::string name = e.name();
    if (name.empty())
        // Give unique symbol to anonymous entities
        name = fmt::format("anonymous_{}", (void*) &e);

    std::vector<std::string> elements{name};
    type_safe::optional_ref<const cppast::cpp_entity> ref(e.parent());
    while (ref.has_value())
    {
        if (!cppast::is_templated(ref.value()) && !cppast::is_friended(ref.value()) &&
            ref.value().kind() != cppast::cpp_entity_kind::file_t)
        {
            const auto& scope = ref.value().scope_name();
            if (scope.has_value() && !scope.value().name().empty())
                elements.emplace_back(scope.value().name());
        }
        ref = ref.value().parent();
    }
    std::reverse(elements.begin(), elements.end());
    return str::join(elements, "::");
}

std::string GetUniqueName(const cppast::cpp_entity& e)
{
    std::string name = GetScopeName(e);
    // Make signature unique for overloaded functions
    switch (e.kind())
    {
    case cppast::cpp_entity_kind::function_t:
    {
        const auto& func = dynamic_cast<const cppast::cpp_function&>(e);
        name += func.signature().c_str();
        break;
    }
    case cppast::cpp_entity_kind::member_function_t:
    {
        const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);
        name += func.signature().c_str();
        break;
    }
    case cppast::cpp_entity_kind::constructor_t:
    {
        const auto& func = dynamic_cast<const cppast::cpp_constructor&>(e);
        name += func.signature().c_str();
        break;
    }
    default:
        break;
    }

    return name;
}

std::string GetUniqueName(const cppast::cpp_entity* e)
{
    return GetUniqueName(*e);
}

std::string GetSymbolName(const cppast::cpp_entity& e)
{
    switch (e.kind())
    {
    case cppast::cpp_entity_kind::namespace_t:
    case cppast::cpp_entity_kind::enum_t:
    case cppast::cpp_entity_kind::enum_value_t:
    case cppast::cpp_entity_kind::class_t:
    case cppast::cpp_entity_kind::variable_t:
    case cppast::cpp_entity_kind::member_variable_t:
    case cppast::cpp_entity_kind::function_t:
    case cppast::cpp_entity_kind::member_function_t:
    case cppast::cpp_entity_kind::constructor_t:
    case cppast::cpp_entity_kind::destructor_t:
    // case cppast::cpp_entity_kind::conversion_op_t:
    {
        auto name = e.name();
        if (e.parent().has_value())
        {
            auto parentName = GetSymbolName(e.parent().value());
            if (!parentName.empty())
                name = parentName + "::" + name;
        }
        return name;
    }
    default:
        break;
    }
    return "";
}

std::string Sanitize(const std::string& value)
{
    std::string result = std::regex_replace(value, std::regex("[^a-zA-Z0-9_]"), "_").c_str();
    if (result[0] >= '0' && result[0] <= '9')
        result = "_" + result;
    return result;
}

bool IsVoid(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::builtin_t)
        return dynamic_cast<const cppast::cpp_builtin_type&>(type).builtin_type_kind() == cppast::cpp_builtin_type_kind::cpp_void;
    return false;
}

bool IsVoid(const cppast::cpp_type* type)
{
    return type == nullptr || IsVoid(*type);
}

std::string EnsureNotKeyword(const std::string& value)
{
    // TODO: Refactor this
    if (value == "object" || value == "params")
        return value + "_";
    return value;
}

std::string MapParameterList(std::vector<std::shared_ptr<MetaEntity>>& parameters,
    const std::function<std::string(MetaEntity*)>& callable)
{
    std::vector<std::string> parts;
    for (const auto& param : parameters)
        parts.emplace_back(callable(param.get()));
    return str::join(parts, ", ");
}

std::string ParameterList(const CppParameters& params,
    const std::function<std::string(const cppast::cpp_type&)>& typeToString)
{
    std::vector<std::string> parts;
    for (const auto& param : params)
    {
        std::string typeString;
        if (typeToString)
            typeString = typeToString(param.type());
        else
            typeString = cppast::to_string(param.type());
        typeString += " " + EnsureNotKeyword(param.name());
        parts.emplace_back(typeString);
    }
    return str::join(parts, ", ");
}

std::string ParameterNameList(const CppParameters& params,
    const std::function<std::string(const cppast::cpp_function_parameter&)>& nameFilter)
{
    std::vector<std::string> parts;
    for (const auto& param : params)
    {
        auto name = EnsureNotKeyword(param.name());
        if (nameFilter)
            name = nameFilter(param);
        parts.emplace_back(name);
    }
    return str::join(parts, ", ");
}

const cppast::cpp_type& GetBaseType(const cppast::cpp_type& type)
{
    switch (type.kind())
    {
    case cppast::cpp_type_kind::cv_qualified_t:
        return GetBaseType(static_cast<const cppast::cpp_cv_qualified_type&>(type).type());
    case cppast::cpp_type_kind::pointer_t:
        return GetBaseType(static_cast<const cppast::cpp_pointer_type&>(type).pointee());
    case cppast::cpp_type_kind::reference_t:
        return GetBaseType(static_cast<const cppast::cpp_reference_type&>(type).referee());
    default:
        return type;
    }
}

std::string GetTypeName(const cppast::cpp_type& type)
{
    return cppast::to_string(GetBaseType(type));
}

bool IsEnumType(const cppast::cpp_type& type)
{
    if (auto* entity = generator->GetSymbol(GetTypeName(type)))
         return entity->kind_ == cppast::cpp_entity_kind::enum_t;
    return false;
}

bool IsComplexType(const cppast::cpp_type& type)
{
    if (auto* map = generator->GetTypeMap(type))
        return !map->isValueType_;

    switch (type.kind())
    {
    case cppast::cpp_type_kind::builtin_t:
        return false;
    case cppast::cpp_type_kind::pointer_t:
        return IsComplexType(dynamic_cast<const cppast::cpp_pointer_type&>(type).pointee());
    case cppast::cpp_type_kind::reference_t:
        return IsComplexType(dynamic_cast<const cppast::cpp_reference_type&>(type).referee());
    case cppast::cpp_type_kind::user_defined_t:
        return !IsEnumType(type);
    case cppast::cpp_type_kind::cv_qualified_t:
        return IsComplexType(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
    default:
        return true;
    }
}

bool IsValueType(const cppast::cpp_type& type)
{
    if (auto* map = generator->GetTypeMap(type, false))
        return map->isValueType_;

    switch (type.kind())
    {
    case cppast::cpp_type_kind::builtin_t:
        return true;
    case cppast::cpp_type_kind::user_defined_t:
        return true;
    case cppast::cpp_type_kind::cv_qualified_t:
        return IsValueType(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
    default:
        return false;
    }
}


IncludedChecker::IncludedChecker(const rapidjson::Value& rules)
{
    Load(rules);
}

void IncludedChecker::Load(const rapidjson::Value& rules)
{
    assert(rules.HasMember("include"));

    const auto& include = rules["include"];
    for (auto it = include.Begin(); it != include.End(); ++it)
        includes_.emplace_back(WildcardToRegex(it->GetString()));

    if (!rules.HasMember("exclude"))
        return;

    const auto& exclude = rules["exclude"];
    for (auto it = exclude.Begin(); it != exclude.End(); ++it)
        excludes_.emplace_back(WildcardToRegex(it->GetString()));
}

bool IncludedChecker::IsIncluded(const std::string& value)
{
    // Verify item is included
    bool match = false;
    for (const auto& re : includes_)
    {
        match = std::regex_match(value, re);
        if (match)
            break;
    }

    // Verify header is not excluded
    if (match)
    {
        for (const auto& re : excludes_)
        {
            if (std::regex_match(value, re))
            {
                match = false;
                break;
            }
        }
    }

    return match;
}

std::string ToString(const cppast::cpp_expression& expression)
{
    if (expression.kind() == cppast::cpp_expression_kind::literal_t)
        return dynamic_cast<const cppast::cpp_literal_expression&>(expression).value();
    else
        return dynamic_cast<const cppast::cpp_unexposed_expression&>(expression).expression().as_string();
}

const cppast::cpp_entity* GetEntity(const cppast::cpp_type& type)
{
    const auto& realType = GetBaseType(type);
    if (realType.kind() == cppast::cpp_type_kind::user_defined_t)
    {
        const auto& userType = dynamic_cast<const cppast::cpp_user_defined_type&>(realType);
        const auto& ref = userType.entity().get(generator->index_);
        if (!ref.empty())
        {
            const auto& definition = ref[0].get();
            if (cppast::is_definition(definition))
                return &definition;
        }
    }
    return nullptr;
}

bool HasVirtual(const cppast::cpp_class& cls)
{
    for (const auto& e : cls)
    {
        if (e.kind() == cppast::cpp_entity_kind::member_function_t)
        {
            const auto& func = dynamic_cast<const cppast::cpp_member_function&>(e);
            if (cppast::is_virtual(func.virtual_info()))
                return true;
        }
    }
    return false;
}

bool HasProtected(const cppast::cpp_class& cls)
{
    auto* entity = static_cast<MetaEntity*>(cls.user_data());
    if (entity == nullptr)
        return false;

    for (auto& child : entity->children_)
    {
        if (child->kind_ == cppast::cpp_entity_kind::member_function_t ||
            child->kind_ == cppast::cpp_entity_kind::member_variable_t)
        {
            if (child->access_ == cppast::cpp_access_specifier_kind::cpp_protected)
                return true;
        }
    }

    return false;
}

bool IsSubclassOf(const cppast::cpp_class& cls, const std::string& symbol)
{
    auto* entity = static_cast<MetaEntity*>(cls.user_data());
    if (entity->uniqueName_ == symbol)
        return true;

    for (const auto& base : cls.bases())
    {
        if (const auto* baseEntity = GetEntity(base.type()))
        {
            if (IsSubclassOf(dynamic_cast<const cppast::cpp_class&>(*baseEntity), symbol))
                return true;
        }
    }

    return false;
}

bool IsAbstract(const cppast::cpp_class& cls)
{
    for (const auto& member : cls)
    {
        if (member.kind() == cppast::cpp_entity_kind::member_function_t)
        {
            auto virtualInfo = dynamic_cast<const cppast::cpp_member_function&>(member).virtual_info();
            if (virtualInfo.has_value())
            {
                if (cppast::is_pure(virtualInfo.value()))
                    return true;
            }
        }
    }

    for (const auto& base : cls.bases())
    {
        const auto* baseCls = GetEntity(base.type());
        assert(baseCls != nullptr);
        if (IsAbstract(dynamic_cast<const cppast::cpp_class&>(*baseCls)))
            return true;
    }
    return false;
}

bool IsConst(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::cv_qualified_t)
        return cppast::is_const(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).cv_qualifier());
    return false;
}

bool IsStatic(const cppast::cpp_entity& entity)
{
    switch (entity.kind())
    {
    case cppast::cpp_entity_kind::class_t:
    {
        if (!dynamic_cast<const cppast::cpp_class&>(entity).bases().empty())
            return false;

        for (const auto& child : dynamic_cast<const cppast::cpp_class&>(entity))
        {
            if (!IsStatic(child))
                return false;
        }
        return true;
    }
    case cppast::cpp_entity_kind::variable_t:
    case cppast::cpp_entity_kind::function_t:
    case cppast::cpp_entity_kind::namespace_t:
        return true;
    case cppast::cpp_entity_kind::member_variable_t:
    case cppast::cpp_entity_kind::member_function_t:
    case cppast::cpp_entity_kind::constructor_t:
    case cppast::cpp_entity_kind::destructor_t:
        return false;
    }

    return true;
}

std::string PrimitiveToPInvokeType(cppast::cpp_builtin_type_kind kind)
{
    switch (kind)
    {
    case cppast::cpp_void: return "void";
    case cppast::cpp_bool: return "bool";
    case cppast::cpp_uchar: return "byte";
    case cppast::cpp_ushort: return "ushort";
    case cppast::cpp_uint: return "uint";
    case cppast::cpp_ulong: return "uint";
    case cppast::cpp_ulonglong: return "ulong";
    case cppast::cpp_uint128: assert(false);
    case cppast::cpp_schar: return "byte";
    case cppast::cpp_short: return "short";
    case cppast::cpp_int: return "int";
    case cppast::cpp_long: return "int";
    case cppast::cpp_longlong: return "long";
    case cppast::cpp_int128: assert(false);
    case cppast::cpp_float: return "float";
    case cppast::cpp_double: return "double";
    case cppast::cpp_longdouble: assert(false);
    case cppast::cpp_float128: assert(false);
    case cppast::cpp_char: return "char";
    case cppast::cpp_wchar: assert(false);
    case cppast::cpp_char16: assert(false);
    case cppast::cpp_char32: assert(false);
    case cppast::cpp_nullptr: return "IntPtr";
    }
}

std::string BuiltinToPInvokeType(const cppast::cpp_type& type)
{
    switch (type.kind())
    {

    case cppast::cpp_type_kind::builtin_t:
    {
        const auto& builtin = dynamic_cast<const cppast::cpp_builtin_type&>(type);
        return PrimitiveToPInvokeType(builtin.builtin_type_kind());
    }
    case cppast::cpp_type_kind::user_defined_t: return "IntPtr";
    case cppast::cpp_type_kind::cv_qualified_t:
    {
        auto name = BuiltinToPInvokeType(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
        if (name == "char*")
            return "string";
        return name;
    }
    case cppast::cpp_type_kind::pointer_t:
        return BuiltinToPInvokeType(dynamic_cast<const cppast::cpp_pointer_type&>(type).pointee()) + "*";
    case cppast::cpp_type_kind::reference_t:
        return BuiltinToPInvokeType(dynamic_cast<const cppast::cpp_reference_type&>(type).referee()) + "*";
    default:
        assert(false);
    }
}

cppast::cpp_builtin_type_kind PrimitiveToCppType(const std::string& type)
{
    if (type == "void")
        return cppast::cpp_void;

    if (type == "bool")
        return cppast::cpp_bool;

    if (type == "unsigned char")
        return cppast::cpp_uchar;
    if (type == "unsigned short")
        return cppast::cpp_ushort;
    if (type == "unsigned int")
        return cppast::cpp_uint;
    if (type == "unsigned long")
        return cppast::cpp_ulong;
    if (type == "unsigned long long")
        return cppast::cpp_ulonglong;
    if (type == "unsigned __int128")
        return cppast::cpp_uint128;

    if (type == "signed char")
        return cppast::cpp_schar;
    if (type == "short")
        return cppast::cpp_short;
    if (type == "int")
        return cppast::cpp_int;
    if (type == "long")
        return cppast::cpp_long;
    if (type == "long long")
        return cppast::cpp_longlong;
    if (type == "__int128")
        return cppast::cpp_int128;

    if (type == "float")
        return cppast::cpp_float;
    if (type == "double")
        return cppast::cpp_double;
    if (type == "long double")
        return cppast::cpp_longdouble;
    if (type == "__float128")
        return cppast::cpp_float128;

    if (type == "char")
        return cppast::cpp_char;
    if (type == "wchar_t")
        return cppast::cpp_wchar;
    if (type == "char16_t")
        return cppast::cpp_char16;
    if (type == "char32_t")
        return cppast::cpp_char32;

    return cppast::cpp_builtin_type_kind::cpp_void;
}

std::string ToPInvokeType(const cppast::cpp_type& type, const std::string& default_)
{
    if (const auto* map = generator->GetTypeMap(type))
        return map->pInvokeType_;
    else if (IsEnumType(type))
        return "global::" + str::replace_str(Urho3D::GetTypeName(type), "::", ".");
    else if (IsComplexType(type))
        return default_;
    else
        return BuiltinToPInvokeType(type);
}

std::string GetTemplateSubtype(const cppast::cpp_type& type)
{
    const auto& baseType = GetBaseType(type);
    if (baseType.kind() == cppast::cpp_type_kind::template_instantiation_t)
    {
        const auto& templateType = dynamic_cast<const cppast::cpp_template_instantiation_type&>(baseType);
        auto templateName = templateType.primary_template().name();
        if (templateName == "SharedPtr" || templateName == "WeakPtr")
        {
            if (templateType.arguments_exposed())
            {
                assert(templateType.arguments().has_value());
                const auto& templateArgs = templateType.arguments().value();
                const auto& realType = templateArgs[0u].type().value();
                return Urho3D::GetTypeName(realType);
            }
            else
                return templateType.unexposed_arguments();
        }
    }
    return "";
}

std::string CamelCaseIdentifier(const std::string& name)
{
    auto tokens = str::split(name, "_");
    for (auto& value : tokens)
    {
        std::transform(value.begin(), value.end(), value.begin(), tolower);
        value.front() = (char)toupper(value.front());
    }
    return str::join(tokens, "");
}

bool IsOutType(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::reference_t || type.kind() == cppast::cpp_type_kind::pointer_t)
    {
        const auto& pointee = type.kind() == cppast::cpp_type_kind::pointer_t ?
            dynamic_cast<const cppast::cpp_pointer_type&>(type).pointee() :
            dynamic_cast<const cppast::cpp_reference_type&>(type).referee();

        if (IsConst(pointee))
            return false;

        const auto& nonCvPointee = cppast::remove_cv(pointee);

        // A pointer to builtin is (almost) definitely output parameter. In some cases (like when pointer to builtin
        // type means a location within array) c++ code should be tweaked to better reflect intent of parameter or c++
        // function should be ignored completely.
        if (nonCvPointee.kind() == cppast::cpp_type_kind::builtin_t && type.kind() == cppast::cpp_type_kind::reference_t)
            return true;

        // Any type mapped to a value type (like std::string mapped to System.String) are output parameters.
        auto* map = generator->GetTypeMap(type);
        if (map != nullptr && map->isValueType_)
            return true;

        if (nonCvPointee.kind() == cppast::cpp_type_kind::user_defined_t)
        {
            if (IsEnumType(nonCvPointee))
                // Does this even happen ever?
                return true;
            else
                // A pointer to object is not necessarily output parameter.
                return false;
        }
    }

    return false;
}

bool IsComplexOutputType(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::reference_t && IsOutType(type) && generator->GetTypeMap(type) != nullptr)
    {
        // Complex typemapped have to output to c++ type and have it converted to c type before function return.
        auto kind = cppast::remove_cv(dynamic_cast<const cppast::cpp_reference_type&>(type).referee()).kind();
        if (kind == cppast::cpp_type_kind::user_defined_t || kind == cppast::cpp_type_kind::template_instantiation_t)
            return true;
    }
    return false;
}

bool IsReference(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::reference_t)
        return true;
    if (type.kind() == cppast::cpp_type_kind::cv_qualified_t)
        return IsReference(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
    return false;
}

bool IsPointer(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::pointer_t)
        return true;
    if (type.kind() == cppast::cpp_type_kind::cv_qualified_t)
        return IsReference(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
    return false;
}

bool IsExported(const cppast::cpp_class& cls)
{
    if (generator->isStatic_)
        // Binding static library. All symbols are always visible.
        return true;

    for (const auto& attr : cls.attributes())
    {
        if (attr.name() == "visibility")
        {
            if (attr.arguments().has_value())
            {
                if (attr.arguments().value().as_string() == "\"default\"")
                    return true;
            }
        }
        else if (attr.name() == "dllexport" || attr.name() == "dllimport")
            // dllimport also checked because headers are likely to have system in place which uses dllexport during
            // build and dllimport when headers are included by consumer application. Instead of having user take care
            // of this case in rules file we just assume that imported entity is also exported entity.
            return true;
    }
    return false;
}

bool ScanDirectory(const std::string& directoryPath, std::vector<std::string>& result, int flags,
                   const std::string& relativeTo)
{
    tinydir_dir dir{};
    if (tinydir_open(&dir, directoryPath.c_str()) != 0)
    {
        spdlog::get("console")->error("Failed to scan directory {}", directoryPath);
        return false;
    }

    auto takePath = [&](const char* path) {
        std::string finalPath(path);
#if _WIN32
        str::replace_str(finalPath, "\\", "/");
#endif
        if (!relativeTo.empty())
        {
            assert(finalPath.find(relativeTo) == 0);
            finalPath = finalPath.substr(relativeTo.length() + (relativeTo.back() == '/' ? 0 : 1));
        }
        result.emplace_back(finalPath);
    };

    while (dir.has_next)
    {
        tinydir_file file;
        if (tinydir_readfile(&dir, &file) != 0)
        {
            spdlog::get("console")->error("Reading directory file failed");
            continue;
        }

        // todo: convert wchar_t* to utf-8 on windows

        if (file.is_dir)
        {
            if (strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0)
            {
                if (flags & ScanDirectoryFlags::IncludeDirs)
                    takePath(file.path);
                if (flags & ScanDirectoryFlags::Recurse)
                {
                    if (!ScanDirectory(file.path, result, flags, relativeTo))
                    {
                        tinydir_close(&dir);
                        return false;
                    }
                }
            }
        }
        else
            takePath(file.path);
        tinydir_next(&dir);
    }

    tinydir_close(&dir);
    return true;
}

void CreateDirsRecursive(const std::string& path)
{
    auto parts = str::split(path, "/", true);
    std::string partialPath;
    for (const auto& part : parts)
    {
        partialPath += part;
        partialPath += "/";
#if _WIN32
        _mkdir(partialPath.c_str());
#else
        mkdir(partialPath.c_str(), 0755);
#endif
    }
}

}

namespace str
{

std::string& replace_str(std::string& dest, const std::string& find, const std::string& replace, unsigned maxReplacements)
{
    int pos = 0;
    while((pos = dest.find(find, pos)) != std::string::npos)
    {
        dest.replace(dest.find(find), find.size(), replace);
        maxReplacements--;
        pos += replace.size();
        if (maxReplacements == 0)
            break;
    }
    return dest;
}

std::string join(const std::vector<std::string>& collection, const std::string& glue)
{
    std::string result;
    if (!collection.empty())
    {
        result = collection.front();

        for (auto it = collection.begin() + 1; it != collection.end(); it++)
            result += glue + *it;
    }
    return result;
}

std::string& replace_str(std::string&& dest, const std::string& find, const std::string& replace, unsigned maxReplacements)
{
    return replace_str(dest, find, replace, maxReplacements);
}

std::vector<std::string> split(const std::string& value, const std::string& separator, bool keepEmpty)
{
    assert(!separator.empty());
    std::vector<std::string> result;
    std::string::const_iterator itStart = value.begin(), itEnd;

    for (;;)
    {
        itEnd = search(itStart, value.end(), separator.begin(), separator.end());
        std::string token(itStart, itEnd);
        if (keepEmpty || !token.empty())
            result.push_back(token);
        if (itEnd == value.end())
            break;
        itStart = itEnd + separator.size();
    }
    return result;
}

void rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::vector<std::string> SplitName(const std::string& name)
{
    std::vector<std::string> result;

    std::string fragment;
    for (int last = 0, first = 0; first < name.length();)
    {
        bool isUnderscore = name[last] == '_';
        if (isUnderscore || (isupper(name[last]) && islower(name[std::max(last - 1, 0)])) || last == name.length())
        {
            fragment = name.substr(first, last - first);
            if (isUnderscore)
                last++;
            first = last;
            if (!fragment.empty())
            {
                std::transform(fragment.begin(), fragment.end(), fragment.begin(), ::tolower);
                fragment[0] = static_cast<char>(::toupper(fragment[0]));
                result.emplace_back(fragment);
            }
        }
        last++;
    }

    return result;
}

std::string AddTrailingSlash(const std::string& str)
{
    if (str.back() != '/')
        return str + "/";
    return str;
}

}
