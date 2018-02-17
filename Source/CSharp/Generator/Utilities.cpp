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

String GetSymbolName(const cppast::cpp_entity& e)
{
    Vector<String> elements = {e.name()};
    type_safe::optional_ref<const cppast::cpp_entity> ref(e.parent());
    while (ref.has_value())
    {
        if (!cppast::is_templated(ref.value()) && !cppast::is_friended(ref.value()) && ref.value().kind() != cppast::cpp_entity_kind::file_t)
        {
            const auto& scope = ref.value().scope_name();
            if (scope.has_value())
                elements.Push(scope.value().name().c_str());
        }
        ref = ref.value().parent();
    }

    // Reverse
    for (auto i = 0; i < elements.Size() / 2; i++)
        Swap(elements[i], elements[elements.Size() - i - 1]);

    return String::Joined(elements, "::");
}

String GetSymbolName(const cppast::cpp_entity* e)
{
    return GetSymbolName(*e);
}

String Sanitize(const String& value)
{
    String result = std::regex_replace(value.CString(), std::regex("[^a-zA-Z0-9_]"), "_").c_str();
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

String EnsureNotKeyword(const String& value)
{
    if (value == "object")
        return value + "_";
    return value;
}

String ParameterList(const cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>& params,
    const std::function<String(const cppast::cpp_type&)>& typeToString)
{
    Vector<String> parts;
    for (const auto& param : params)
    {
        String typeString;
        if (typeToString)
            typeString = typeToString(param.type());
        else
            typeString = cppast::to_string(param.type());
        parts.Push(typeString + " " + EnsureNotKeyword(param.name()));
    }
    return String::Joined(parts, ", ");
}

String ParameterNameList(const cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>& params,
    const std::function<String(const cppast::cpp_function_parameter&)>& nameFilter)
{
    Vector<String> parts;
    for (const auto& param : params)
    {
        String name = EnsureNotKeyword(param.name());
        if (nameFilter)
            name = nameFilter(param);
        parts.Push(name);
    }
    return String::Joined(parts, ", ");
}

String ParameterTypeList(const cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>& params,
    const std::function<String(const cppast::cpp_type&)>& typeToString)
{
    Vector<String> parts;
    for (const auto& param : params)
    {
        String typeString;
        if (typeToString)
            typeString = typeToString(param.type());
        else
            typeString = cppast::to_string(param.type());
        parts.Push(typeString);
    }
    return String::Joined(parts, ", ");
}

String GetConversionType(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::reference_t || type.kind() == cppast::cpp_type_kind::pointer_t)
        return Urho3D::GetTypeName(type);
    else
        return cppast::to_string(type);
}

String GetTypeName(const cppast::cpp_type& type)
{
    switch (type.kind())
    {
    case cppast::cpp_type_kind::cv_qualified_t:
        return cppast::to_string(static_cast<const cppast::cpp_cv_qualified_type&>(type).type());
    case cppast::cpp_type_kind::pointer_t:
        return cppast::to_string(static_cast<const cppast::cpp_pointer_type&>(type).pointee());
    case cppast::cpp_type_kind::reference_t:
        return cppast::to_string(static_cast<const cppast::cpp_reference_type&>(type).referee());
    default:
        return cppast::to_string(type);
    }
}

bool IsEnumType(const cppast::cpp_type& type)
{
    if (type.kind() == cppast::cpp_type_kind::user_defined_t)
    {
        const auto& entity = static_cast<const cppast::cpp_user_defined_type&>(type).entity();
        const auto& ref = entity.get(generator->index_);
        if (!ref.empty())
        {
            const auto& definition = ref[0u].get();
            if (definition.kind() == cppast::cpp_entity_kind::enum_t)
                return true;
        }
    }
    return false;
}

bool IsComplexValueType(const cppast::cpp_type& type)
{
    switch (type.kind())
    {
    case cppast::cpp_type_kind::builtin_t:
    case cppast::cpp_type_kind::pointer_t:
    case cppast::cpp_type_kind::reference_t:
        return false;
    case cppast::cpp_type_kind::user_defined_t:
        return !IsEnumType(type);
    default:
        return true;
    }
}

IncludedChecker::IncludedChecker(const XMLElement& rules)
{
    Load(rules);
}

void IncludedChecker::Load(const XMLElement& rules)
{
    for (XMLElement include = rules.GetChild("include"); include.NotNull(); include = include.GetNext("include"))
        includes_.Push(WildcardToRegex(include.GetValue()));

    for (XMLElement exclude = rules.GetChild("exclude"); exclude.NotNull(); exclude = exclude.GetNext("exclude"))
        excludes_.Push(WildcardToRegex(exclude.GetValue()));

    // "Manual" stuff is also excluded. We guarantee that user implementation will exist.
    for (XMLElement manual = rules.GetChild("manual"); manual.NotNull(); manual = manual.GetNext("manual"))
        excludes_.Push(WildcardToRegex(manual.GetValue()));
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
}
