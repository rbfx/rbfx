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

class TestComponentRegistry : public ReferencedComponentRegistryBase
{
    URHO3D_OBJECT(TestComponentRegistry, ReferencedComponentRegistryBase);

public:
    static constexpr bool IsOnlyEnabledTracked = true;

    explicit TestComponentRegistry(Context* context);
};

class TestTrackedComponent : public TrackedComponent<ReferencedComponentBase, TestComponentRegistry>
{
    URHO3D_OBJECT(TestTrackedComponent, ReferencedComponentBase);

public:
    using TrackedComponent<ReferencedComponentBase, TestComponentRegistry>::TrackedComponent;
};

TestComponentRegistry::TestComponentRegistry(Context* context)
    : ReferencedComponentRegistryBase(context, TestTrackedComponent::GetTypeStatic())
{
}

TEST_CASE("Tracked components are indexed in the registry")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<TestComponentRegistry, TestTrackedComponent>(context);

    auto scene = MakeShared<Scene>(context);
    auto registry = scene->CreateComponent<TestComponentRegistry>();

    // Create tracked components
    auto node1 = scene->CreateChild("Node 1");
    auto component1 = node1->CreateComponent<TestTrackedComponent>();
    auto node2 = scene->CreateChild("Node 2");
    auto component2 = node2->CreateComponent<TestTrackedComponent>();
    auto node3 = scene->CreateChild("Node 3");
    auto component3 = node3->CreateComponent<TestTrackedComponent>();

    REQUIRE(component1->GetRegistry() == registry);
    REQUIRE(component2->GetRegistry() == registry);
    REQUIRE(component3->GetRegistry() == registry);

    REQUIRE(registry->GetNumTrackedComponents() == 3);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component1);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component2);
    REQUIRE(registry->GetTrackedComponentByIndex(2) == component3);

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component2->GetIndexInArray() == 1);
    REQUIRE(component3->GetIndexInArray() == 2);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000001}) == component1);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000002}) == component2);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000003}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference{0x00000001});
    REQUIRE(component2->GetReference() == ComponentReference{0x00000002});
    REQUIRE(component3->GetReference() == ComponentReference{0x00000003});

    // Remove tracked component
    SharedPtr<Node> shaderNode2{node2};
    node2->Remove();

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component1);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component3);

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component3->GetIndexInArray() == 1);
    REQUIRE(component2->GetIndexInArray() == M_MAX_UNSIGNED);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000001}) == component1);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000003}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference{0x00000001});
    REQUIRE(component2->GetReference() == ComponentReference::None);
    REQUIRE(component3->GetReference() == ComponentReference{0x00000003});

    shaderNode2 = nullptr;
    component2 = nullptr;

    // Disable tracked component
    const auto node1Reference = component1->GetReference();
    node1->SetEnabled(false);

    REQUIRE(registry->GetNumTrackedComponents() == 1);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component3);

    REQUIRE(component3->GetIndexInArray() == 0);

    REQUIRE(component1->GetIndexInArray() == M_MAX_UNSIGNED);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000003}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference::None);
    REQUIRE(component3->GetReference() == ComponentReference{0x00000003});

    // Enable tracked component
    component1->SetReference(node1Reference);
    node1->SetEnabled(true);

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component3);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component1);

    REQUIRE(component3->GetIndexInArray() == 0);
    REQUIRE(component1->GetIndexInArray() == 1);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000001}) == component1);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000003}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference{0x00000001});
    REQUIRE(component3->GetReference() == ComponentReference{0x00000003});

    // Create disabled tracked component
    auto node4 = scene->CreateChild("Node 4");
    auto component4 = MakeShared<TestTrackedComponent>(context);
    component4->SetEnabled(false);
    node4->AddComponent(component4, 0);

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component3);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component1);

    REQUIRE(component3->GetIndexInArray() == 0);
    REQUIRE(component1->GetIndexInArray() == 1);
    REQUIRE(component4->GetIndexInArray() == M_MAX_UNSIGNED);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000001}) == component1);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000003}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference{0x00000001});
    REQUIRE(component3->GetReference() == ComponentReference{0x00000003});

    // Remove registry
    registry->Remove();

    REQUIRE(component3->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component1->GetIndexInArray() == M_MAX_UNSIGNED);
    REQUIRE(component4->GetIndexInArray() == M_MAX_UNSIGNED);

    REQUIRE(component3->GetReference() == ComponentReference::None);
    REQUIRE(component1->GetReference() == ComponentReference::None);
    REQUIRE(component4->GetReference() == ComponentReference::None);

    // Add registry
    registry = scene->CreateComponent<TestComponentRegistry>();

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component3->GetIndexInArray() == 1);
    REQUIRE(component4->GetIndexInArray() == M_MAX_UNSIGNED);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000001}) == component1);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000002}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference{0x00000001});
    REQUIRE(component3->GetReference() == ComponentReference{0x00000002});
    REQUIRE(component4->GetReference() == ComponentReference::None);

    // Remove disabled tracked component
    node4->Remove();

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component1);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component3);

    REQUIRE(component1->GetIndexInArray() == 0);
    REQUIRE(component3->GetIndexInArray() == 1);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000001}) == component1);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000002}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference{0x00000001});
    REQUIRE(component3->GetReference() == ComponentReference{0x00000002});
    REQUIRE(component4->GetReference() == ComponentReference::None);

    // Disable and enable tracked component
    component1->SetEnabled(false);
    component1->SetEnabled(true);

    REQUIRE(component1->GetRegistry() == registry);
    REQUIRE(component3->GetRegistry() == registry);

    REQUIRE(registry->GetNumTrackedComponents() == 2);
    REQUIRE(registry->GetTrackedComponentByIndex(0) == component3);
    REQUIRE(registry->GetTrackedComponentByIndex(1) == component1);

    REQUIRE(component3->GetIndexInArray() == 0);
    REQUIRE(component1->GetIndexInArray() == 1);

    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference::None) == nullptr);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x01000001}) == component1);
    REQUIRE(registry->GetTrackedComponentByReference(ComponentReference{0x00000002}) == component3);

    REQUIRE(component1->GetReference() == ComponentReference{0x01000001});
    REQUIRE(component3->GetReference() == ComponentReference{0x00000002});
    REQUIRE(component4->GetReference() == ComponentReference::None);
}
