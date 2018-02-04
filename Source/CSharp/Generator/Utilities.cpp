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
#include "Utilities.h"
#include "GeneratorContext.h"


namespace Urho3D
{

std::regex WildcardToRegex(const Urho3D::String& wildcard)
{
    // Wildcard is converted to regex
    String regex = wildcard;

    // Escape regex characters except for *
    const char* special = "\\.^$|()[]{}+?";
    for (char c = *special; c != 0; c = *(++special))
        regex.Replace(String(c), ToString("\\%c", c));

    // Replace wildcard characters
    regex.Replace("**", ".+");
    regex.Replace("*", "[^/]+");
    regex = "^" + regex + "$";

    return std::regex(regex.CString());
}

UserData* GetUserData(const cppast::cpp_entity& e)
{
    auto* result = static_cast<UserData*>(e.user_data());
    if (result == nullptr)
    {
        result = new UserData{};
        e.set_user_data(result);
    }
    return result;
}

String GetSymbolName(const cppast::cpp_entity& e)
{
    Vector<String> elements = { e.name().c_str() };

    auto parentRef = e.parent();

    while (parentRef.has_value())
    {
        if (parentRef.value().kind() == cppast::cpp_entity_kind::class_t ||
            parentRef.value().kind() == cppast::cpp_entity_kind::namespace_t)
        {
            elements.Push(parentRef.value().name().c_str());
            parentRef = parentRef.value().parent();
        }
        else
            break;
    }

    // Reverse
    for (auto i = 0; i < elements.Size() / 2; i++)
        Swap(elements[i], elements[elements.Size() - i - 1]);

    return String::Joined(elements, "::");
}

bool IsConstructor(const cppast::cpp_entity& e)
{
    if (e.kind() == cppast::cpp_entity_kind::member_function_t)
        return e.name() == e.parent().value().name();
    return false;
}

bool IsDestructor(const cppast::cpp_entity& e)
{
    if (e.kind() == cppast::cpp_entity_kind::member_function_t)
        return e.name()[0] == '~';
    return false;
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
    return cppast::to_string(type) == "void";
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
