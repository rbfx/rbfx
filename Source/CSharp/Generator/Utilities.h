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


namespace Urho3D
{

struct UserData;

std::regex WildcardToRegex(const String& wildcard);
UserData* GetUserData(const cppast::cpp_entity& e);
String GetSymbolName(const cppast::cpp_entity& e);
bool IsConstructor(const cppast::cpp_entity& e);
bool IsDestructor(const cppast::cpp_entity& e);
/// Ensure arbitrary string is a valid identifier by replacing invalid characters with "_". "_" will be prepended if string starts with a number.
String Sanitize(const String& value);
bool IsVoid(const cppast::cpp_type& type);

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


}
