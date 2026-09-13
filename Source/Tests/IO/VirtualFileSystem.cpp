// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/IO/VirtualFileSystem.h>

TEST_CASE("FileIdentifier tests")
{
    REQUIRE(!FileIdentifier::Empty);
}

TEST_CASE("VirtualFileSystem has mount points")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto vfs = context->GetSubsystem<VirtualFileSystem>();

    auto numMountPoints = vfs->NumMountPoints();
    CHECK(numMountPoints > 0);
    for (unsigned i = 0; i < numMountPoints; ++i)
    {
        auto mountPoint = vfs->GetMountPoint(i);
        CHECK(mountPoint);
    }
}

TEST_CASE("VirtualFileSystem can read and write text")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto vfs = context->GetSubsystem<VirtualFileSystem>();

    FileIdentifier fileId{"conf", "test_file.txt"};
    ea::string testString{"BlaBla\xE2\x82\xAC"};
    REQUIRE(vfs->WriteAllText(fileId, testString));

    auto restoredText = vfs->ReadAllText(fileId);
    REQUIRE(testString == restoredText);
}
