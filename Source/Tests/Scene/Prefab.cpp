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
#include "Urho3D/Graphics/StaticModel.h"

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/RigidBody.h>

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
    auto nodeElement = xmlFile->GetOrCreateRoot("node");
    auto componentElement = nodeElement.CreateChild("component");
    componentElement.SetAttribute("type", "StaticModel");

    cache->AddManualResource(xmlFile);

    SharedPtr<PrefabReference> prefabRef{node0->CreateComponent<PrefabReference>(CreateMode::REPLICATED)};
    prefabRef->SetPrefab(xmlFile);

    // Setting prefab to enabled node makes component to create temporary node attached to the component's node.
    // Make it shared ptr to ensure that new node won't be allocated at the same address.
    SharedPtr<Node> prefabRoot{prefabRef->GetRootNode()};
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
    nodeElement.CreateChild("node");
    {
        using namespace ReloadFinished;
        VariantMap data;
        xmlFile->SendEvent(E_RELOADFINISHED, data);
    }
    CHECK(prefabRef->GetRootNode() != prefabRoot);
    prefabRoot = prefabRef->GetRootNode();
    CHECK(prefabRoot->GetNumChildren() == 1);

    prefabRef->Inline();
    CHECK(prefabRef->GetNode() == nullptr);
    CHECK(!prefabRoot->IsTemporary());
}

TEST_CASE("Prefab with node reference")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto cache = context->GetSubsystem<ResourceCache>();
    auto scene = MakeShared<Scene>(context);

    auto node0 = scene->CreateChild();
    auto node1 = scene->CreateChild();

    auto xmlFile = new XMLFile(context);
    xmlFile->SetName("Objects/Obj1.xml");

    auto nodeElement1 = xmlFile->GetOrCreateRoot("node");
    nodeElement1.SetAttribute("id", "1");
    auto nodeElement2 = nodeElement1.CreateChild("node");
    nodeElement2.SetAttribute("id", "2");
    auto rigidBody2Element = nodeElement2.CreateChild("component");
    rigidBody2Element.SetAttribute("type", "RigidBody");
    auto constraint2Element = nodeElement2.CreateChild("component");
    constraint2Element.SetAttribute("type", "Constraint");
    auto constraint2Attr = constraint2Element.CreateChild("attribute");
    constraint2Attr.SetAttribute("name", "Other Body NodeID");
    constraint2Attr.SetAttribute("value", "3");
    auto nodeElement3 = nodeElement1.CreateChild("node");
    nodeElement3.SetAttribute("id", "3");
    auto rigidBody3Element = nodeElement3.CreateChild("component");
    rigidBody3Element.SetAttribute("type", "RigidBody");
    auto staticModel3Element = nodeElement3.CreateChild("component");
    staticModel3Element.SetAttribute("type", "StaticModel");
    
    cache->AddManualResource(xmlFile);

    SharedPtr<PrefabReference> prefabRef{node0->CreateComponent<PrefabReference>(CreateMode::REPLICATED)};
    {
        prefabRef->SetPrefab(xmlFile);

        auto* prefabRoot = prefabRef->GetRootNode();
        REQUIRE(prefabRoot);
        auto* constraint = prefabRoot->GetComponent<Constraint>(true);
        REQUIRE(constraint);
        auto* constraintNode = constraint->GetNode();
        REQUIRE(constraintNode);
        auto* staticModel = prefabRoot->GetComponent<StaticModel>(true);
        REQUIRE(staticModel);
        auto* otherNode = staticModel->GetNode();
        REQUIRE(otherNode);
        REQUIRE(constraint->GetOtherBody() == otherNode->GetComponent<RigidBody>());
    }
    SharedPtr<PrefabReference> prefabRef2{node1->CreateComponent<PrefabReference>(CreateMode::REPLICATED)};
    {
        prefabRef2->SetPrefab(xmlFile);

        auto* prefabRoot = prefabRef2->GetRootNode();
        REQUIRE(prefabRoot);
        auto* constraint = prefabRoot->GetComponent<Constraint>(true);
        REQUIRE(constraint);
        auto* constraintNode = constraint->GetNode();
        REQUIRE(constraintNode);
        auto* staticModel = prefabRoot->GetComponent<StaticModel>(true);
        REQUIRE(staticModel);
        auto* otherNode = staticModel->GetNode();
        REQUIRE(otherNode);
        REQUIRE(constraint->GetOtherBody() == otherNode->GetComponent<RigidBody>());
    }
}

TEST_CASE("Load prefab from scene file")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto child = scene->CreateChild("Child");
    auto prefab = child->CreateComponent<PrefabReference>();

    auto file = MakeShared<XMLFile>(context);
    auto sceneElement = file->GetOrCreateRoot("scene");
    auto nodeElement = sceneElement.CreateChild("node");
    auto constraint2Attr = nodeElement.CreateChild("attribute");
    constraint2Attr.SetAttribute("name", "Name");
    constraint2Attr.SetAttribute("value", "NodeName");
    auto componentElement = nodeElement.CreateChild("component");
    componentElement.SetAttribute("type", "StaticModel");

    prefab->SetPrefab(file);
    auto root = prefab->GetRootNode();

    REQUIRE(root);
    CHECK(root->GetName() == "NodeName");
    CHECK(root->GetComponent<StaticModel>());
};
