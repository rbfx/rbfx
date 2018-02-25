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


#include <Urho3D/Container/Str.h>
#include <Mustache/mustache.hpp>


namespace Urho3D
{

class CodePrinter
{
public:
    void Indent();
    void Dedent();

    void Write(const String& text);
    void WriteLine(const String& line="", bool indent=true);
    CodePrinter& operator<<(const String& line);

    String Get();

    void Flush();

protected:

    int indent_ = 0;
    Vector<String> buffer_;
    Vector<String> lines_;
};

class PrinterScope
{
public:
    explicit PrinterScope(CodePrinter& printer)
        : printer_(printer)
    {
        printer_.Indent();
    }

    ~PrinterScope()
    {
        printer_.Dedent();
    }

protected:
    CodePrinter& printer_;
};

mustache::data fmt(const std::initializer_list<std::pair<std::string, mustache::data>>& params);
std::string fmt(const char* format, const std::initializer_list<std::pair<std::string, mustache::data>>& params);
std::string fmt(const char* format, const mustache::data& params);

}
