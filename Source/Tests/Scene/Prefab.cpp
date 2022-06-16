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
#include "../SceneUtils.h"
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include <Urho3D/Scene/PrefabReference.h>

TEST_CASE("Prefab reference")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto cache = context->GetSubsystem<ResourceCache>();
    auto scene = MakeShared<Scene>(context);

    auto node0 = scene->CreateChild();
    auto node1 = scene->CreateChild();

    auto xmlFile = new XMLFile(context);
    xmlFile->SetName("Objects/Obj0.xml");
    auto fileText = MemoryBuffer("<node><component type=\"StaticModel\"/></node>");
    xmlFile->Load(fileText);
    cache->AddManualResource(xmlFile);

    SharedPtr<PrefabReference> prefabRef{node0->CreateComponent<PrefabReference>(CreateMode::REPLICATED)};
    prefabRef->SetPrefab(xmlFile);

    // Setting prefab to enabled node makes component to create temporary node attached to the component's node.
    auto* prefabRoot = prefabRef->GetRootNode();
    REQUIRE(prefabRoot != nullptr);
    CHECK(prefabRoot->IsTemporary());
    CHECK(prefabRoot->GetParent() == node0);
    CHECK(prefabRoot->GetNumChildren() == 0);

    // Component should preserve the node but detach it from the parent
    prefabRef->Remove();
    REQUIRE(prefabRef->GetRootNode() == prefabRoot);
    CHECK(prefabRoot->GetParent() == nullptr);

    // Moving component to another node makes prefab root attached
    node1->AddComponent(prefabRef, prefabRef->GetID(), CreateMode::REPLICATED);
    CHECK(prefabRoot->GetParent() == node1);

    // Reload the prefab on file change
    auto fileText2 = MemoryBuffer("<node><component type=\"StaticModel\"/><node/></node>");
    xmlFile->Load(fileText2);
    {
        using namespace ReloadFinished;
        VariantMap data;
        xmlFile->SendEvent(E_RELOADFINISHED, data);
    }
    CHECK(prefabRef->GetRootNode() != prefabRoot);
    prefabRoot = prefabRef->GetRootNode();
    CHECK(prefabRoot->GetNumChildren() == 1);
}

