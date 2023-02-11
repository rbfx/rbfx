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

#include <Urho3D/IO/FileIdentifier.h>

TEST_CASE("FileLocator")
{
    FileIdentifier singleForwardSlash("file:/FileName");
    CHECK(singleForwardSlash.scheme_ == "file");
    CHECK(singleForwardSlash.fileName_ == "FileName");

    FileIdentifier twoForwardSlash("file://FileName");
    CHECK(twoForwardSlash.scheme_ == "file");
    CHECK(twoForwardSlash.fileName_ == "FileName");

    FileIdentifier threeForwardSlash("file:///FileName");
    CHECK(threeForwardSlash.scheme_ == "file");
    CHECK(threeForwardSlash.fileName_ == "FileName");

    FileIdentifier noSchema("Dir/FileName");
    CHECK(noSchema.scheme_ == "");
    CHECK(noSchema.fileName_ == "Dir/FileName");

    FileIdentifier dotSlash("./Dir/FileName");
    CHECK(dotSlash.scheme_ == "");
    CHECK(dotSlash.fileName_ == "Dir/FileName");

    FileIdentifier backSlash("Dir\\SubDir\\FileName");
    CHECK(backSlash.scheme_ == "");
    CHECK(backSlash.fileName_ == "Dir/SubDir/FileName");
}
