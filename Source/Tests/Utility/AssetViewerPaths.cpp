// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

#include "../CommonUtils.h"

#include "../../Tools/AssetViewer/DroppedPath.h"

using namespace Urho3D;

TEST_CASE("AssetViewer normalizes dropped paths")
{
    using AssetViewerDetail::NormalizeDroppedPath;
    const ea::string cwd = "/work/dir/";

    CHECK(NormalizeDroppedPath("/a/b.fbx", cwd) == "/a/b.fbx");
    CHECK(NormalizeDroppedPath("b.fbx", cwd) == "/work/dir/b.fbx");
    CHECK(NormalizeDroppedPath("  /a/b.fbx\r\n", cwd) == "/a/b.fbx");
    CHECK(NormalizeDroppedPath("\"/a/b c.fbx\"", cwd) == "/a/b c.fbx");
    CHECK(NormalizeDroppedPath("'/a/b.fbx'\n", cwd) == "/a/b.fbx");
    CHECK(NormalizeDroppedPath("/a/b.fbx\r\n/a/c.fbx\r\n", cwd) == "/a/b.fbx");
    CHECK(NormalizeDroppedPath("file:///a/b%20c.fbx", cwd) == "/a/b c.fbx");
    CHECK(NormalizeDroppedPath("file://localhost/a/b.fbx", cwd) == "/a/b.fbx");
    CHECK(NormalizeDroppedPath("FILE:///a/%E2%82%AC.fbx\r\n", cwd) == "/a/\xE2\x82\xAC.fbx");
    CHECK(NormalizeDroppedPath("/a/50%25%zz.fbx", cwd) == "/a/50%25%zz.fbx");
    CHECK(NormalizeDroppedPath("file:///a/50%25%zz", cwd) == "/a/50%%zz");
    CHECK(NormalizeDroppedPath("", cwd).empty());
    CHECK(NormalizeDroppedPath(" \r\n", cwd).empty());

    const char* home = std::getenv("HOME");
    if (home && *home)
        CHECK(NormalizeDroppedPath("~/b.fbx", cwd) == AddTrailingSlash(ea::string(home)) + "b.fbx");
}
