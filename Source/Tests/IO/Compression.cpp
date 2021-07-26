//
// Copyright (c) 2017-2021 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/IO/Compression.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

namespace 
{
    const static ea::string LongMessage("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
}

TEST_CASE("Compress stream roundtrip")
{
    ea::string message = GENERATE(
        ea::string(),
        ea::string("Short message"),
        LongMessage);
    
    MemoryBuffer source(message.data(), message.size());
    VectorBuffer compressed;
    VectorBuffer decompressed;

    REQUIRE(CompressStream(compressed, source, 32));
    REQUIRE(DecompressStream(decompressed, compressed));
    ea::string receivedMessage(message.data(), message.size());
    REQUIRE(receivedMessage == message);
}

TEST_CASE("Compressed Stream Serializer and Deserializer")
{
    VectorBuffer compressed;
    CompressedStreamSerializer serializer(compressed, 32);

    ea::string message = GENERATE(
        ea::string(),
        ea::string("Short message"),
        LongMessage);

    REQUIRE(serializer.Write(message.data(), message.size()) == message.size());
    REQUIRE(serializer.Flush());

    compressed.Seek(0);
    CompressedStreamDeserializer deserializer(compressed);
    ea::string streamMessage; streamMessage.resize(message.size());
    // Let's read beyond original content size. The deserializer should return exactly what we stored.
    unsigned readed = deserializer.Read(streamMessage.data(), message.size() + 32);
    REQUIRE(readed == message.size());
    REQUIRE(streamMessage == message);
}

TEST_CASE("Compress stream roundtrip via reader")
{
    ea::string message = GENERATE(
        ea::string("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaLorem ipsum dolor sit amet, consectetur adipiscing elit"),
        ea::string(),
        ea::string("Short message"),
        LongMessage);

    MemoryBuffer source(message.data(), message.size());
    VectorBuffer compressed;
    REQUIRE(CompressStream(compressed, source, 32));
    compressed.Seek(0);

    CompressedStreamDeserializer reader(compressed);
    ea::string streamMessage; streamMessage.resize(message.size());
    char* ptr = streamMessage.data();
    unsigned left = message.size();
    do
    {
        const unsigned blockSize = GENERATE(17, 32, 1);
        unsigned expectedReadSize = std::min(left, blockSize);
        unsigned readed = reader.Read(ptr, blockSize);
        REQUIRE(readed == expectedReadSize);
        ptr += expectedReadSize;
        left -= expectedReadSize;
    } while (left != 0);
    REQUIRE(streamMessage == message);
}

TEST_CASE("CompressedStreamDeserializer::Seek tests")
{
    {
        constexpr unsigned offsetSize = 12;
        MemoryBuffer source(LongMessage.data(), LongMessage.size());
        VectorBuffer compressed;
        for (unsigned i = 0; i < offsetSize; ++i)
            compressed.WriteByte(0);
        REQUIRE(CompressStream(compressed, source, 32));


        SECTION("Seek to a known chunk and read")
        {
            compressed.Seek(offsetSize);
            CompressedStreamDeserializer deserializer(compressed);

            std::pair<unsigned, unsigned> positions[]
            {
                // Complete message to populate list of chunks
                std::make_pair(0, LongMessage.size()),
                // Second chunk exactly
                std::make_pair(32, 32),
                // Data from to chunks starting from within chunk
                std::make_pair(38, 32),
                // Last chunk completely
                std::make_pair(32*(LongMessage.size()/32), LongMessage.size() - 32 * (LongMessage.size() / 32)),
                // Last portion
                std::make_pair(LongMessage.size()-1, 1),
                // Zero bytes beyond the message
                std::make_pair(LongMessage.size(), 0),
            };
            for (auto posAndSize: positions)
            {
                REQUIRE(deserializer.Seek(posAndSize.first) == posAndSize.first);
                ea::string buf;
                buf.resize(posAndSize.second);
                REQUIRE(deserializer.Read(buf.data(), posAndSize.second) == posAndSize.second);
                const auto expected = LongMessage.substr(posAndSize.first, posAndSize.second);
                REQUIRE(expected == buf);
            }
        }

        SECTION("Seek to an unknown chunk and read")
        {
            std::pair<unsigned, unsigned> positions[]
            {
                // Complete message to populate list of chunks
                std::make_pair(0, LongMessage.size()),
                // Second chunk exactly
                std::make_pair(32, 32),
                // Data from to chunks starting from within chunk
                std::make_pair(38, 32),
                // Last chunk completely
                std::make_pair(32 * (LongMessage.size() / 32), LongMessage.size() - 32 * (LongMessage.size() / 32)),
                // Last portion
                std::make_pair(LongMessage.size() - 1, 1),
                // Zero bytes beyond the message
                std::make_pair(LongMessage.size(), 0),
            };
            for (auto posAndSize : positions)
            {
                compressed.Seek(offsetSize);
                CompressedStreamDeserializer deserializer(compressed);

                REQUIRE(deserializer.Seek(posAndSize.first) == posAndSize.first);
                ea::string buf;
                buf.resize(posAndSize.second);
                REQUIRE(deserializer.Read(buf.data(), posAndSize.second) == posAndSize.second);
                const auto expected = LongMessage.substr(posAndSize.first, posAndSize.second);
                REQUIRE(expected == buf);
            }
        }
    }
}
