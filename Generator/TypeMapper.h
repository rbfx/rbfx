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


#include <string>
#include <unordered_map>
#include <Urho3D/Container/HashMap.h>
#include <cppast/cpp_type.hpp>


namespace Urho3D
{

struct TypeMap
{
    std::string cppType_ = "void*";
    std::string cType_ = "void*";
    std::string csType_ = "";
    std::string pInvokeType_ = "IntPtr";
    std::string cToCppTemplate_ = "{{value}}";
    std::string cppToCTemplate_ = "{{value}}";
    std::string csToPInvokeTemplate_ = "{{value}}";
    std::string pInvokeToCSTemplate_ = "{{value}}";
};

class TypeMapper : public Object
{
    URHO3D_OBJECT(TypeMapper, Object)

    TypeMapper(Context* context);;
public:
    void Load(XMLFile* rules);
    const TypeMap* GetTypeMap(const cppast::cpp_type& type);
    const TypeMap* GetTypeMap(const std::string& typeName);

    std::string ToCType(const cppast::cpp_type& type);
    std::string ToCSType(const cppast::cpp_type& type);
    std::string ToPInvokeTypeReturn(const cppast::cpp_type& type);
    std::string ToPInvokeTypeParam(const cppast::cpp_type& type);

    std::string MapToC(const cppast::cpp_type& type, const std::string& expression);
    std::string MapToCNoCopy(const std::string& type, const std::string& expression);
    std::string MapToCpp(const cppast::cpp_type& type, const std::string& expression);
    std::string MapToPInvoke(const cppast::cpp_type& type, const std::string& expression);
    std::string MapToCS(const cppast::cpp_type& type, const std::string& expression);

    std::string ToPInvokeType(const std::string& name, const std::string& default_="IntPtr");
    std::string ToPInvokeType(const cppast::cpp_type& type, const std::string& default_="IntPtr");

    std::unordered_map<std::string, TypeMap> typeMaps_;
};

}
