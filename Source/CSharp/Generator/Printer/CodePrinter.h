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
#include <vector>


namespace Urho3D
{

class CodePrinter
{
public:
    void Indent();
    void Dedent();

    void Write(const std::string& text);
    void WriteLine(const std::string& line="", bool indent=true);
    CodePrinter& operator<<(const std::string& line);
    std::string Get();
    void Flush();

    int indent_ = 0;

protected:
    std::vector<std::string> buffer_;
    std::vector<std::string> lines_;
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

}
