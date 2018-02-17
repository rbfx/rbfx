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

#pragma once


#include <regex>
#include <Urho3D/Container/Str.h>
#include <cppast/cpp_entity.hpp>
#include <Urho3D/Resource/XMLElement.h>
#include <cppast/cpp_type.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_member_function.hpp>


namespace Urho3D
{

/// Convert a wildcard string to regular expression. "*" matches anything except /, "**" matches everything including /.
std::regex WildcardToRegex(const String& wildcard);
/// Returns entity name including names of it's parents (separated by ::).
String GetSymbolName(const cppast::cpp_entity& e);
/// Returns entity name including names of it's parents (separated by ::).
String GetSymbolName(const cppast::cpp_entity* e);
/// Ensure arbitrary string is a valid identifier by replacing invalid characters with "_". "_" will be prepended if string starts with a number.
String Sanitize(const String& value);
/// Returns true if type is void.
bool IsVoid(const cppast::cpp_type& type);
/// Returns string padded with _ if value is a common keyword in programming languages.
String EnsureNotKeyword(const String& value);
/// Return name of underlying type.
String GetTypeName(const cppast::cpp_type& type);

class IncludedChecker
{
public:
    IncludedChecker() = default;
    /// Initialize with XMLElement that consists of <include> and <exclude> children tags with wildcards as their values.
    explicit IncludedChecker(const XMLElement& rules);
    /// Initialize with XMLElement that consists of <include> and <exclude> children tags with wildcards as their values.
    void Load(const XMLElement& rules);
    /// Verify string is matched by include rules and not matched by exclude rules.
    bool IsIncluded(const String& value);

protected:
    Vector<std::regex> includes_;
    Vector<std::regex> excludes_;
};
/// Returns a list of parameter types and names as if they were in a function declaration.
String ParameterList(const cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>& params,
    const std::function<String(const cppast::cpp_type&)>& typeToString=nullptr);
/// Returns a list of parameter names separated by comma.
String ParameterNameList(const cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>& params,
    const std::function<String(const cppast::cpp_function_parameter&)>& nameFilter=nullptr);
/// Returns a list of parameter types separated comma. Useful for creating function signatures.
String ParameterTypeList(const cppast::detail::iteratable_intrusive_list<cppast::cpp_function_parameter>& params,
    const std::function<String(const cppast::cpp_type&)>& typeToString=nullptr);
/// Returns a type string which is used as template parameter for CSharpTypeConverter<> struct.
String GetConversionType(const cppast::cpp_type& type);
/// Returns true if specified type is an enumeration.
bool IsEnumType(const cppast::cpp_type& type);
/// Returns true if a type is non-builtin value type (not a pointer or reference to a struct/class).
bool IsComplexValueType(const cppast::cpp_type& type);

}
