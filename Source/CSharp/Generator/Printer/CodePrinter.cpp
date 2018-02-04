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

void CodePrinter::Write(const String& text)
{
    buffer_ += text;
}

void CodePrinter::WriteLine(const String& line)
{
    Flush();
    lines_ += (String(' ', indent_ * 4) + line);
}

CodePrinter& CodePrinter::operator<<(const String& line)
{
    WriteLine(line);
    return *this;
}

void CodePrinter::Flush()
{
    if (!buffer_.Empty())
    {
        lines_ += (String(' ', indent_ * 4) + String::Joined(buffer_, ""));
        buffer_.Clear();
    }
}

String CodePrinter::Get()
{
    Flush();
    String text = String::Joined(lines_, "\n");
    lines_.Clear();
    return text;
}

CodePrinter& CodePrinter::operator<(const String& text)
{
    Write(text);
    return *this;
}

}
