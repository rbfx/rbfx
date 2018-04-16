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

#include "CodePrinter.h"
#include "Utilities.h"


namespace Urho3D
{

void CodePrinter::Indent()
{
    indent_++;
}

void CodePrinter::Dedent()
{
    indent_--;
}

void CodePrinter::Write(const std::string& text)
{
    buffer_.emplace_back(text);
    if (text.back() == '\n')
        Flush();
}

void CodePrinter::WriteLine(const std::string& line, bool indent)
{
    Flush();
    lines_.emplace_back(std::string(indent ? indent_ * 4 : 0, ' ') + line);
}

CodePrinter& CodePrinter::operator<<(const std::string& line)
{
    WriteLine(line);
    return *this;
}

void CodePrinter::Flush()
{
    if (!buffer_.empty())
    {
        lines_.emplace_back(std::string(indent_ * 4, ' ') + str::join(buffer_, ""));
        buffer_.clear();
    }
}

std::string CodePrinter::Get()
{
    Flush();
    auto text = str::join(lines_, "\n");
    lines_.clear();
    return text;
}

}
