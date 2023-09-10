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

TEST_CASE("Empty FileIdentifier is checked and created")
{
    REQUIRE_FALSE(static_cast<bool>(FileIdentifier::Empty));
    REQUIRE(FileIdentifier::Empty.IsEmpty());

    REQUIRE(static_cast<bool>(FileIdentifier{"file", "/path/to/file"}));
    REQUIRE_FALSE(FileIdentifier{"file", "/path/to/file"}.IsEmpty());

    REQUIRE(FileIdentifier{} == FileIdentifier::Empty);
    REQUIRE(FileIdentifier::FromUri("") == FileIdentifier::Empty);
    REQUIRE(FileIdentifier{"", ""} == FileIdentifier::Empty);
    REQUIRE(FileIdentifier{"file", "/path/to/file"} != FileIdentifier::Empty);
    REQUIRE(FileIdentifier{"", "/path/to/file"} != FileIdentifier::Empty);
    REQUIRE(FileIdentifier{"file", ""} != FileIdentifier::Empty);
}

TEST_CASE("FileIdentifier is created from URI")
{
    // Absolute path
    CHECK(FileIdentifier::FromUri("/path/to/file") == FileIdentifier{"file", "/path/to/file"});
    CHECK(FileIdentifier::FromUri("c:/path/to/file") == FileIdentifier{"file", "c:/path/to/file"});
    CHECK(FileIdentifier::FromUri("D:/path/to/file") == FileIdentifier{"file", "D:/path/to/file"});

    // No scheme
    CHECK(FileIdentifier::FromUri("relative/path/to/file") == FileIdentifier{"", "relative/path/to/file"});
    CHECK(FileIdentifier::FromUri("filename") == FileIdentifier{"", "filename"});

    // file scheme
    CHECK(FileIdentifier::FromUri("file:path/to/file") == FileIdentifier::Empty);
    CHECK(FileIdentifier::FromUri("file:/path/to/file") == FileIdentifier{"file", "/path/to/file"});
    CHECK(FileIdentifier::FromUri("file://path/to/file") == FileIdentifier{"file", "/path/to/file"});
    CHECK(FileIdentifier::FromUri("file:///path/to/file") == FileIdentifier{"file", "/path/to/file"});
    CHECK(FileIdentifier::FromUri("file:////path/to/file") == FileIdentifier::Empty);
    CHECK(FileIdentifier::FromUri("file:/c:/path/to/file") == FileIdentifier{"file", "c:/path/to/file"});
    CHECK(FileIdentifier::FromUri("file://c:/path/to/file") == FileIdentifier{"file", "c:/path/to/file"});
    CHECK(FileIdentifier::FromUri("file:///c:/path/to/file") == FileIdentifier{"file", "c:/path/to/file"});
    CHECK(FileIdentifier::FromUri("file:////c:/path/to/file") == FileIdentifier::Empty);

    // other schemes
    CHECK(FileIdentifier::FromUri("http://example.com/a/b/c") == FileIdentifier{"http", "example.com/a/b/c"});
    CHECK(FileIdentifier::FromUri("conf://config.json") == FileIdentifier{"conf", "config.json"});
    CHECK(FileIdentifier::FromUri("conf:config.json") == FileIdentifier{"conf", "config.json"});
}

TEST_CASE("FileIdentifier is converted to URI")
{
    // No scheme
    CHECK(FileIdentifier{"", "relative/path/to/file"}.ToUri() == "relative/path/to/file");
    CHECK(FileIdentifier{"", "file"}.ToUri() == "file");

    // file scheme
    CHECK(FileIdentifier{"file", "/path/to/file"}.ToUri() == "file:///path/to/file");
    CHECK(FileIdentifier{"file", "c:/path/to/file"}.ToUri() == "file:///c:/path/to/file");

    // other schemes
    CHECK(FileIdentifier{"http", "example.com/a/b/c"}.ToUri() == "http://example.com/a/b/c");
    CHECK(FileIdentifier{"conf", "config.json"}.ToUri() == "conf://config.json");
}

TEST_CASE("String is appended to FileIdentifier")
{
    CHECK(FileIdentifier{"", ""} + "" == FileIdentifier{"", ""});

    CHECK(FileIdentifier{"", ""} + "path" == FileIdentifier{"", "path"});
    CHECK(FileIdentifier{"", "path"} + "" == FileIdentifier{"", "path"});

    CHECK(FileIdentifier{"", "path"} + "to" == FileIdentifier{"", "path/to"});
    CHECK(FileIdentifier{"", "path/"} + "to" == FileIdentifier{"", "path/to"});
    CHECK(FileIdentifier{"", "path"} + "/to" == FileIdentifier{"", "path/to"});
    CHECK(FileIdentifier{"", "path/"} + "/to" == FileIdentifier{"", "path/to"});

    CHECK(FileIdentifier{"", "path"} + "to/file" == FileIdentifier{"", "path/to/file"});
    CHECK(FileIdentifier{"", "path/"} + "to/file" == FileIdentifier{"", "path/to/file"});
    CHECK(FileIdentifier{"", "path"} + "/to/file" == FileIdentifier{"", "path/to/file"});
    CHECK(FileIdentifier{"", "path/"} + "/to/file" == FileIdentifier{"", "path/to/file"});

    CHECK(FileIdentifier{"", "path"} + "to/file/" == FileIdentifier{"", "path/to/file/"});
    CHECK(FileIdentifier{"", "path/"} + "to/file/" == FileIdentifier{"", "path/to/file/"});
    CHECK(FileIdentifier{"", "path"} + "/to/file/" == FileIdentifier{"", "path/to/file/"});
    CHECK(FileIdentifier{"", "path/"} + "/to/file/" == FileIdentifier{"", "path/to/file/"});
}

TEST_CASE("SanitizeFileName handles ..")
{
    // Keep old behaviour for ../ at root position
    CHECK(FileIdentifier{"../bla"} == FileIdentifier{"", "bla"});

    // Eliminate parent path if it is root
    CHECK(FileIdentifier{"root/../bla"} == FileIdentifier{"", "bla"});

    // Eliminate parent path
    CHECK(FileIdentifier{"root/sub/../bla"} == FileIdentifier{"", "root/bla"});

    // Eliminate parent paths when consecutive ..
    CHECK(FileIdentifier{"root/sub/sub2/../../bla"} == FileIdentifier{"", "root/bla"});

    // Eliminate parent paths when consecutive ..
    CHECK(FileIdentifier{"root/sub/../../bla"} == FileIdentifier{"", "bla"});
}

TEST_CASE("SanitizeFileName handles .")
{
    // Keep old behaviour for ../ at root position
    CHECK(FileIdentifier{"./bla"} == FileIdentifier{"", "bla"});

    // Eliminate parent path if it is root
    CHECK(FileIdentifier{"root/./bla"} == FileIdentifier{"", "root/bla"});
}
