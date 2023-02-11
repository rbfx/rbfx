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

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/VirtualFileSystem.h>

#include "../IO/InMemoryMountPoint.h"

namespace Tests
{

TEST_CASE("ResourceCache material tests")
{
    const auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    const auto vfs = context->GetSubsystem<VirtualFileSystem>();
    vfs->SetWatching(true);
    const auto resourceCache = context->GetSubsystem<ResourceCache>();
    const InMemoryMountPointPtr mountPoint(context);

    const ea::string resName = "ResourceCache/XmlFile.xml";
    mountPoint->SetFile(resName, "<material/>");

    auto xmlFile = resourceCache->GetResource<XMLFile>(resName);
    REQUIRE(xmlFile);
    CHECK(xmlFile->GetRoot().GetName() == "material");

    mountPoint->SetFile(resName, "<something_else/>");

    xmlFile = resourceCache->GetResource<XMLFile>(resName);
    REQUIRE(xmlFile);
    CHECK(xmlFile->GetRoot().GetName() == "something_else");
}

} // namespace Tests
