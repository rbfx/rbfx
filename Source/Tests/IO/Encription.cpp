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

#include <Urho3D/IO/Encription.h>
#include <Urho3D/IO/VectorBuffer.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

namespace
{
    const static ea::string LongMessage("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
}

TEST_CASE("Encription roundtrip")
{
    EncryptionKey key = GenerateSymmetricEncryptionKey();
    unsigned char encData[1024];
    unsigned char decData[1024];
    unsigned encSize = EncryptData(encData, LongMessage.data(), LongMessage.size(), key);
    unsigned decSize = DecryptData(decData, encData, encSize, key);

    const ea::string decriptedMessage(reinterpret_cast<char*>(decData), reinterpret_cast<char*>(decData + decSize));

    REQUIRE(LongMessage == decriptedMessage);
}

TEST_CASE("Encrypted Stream Serializer and Deserializer")
{
    EncryptionKey key = GenerateSymmetricEncryptionKey();

    VectorBuffer encrypted;
    EncryptedStreamSerializer serializer(encrypted, key, EncryptionNonce::ZERO, 32);

    ea::string message = GENERATE(
        LongMessage,
        ea::string(),
        ea::string("Short message")
        );

    REQUIRE(serializer.Write(message.data(), message.size()) == message.size());
    REQUIRE(serializer.Flush());

    encrypted.Seek(0);
    EncryptedStreamDeserializer reader(encrypted, key, EncryptionNonce::ZERO);
    ea::string streamMessage; streamMessage.resize(message.size());
    // Let's read beyond original content size. The deserializer should return exactly what we stored.
    unsigned readed = reader.Read(streamMessage.data(), message.size()+32);
    REQUIRE(readed == message.size());
    REQUIRE(streamMessage == message);
}
