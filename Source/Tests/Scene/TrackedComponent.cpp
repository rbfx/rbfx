//
// Copyright (c) 2017-2021 the rbfx project.
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

#include <Urho3D/Scene/TrackedComponent.h>

class TestComponentRegistry : public BaseComponentRegistry
{
    URHO3D_OBJECT(TestComponentRegistry, Component);

public:
    explicit TestComponentRegistry(Context* context);
};

class TestTrackedComponent : public TrackedComponent<TestComponentRegistry, EnabledOnlyTag>
{
    URHO3D_OBJECT(TestTrackedComponent, Component);

public:
    using TrackedComponent<TestComponentRegistry, EnabledOnlyTag>::TrackedComponent;
};

TestComponentRegistry::TestComponentRegistry(Context* context)
    : BaseComponentRegistry(context, TestTrackedComponent::GetTypeStatic())
{
}

TEST_CASE("Tracked components are indexed in the registry")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    if (!context->IsReflected<TestComponentRegistry>())
        context->RegisterFactory<TestComponentRegistry>();
    if (!context->IsReflected<TestTrackedComponent>())
        context->RegisterFactory<TestTrackedComponent>();

    auto scene = MakeShared<Scene>(context);
    auto registry = scene->CreateComponent<TestComponentRegistry>();

    auto node1 = scene->CreateChild("Node 1");
    auto component1 = node1->CreateComponent<TestTrackedComponent>();
    auto node2 = scene->CreateChild("Node 2");
    auto component2 = node2->CreateComponent<TestTrackedComponent>();
    auto node3 = scene->CreateChild("Node 3");
    auto component3 = node3->CreateComponent<TestTrackedComponent>();

    // Create tracked components
    REQUIRE(registry->GetNumTrackedComponents() == 3);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component1);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component2);
    REQUIRE(registry->GetTrackedComponentByIndex(2) == component3);

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component2->GetIndexInArray() == 1);
    REQUIRE(component3->GetIndexInArray() == 2);

    // Remove tracked component
    SharedPtr<Node> shaderNode2{node2};
    node2->Remove();

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component1);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component3);

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component3->GetIndexInArray() == 1);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);

    // Disable tracked component
    node1->SetEnabled(false);
    REQUIRE(registry->GetNumTrackedComponents() == 1);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component3);

    REQUIRE(component3->GetIndexInArray() == 0);

    REQUIRE(component1->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);

    // Enable tracked component
    node1->SetEnabled(true);
    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component3);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component1);

    REQUIRE(component3->GetIndexInArray() == 0);
    REQUIRE(component1->GetIndexInArray() == 1);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);

    // Create disabled tracked component
    auto node4 = scene->CreateChild("Node 4");
    auto component4 = MakeShared<TestTrackedComponent>(context);
    component4->SetEnabled(false);
    node4->AddComponent(component4, 0, LOCAL);

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component3);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component1);

    REQUIRE(component3->GetIndexInArray() == 0);
    REQUIRE(component1->GetIndexInArray() == 1);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component4->GetIndexInArray() == M_MAX_UNSIGNED);

    // Remove registry
    registry->Remove();

    REQUIRE(component3->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component1->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component4->GetIndexInArray() == M_MAX_UNSIGNED);

    // Add registry
    registry = scene->CreateComponent<TestComponentRegistry>();

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component3->GetIndexInArray() == 1);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component4->GetIndexInArray() == M_MAX_UNSIGNED);

    // Remove disabled tracked component
    node4->Remove();

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component1);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component3);

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component3->GetIndexInArray() == 1);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);
}
