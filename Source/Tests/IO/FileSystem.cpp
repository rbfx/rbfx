//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "../CommonUtils.h"

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/VectorBuffer.h>

using namespace Urho3D;

TEST_CASE("Write file and read it back")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto fs = context->GetSubsystem<FileSystem>();

    ea::string fileName = "53A296C7691D4148AB642567765D3497.txt";
    {
        auto fileToWrite = fs->OpenFile(fileName, FILE_WRITE);
        REQUIRE((fileToWrite && fileToWrite->IsOpen() && !fileToWrite->CanRead()));
        fileToWrite->Write(fileName.c_str(), fileName.length());
    }

    {
        auto fileToRead = fs->OpenFile(fileName, FILE_READ);
        REQUIRE((fileToRead && fileToRead->IsOpen() && fileToRead->CanRead()));
        REQUIRE(fileToRead->GetSize() == fileName.length());
        ea::vector<char> buf;
        buf.resize(fileName.length());
        CHECK(buf.size() == fileToRead->Read(buf.data(), buf.size()));
        ea::string actualData(reinterpret_cast<const char*>(buf.data()), buf.size());
        CHECK(actualData == fileName);
    }

    {
        auto fileToRead = fs->OpenFile(fileName, FILE_READ);
        REQUIRE((fileToRead && fileToRead->IsOpen() && fileToRead->CanRead()));
        REQUIRE(fileToRead->GetSize() == fileName.length());
        fileToRead->Seek(5);
        ea::vector<char> buf;
        buf.resize(fileName.length()-5);
        CHECK(buf.size() == fileToRead->Read(buf.data(), buf.size()));
        ea::string actualData(reinterpret_cast<const char*>(buf.data()), buf.size());
        CHECK(actualData == fileName.substr(5, buf.size()));
    }

    CHECK(fs->Delete(fileName));
}
