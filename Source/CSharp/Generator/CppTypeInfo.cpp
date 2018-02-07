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

#include <cppast/cpp_type_alias.hpp>
#include <Urho3D/IO/Log.h>
#include "CppTypeInfo.h"


namespace Urho3D
{

CppTypeInfo::CppTypeInfo(const cppast::cpp_type& type)
    : type_(&type)
{
    ParseType(cppast::to_string(type));
}

CppTypeInfo::CppTypeInfo(const String& type)
{
    ParseType(type);
}

void CppTypeInfo::ParseType(const String& type)
{
    // Worst code ever. Do not do this at home...
    fullName_ = type;
    std::vector<std::string> tokens;

    bool nameSaved = false;
    bool nameAlmostSaved = false;
    std::string buffer;
    for (const char* c = fullName_.CString(); c < fullName_.CString() + fullName_.Length() + 1; c++)
    {
        switch (*c)
        {
        case '&':
            notNull_ = true;
        case '*':
            pointer_ = nameAlmostSaved = true;
        case ' ':
        case '\0':
            if (buffer.empty())
                break;

            if (buffer == "const")
            {
                const_ = true;
                nameAlmostSaved = true;
            }
            else if (!nameSaved)
            {
                if (!name_.Empty())
                    name_ += " ";
                name_ += buffer.c_str();
            }
            else
                valid_ = false;
            nameSaved = nameAlmostSaved;
            buffer.clear();
            break;
        default:
            buffer += *c;
            break;
        }
    }

    if (!valid_)
        URHO3D_LOGDEBUGF("CppTypeInfo: Parsing type %s failed.", fullName_.CString());
}

}
