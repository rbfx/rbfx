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
#include <Urho3D/Container/HashMap.h>
#include <cppast/cpp_type.hpp>


namespace Urho3D
{

struct TypeMap
{
    String cppType_ = "void*";
    String cType_ = "void*";
    String csType_ = "IntPtr";
    String pInvokeType_ = "IntPtr";
    String cToCppTemplate_ = "{{value}}";
    String cppToCTemplate_ = "{{value}}";
    String copyTemplate_ = "{{value}}";
    String csToPInvokeTemplate_ = "{{value}}";
    String pInvokeToCSTemplate_ = "{{value}}";
    String copyToCSTemplate_ = "{{value}}";
};

class TypeMapper : public Object
{
    URHO3D_OBJECT(TypeMapper, Object)

    TypeMapper(Context* context);;
public:
    void Load(XMLFile* rules);
    const TypeMap* GetTypeMap(const cppast::cpp_type& type);
    const TypeMap* GetTypeMap(const String& typeName);

    String ToCType(const cppast::cpp_type& type);
    String ToCSType(const cppast::cpp_type& type);
    String ToPInvokeTypeReturn(const cppast::cpp_type& type, bool safe);
    String ToPInvokeTypeParam(const cppast::cpp_type& type);

    String MapToC(const cppast::cpp_type& type, const String& expression, bool canCopy);
    String MapToCpp(const cppast::cpp_type& type, const String& expression);
    String MapToPInvoke(const cppast::cpp_type& type, const String& expression);
    String MapToCS(const cppast::cpp_type& type, const String& expression, bool canCopy);

protected:
    String ToPInvokeType(const String& name, const String& default_="IntPtr");
    String ToPInvokeType(const cppast::cpp_type& type, const String& default_="IntPtr");

    HashMap<String, TypeMap> typeMaps_;
};

}
