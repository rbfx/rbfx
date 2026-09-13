// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/IO/MountedExternalMemory.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace Tests
{

TEST_CASE("ResourceCache loads resources from memory")
{
    const auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    const auto vfs = context->GetSubsystem<VirtualFileSystem>();
    vfs->SetWatching(true);
    const auto resourceCache = context->GetSubsystem<ResourceCache>();
    auto mountPoint = MakeShared<MountedExternalMemory>(context, "memory");
    const MountPointGuard mountPointGuard(mountPoint);

    mountPoint->LinkMemory("path/to/file.xml", "<material/>");

    auto xmlFile = resourceCache->GetResource<XMLFile>("memory://path/to/file.xml");
    REQUIRE(xmlFile);
    CHECK(xmlFile->GetRoot().GetName() == "material");

    mountPoint->LinkMemory("path/to/file.xml", "<something_else/>");
    mountPoint->SendFileChangedEvent("path/to/file.xml");

    xmlFile = resourceCache->GetResource<XMLFile>("memory://path/to/file.xml");
    REQUIRE(xmlFile);
    CHECK(xmlFile->GetRoot().GetName() == "something_else");
}

} // namespace Tests
