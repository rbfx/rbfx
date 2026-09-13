// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
