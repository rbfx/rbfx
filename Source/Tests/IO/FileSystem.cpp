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

TEST_CASE("ResolvePath handles /")
{
    // Leading / preservation
    CHECK(ResolvePath("/bla") == "/bla");

    // Single / preservation
    CHECK(ResolvePath("/") == "/");

    // Consecutive / replaced with a single /
    CHECK(ResolvePath("root///bla") == "root/bla");
}

TEST_CASE("ResolvePath handles ..")
{
    // Keep old behaviour for ../ at root position
    CHECK(ResolvePath("../bla") == "bla");

    // Eliminate parent path if it is root
    CHECK(ResolvePath("root/../bla") == "bla");

    // Eliminate parent path
    CHECK(ResolvePath("root/sub/../bla") == "root/bla");

    // Eliminate parent paths when consecutive ..
    CHECK(ResolvePath("root/sub/sub2/../../bla") == "root/bla");

    // Eliminate parent paths when consecutive ..
    CHECK(ResolvePath("root/sub/../../bla") == "bla");

    // Eliminate trailing ..
    CHECK(ResolvePath("root/bla/..") == "root");
}

TEST_CASE("ResolvePath handles .")
{
    // Keep old behaviour for ./ at root position
    CHECK(ResolvePath("./bla") == "bla");

    // Eliminate parent path if it is root
    CHECK(ResolvePath("root/./bla") == "root/bla");

    // Eliminate trailing dot
    CHECK(ResolvePath("bla/.") == "bla");
}
