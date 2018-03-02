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

#include <Urho3D/Core/StringUtils.h>
#include <cppast/cpp_member_function.hpp>
#include <Urho3D/IO/Log.h>
#include "Utilities.h"
#include "GeneratorContext.h"


namespace Urho3D
{

std::regex WildcardToRegex(const Urho3D::String& wildcard)
{
    const String wildcardCharacter("@@WILDCARD_STAR@@");

    // Wildcard is converted to regex
    String regex = wildcard;

    // * is regex character. Make sure our regex will not interfere with wildcard values
    regex.Replace("*", wildcardCharacter);

    // Escape regex characters except for *
    const char* special = "\\.^$|()[]{}+?";
    for (char c = *special; c != 0; c = *(++special))
        regex.Replace(String(c), ToString("\\%c", c));

    // Replace wildcard characters
    regex.Replace(wildcardCharacter + wildcardCharacter, ".*");
    regex.Replace(wildcardCharacter, "[^/]*");
    regex = "^" + regex + "$";

    return std::regex(regex.CString());
}

std::string GetScopeName(const cppast::cpp_entity& e)
{
    std::string name = e.name();
    if (name.empty())
        // Give unique symbol to anonymous entities
        name = ToString("anonymous_%p", (void*) &e).CString();

    std::vector<std::string> elements{name};
    type_safe::optional_ref<const cppast::cpp_entity> ref(e.parent());
    while (ref.has_value())
    {
        if (!cppast::is_templated(ref.value()) && !cppast::is_friended(ref.value()) &&
            ref.value().kind() != cppast::cpp_entity_kind::file_t)
        {
            const auto& scope = ref.value().scope_name();
            if (scope.has_value())
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

std::string ParameterList(const CppParameters& params,
    const std::function<std::string(const cppast::cpp_type&)>& typeToString, const char* defaultValueNamespaceSeparator)
{
    std::vector<std::string> parts;
    for (const auto& param : params)
    {
        std::string typeString;
        if (typeToString)
            typeString = typeToString(param.type());
        else
            typeString = cppast::to_string(param.type());
        typeString += (" " + EnsureNotKeyword(param.name())).c_str();

        if (defaultValueNamespaceSeparator != nullptr && param.default_value().has_value())
        {
            std::string value = ToString(param.default_value().value());

            // TODO: Ugly band-aid for enum values as default parameter values. Get rid of it ASAP!
            if (strcmp(defaultValueNamespaceSeparator, ".") == 0)
            {
                WeakPtr<MetaEntity> entity;
                if (value == "nullptr")
                    value = "null";
                else if (value == "String::EMPTY")
                    value = "\"\"";
                else if (value == "Variant::EMPTY")
                    value = "";
                else if (value == "Vector3::UP")
                    value = "";
                else if (generator->symbols_.TryGetValue("Urho3D::" + value, entity))
                    value = entity->symbolName_;
                else if (generator->enumValues_.TryGetValue(value, entity))
                    value = entity->symbolName_;
            }
            // ---------------------------------------------------------------------------------------------------------

            if (!value.empty())
                typeString += "=" + str::replace_str(value, "::", defaultValueNamespaceSeparator);
        }
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

std::string ParameterTypeList(const CppParameters& params,
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
        parts.emplace_back(typeString);
    }
    return str::join(parts, ", ");
}

std::string GetConversionType(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::reference_t || type.kind() == cppast::cpp_type_kind::pointer_t)
        return Urho3D::GetTypeName(type);
    else
        return cppast::to_string(type);
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
    const auto* entity = GetEntity(type);
    return entity != nullptr && entity->kind() == cppast::cpp_entity_kind::enum_t;
}

bool IsComplexValueType(const cppast::cpp_type& type)
{
    switch (type.kind())
    {
    case cppast::cpp_type_kind::builtin_t:
        return false;
    case cppast::cpp_type_kind::pointer_t:
        return IsComplexValueType(dynamic_cast<const cppast::cpp_pointer_type&>(type).pointee());
    case cppast::cpp_type_kind::reference_t:
        return IsComplexValueType(dynamic_cast<const cppast::cpp_reference_type&>(type).referee());
    case cppast::cpp_type_kind::user_defined_t:
        return !IsEnumType(type);
    case cppast::cpp_type_kind::cv_qualified_t:
        return IsComplexValueType(dynamic_cast<const cppast::cpp_cv_qualified_type&>(type).type());
    default:
        return true;
    }
}

IncludedChecker::IncludedChecker(const JSONValue& rules)
{
    Load(rules);
}

void IncludedChecker::Load(const JSONValue& rules)
{
    for (const auto& include : rules.Get("include").GetArray())
        includes_.Push(WildcardToRegex(include.GetString()));

    for (const auto& exclude : rules.Get("exclude").GetArray())
        excludes_.Push(WildcardToRegex(exclude.GetString()));
}

bool IncludedChecker::IsIncluded(const String& value)
{
    // Verify item is included
    bool match = false;
    for (const auto& re : includes_)
    {
        match = std::regex_match(value.CString(), re);
        if (match)
            break;
    }

    // Verify header is not excluded
    if (match)
    {
        for (const auto& re : excludes_)
        {
            if (std::regex_match(value.CString(), re))
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

}

namespace str
{

std::string& replace_str(std::string& dest, const std::string& find, const std::string& replace)
{
    while(dest.find(find) != std::string::npos)
        dest.replace(dest.find(find), find.size(), replace);
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

std::string& replace_str(std::string&& dest, const std::string& find, const std::string& replace)
{
    return replace_str(dest, find, replace);
}

}
