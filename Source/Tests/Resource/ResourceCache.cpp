//
// Copyright (c) 2023-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"

#include <Urho3D/IO/MountedExternalMemory.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/UI/SplashScreen.h>

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

TEST_CASE("ResourceCache loads resources referenced from prefabs in scene")
{
    const auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    const auto resourceCache = context->GetSubsystem<ResourceCache>();

    auto splash = MakeShared<SplashScreen>(context);
    splash->QueueSceneResourcesAsync("Scenes/PrefabTestScene.scene");

    do
    {
        Tests::RunFrame(context, 0.1f);
    } while (resourceCache->GetNumBackgroundLoadResources() > 0);
}

} // namespace Tests
