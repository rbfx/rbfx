//
// Copyright (c) 2023 the rbfx project.
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

#include "../CommonUtils.h"

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/Resource.h>

namespace Tests
{

namespace
{

template <unsigned N>
InternalResourceFormat GetFormat(
    const char (&buffer)[N], unsigned offset = 0, BinaryMagic binaryMagic = DefaultBinaryMagic)
{
    MemoryBuffer memoryBuffer(buffer, N - 1);
    memoryBuffer.Seek(offset);
    const auto format = PeekResourceFormat(memoryBuffer, binaryMagic);
    CHECK(memoryBuffer.Tell() == offset);
    return format;
}

}

TEST_CASE("InternalResourceFormat is peeked in Deserializer")
{
    CHECK(GetFormat("\0BI") == InternalResourceFormat::Unknown);
    CHECK(GetFormat("X0\3A") == InternalResourceFormat::Unknown);
    CHECK(GetFormat("TEXT") == InternalResourceFormat::Unknown);
    CHECK(GetFormat("{") == InternalResourceFormat::Unknown);
    CHECK(GetFormat("<a>") == InternalResourceFormat::Unknown);

    CHECK(GetFormat("\0BIN") == InternalResourceFormat::Binary);
    CHECK(GetFormat("\0BIN1234") == InternalResourceFormat::Binary);
    CHECK(GetFormat("1234\0BIN1234", 4) == InternalResourceFormat::Binary);
    CHECK(GetFormat("1234\0BOB1234", 4, {{'\0', 'B', 'O', 'B'}}) == InternalResourceFormat::Binary);

    CHECK(GetFormat("{a") == InternalResourceFormat::Json);
    CHECK(GetFormat("{}") == InternalResourceFormat::Json);
    CHECK(GetFormat("{\"a\":1}") == InternalResourceFormat::Json);
    CHECK(GetFormat(" {\"a\":1}") == InternalResourceFormat::Json);
    CHECK(GetFormat("\n {\"a\":1}") == InternalResourceFormat::Json);
    CHECK(GetFormat("\t\n {\"a\":1}") == InternalResourceFormat::Json);
    CHECK(GetFormat("\t\n\r\t\n    {\"a\":1}") == InternalResourceFormat::Json);
    CHECK(GetFormat("1234{}", 4) == InternalResourceFormat::Json);

    CHECK(GetFormat("<a/>") == InternalResourceFormat::Xml);
    CHECK(GetFormat(" <a t=\"1\"></a>") == InternalResourceFormat::Xml);
    CHECK(GetFormat("\n <a t=\"1\"></a>") == InternalResourceFormat::Xml);
    CHECK(GetFormat("\t\n <a t=\"1\"></a>") == InternalResourceFormat::Xml);
    CHECK(GetFormat("\t\n\r\t\n    <a t=\"1\"></a>") == InternalResourceFormat::Xml);
    CHECK(GetFormat("1234<a/>", 4) == InternalResourceFormat::Xml);
}

} // namespace Tests
