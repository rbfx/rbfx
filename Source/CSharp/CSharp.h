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


#include <Urho3D/Urho3DAll.h>

using namespace Urho3D;


template<typename T>
struct CSharpTypeConverter
{
    using CppType = T;
    using CType = T;

    static CType toC(CppType value, bool byCopy=false) { return value; }
    static CppType toCpp(CType value) { return value; }
};

template<>
struct CSharpTypeConverter<String>
{
    using CppType = String;
    using CType = const char*;

    // Returning pointer to a const reference is safe
    static const char* toC(const String& value, bool byCopy=false)
    {
        if (byCopy)
            return strdup(value.CString());
        return value.CString();
    }
    static String toCpp(const char* value) { return value; }
};

template<>
struct CSharpTypeConverter<StringHash>
{
    using CppType = StringHash;
    using CType = unsigned;

    static unsigned toC(const StringHash& value, bool byCopy=false) { return value.Value(); }
    static StringHash toCpp(unsigned value) { return StringHash(value); }
};
