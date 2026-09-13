// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
