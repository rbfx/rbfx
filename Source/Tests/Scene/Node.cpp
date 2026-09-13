// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../SceneUtils.h"
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/AnimatedModel.h>

TEST_CASE("Load node from XML node file")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto child = scene->CreateChild("Child");

    XMLFile file(context);
    auto nodeElement = file.GetOrCreateRoot("node");
    auto constraint2Attr = nodeElement.CreateChild("attribute");
    constraint2Attr.SetAttribute("name", "Name");
    constraint2Attr.SetAttribute("value", "NodeName");
    auto componentElement = nodeElement.CreateChild("component");
    componentElement.SetAttribute("type", "StaticModel");

    child->LoadXML(nodeElement);

    CHECK(child->GetName() == "NodeName");
    CHECK(child->GetComponent<StaticModel>());
};

TEST_CASE("Test FindComponent")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto root = scene->CreateChild("Root");
    auto parent = root->CreateChild("Parent");
    auto node = parent->CreateChild("Node");
    auto child = node->CreateChild("Child");
    auto grandChild = child->CreateChild("GrandChild");

    auto rootComponent = root->CreateComponent<AnimatedModel>();
    auto parentComponent = parent->CreateComponent<StaticModel>();
    auto nodeComponent = node->CreateComponent<StaticModel>();
    auto childComponent = child->CreateComponent<StaticModel>();
    auto grandChildComponent = grandChild->CreateComponent<AnimatedModel>();

    CHECK(node->FindComponent<StaticModel>(ComponentSearchFlag::Self) == nodeComponent);
    CHECK(node->FindComponent<Drawable>(ComponentSearchFlag::SelfDerived) == nodeComponent);
    CHECK(node->FindComponent<StaticModel>(ComponentSearchFlag::Parent) == parentComponent);
    CHECK(node->FindComponent<Drawable>(ComponentSearchFlag::ParentRecursive | ComponentSearchFlag::Derived) == parentComponent);
    CHECK(node->FindComponent<AnimatedModel>(ComponentSearchFlag::ParentRecursive) == rootComponent);
    CHECK(node->FindComponent<StaticModel>(ComponentSearchFlag::Children) == childComponent);
    CHECK(node->FindComponent<Drawable>(ComponentSearchFlag::Children | ComponentSearchFlag::Derived) == childComponent);
    CHECK(node->FindComponent<AnimatedModel>(ComponentSearchFlag::ChildrenRecursive) == grandChildComponent);
};

TEST_CASE("Test FindComponents")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto root = scene->CreateChild("Root");
    auto parent = root->CreateChild("Parent");
    auto node = parent->CreateChild("Node");
    auto child = node->CreateChild("Child");
    auto grandChild = child->CreateChild("GrandChild");

    auto rootComponent = root->CreateComponent<AnimatedModel>();
    auto parentComponent = parent->CreateComponent<StaticModel>();
    auto nodeComponent = node->CreateComponent<StaticModel>();
    auto childComponent = child->CreateComponent<StaticModel>();
    auto grandChildComponent = grandChild->CreateComponent<AnimatedModel>();

    ea::vector<Component*> dest;
    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::Self);
    CHECK(dest == ea::vector<Component*>{nodeComponent});

    node->FindComponents(dest, Drawable::TypeId, ComponentSearchFlag::SelfDerived);
    CHECK(dest == ea::vector<Component*>{nodeComponent});

    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::Parent);
    CHECK(dest == ea::vector<Component*>{parentComponent});

    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::ParentRecursive);
    CHECK(dest == ea::vector<Component*>{parentComponent});

    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::ParentRecursive | ComponentSearchFlag::Derived);
    CHECK(dest == ea::vector<Component*>{parentComponent, rootComponent});

    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::Self | ComponentSearchFlag::ParentRecursive);
    CHECK(dest == ea::vector<Component*>{nodeComponent, parentComponent});

    node->FindComponents(dest, Drawable::TypeId, ComponentSearchFlag::Self | ComponentSearchFlag::ParentRecursive | ComponentSearchFlag::Derived);
    CHECK(dest == ea::vector<Component*>{nodeComponent, parentComponent, rootComponent});

    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::Children);
    CHECK(dest == ea::vector<Component*>{childComponent});

    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::ChildrenRecursive);
    CHECK(dest == ea::vector<Component*>{childComponent});

    node->FindComponents(dest, StaticModel::TypeId, ComponentSearchFlag::ChildrenRecursive | ComponentSearchFlag::Derived);
    CHECK(dest == ea::vector<Component*>{childComponent, grandChildComponent});

    node->FindComponents(dest, StaticModel::TypeId,
        ComponentSearchFlag::SelfOrChildrenRecursiveDerived);
    CHECK(dest == ea::vector<Component*>{nodeComponent, childComponent, grandChildComponent});
};


TEST_CASE("Test FindComponents<T>")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto root = scene->CreateChild("Root");
    auto parent = root->CreateChild("Parent");
    auto node = parent->CreateChild("Node");
    auto child = node->CreateChild("Child");
    auto grandChild = child->CreateChild("GrandChild");

    auto rootComponent = root->CreateComponent<AnimatedModel>();
    auto parentComponent = parent->CreateComponent<StaticModel>();
    auto nodeComponent = node->CreateComponent<StaticModel>();
    auto childComponent = child->CreateComponent<StaticModel>();
    auto grandChildComponent = grandChild->CreateComponent<AnimatedModel>();

    ea::vector<Component*> dest;
    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::Self);
    CHECK(dest == ea::vector<Component*>{nodeComponent});

    node->FindComponents<Drawable>(dest, ComponentSearchFlag::SelfDerived);
    CHECK(dest == ea::vector<Component*>{nodeComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::Parent);
    CHECK(dest == ea::vector<Component*>{parentComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::ParentRecursive);
    CHECK(dest == ea::vector<Component*>{parentComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::ParentRecursive | ComponentSearchFlag::Derived);
    CHECK(dest == ea::vector<Component*>{parentComponent, rootComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::Self | ComponentSearchFlag::ParentRecursive);
    CHECK(dest == ea::vector<Component*>{nodeComponent, parentComponent});

    node->FindComponents<Drawable>(dest, ComponentSearchFlag::Self | ComponentSearchFlag::ParentRecursive | ComponentSearchFlag::Derived);
    CHECK(dest == ea::vector<Component*>{nodeComponent, parentComponent, rootComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::Children);
    CHECK(dest == ea::vector<Component*>{childComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::ChildrenRecursive);
    CHECK(dest == ea::vector<Component*>{childComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::ChildrenRecursive | ComponentSearchFlag::Derived);
    CHECK(dest == ea::vector<Component*>{childComponent, grandChildComponent});

    node->FindComponents<StaticModel>(dest, ComponentSearchFlag::Self | ComponentSearchFlag::ChildrenRecursive | ComponentSearchFlag::Derived);
    CHECK(dest == ea::vector<Component*>{nodeComponent, childComponent, grandChildComponent});
};

TEST_CASE("Test FindComponents<WeakPtr<T>>")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto node = scene->CreateChild("Root");

    auto nodeComponent = node->CreateComponent<StaticModel>();

    {
        ea::vector<WeakPtr<Component>> dest;
        node->FindComponents<StaticModel>(dest, ComponentSearchFlag::Self);
        CHECK(dest == ea::vector<WeakPtr<Component>>{WeakPtr<Component>(nodeComponent)});
    }
    {
        ea::vector<WeakPtr<StaticModel>> dest;
        node->FindComponents<StaticModel>(dest, ComponentSearchFlag::Self);
        CHECK(dest == ea::vector<WeakPtr<StaticModel>>{WeakPtr<StaticModel>(nodeComponent)});
    }
};
