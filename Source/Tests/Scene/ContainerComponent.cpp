// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Scene/ContainerComponent.h>
#include <Urho3D/Scene/ModuleComponent.h>

#include <EASTL/queue.h>

namespace
{
class TestModuleObserver : public ModuleObserver
{
public:
    TestModuleObserver(StringHash type)
        : type_(type)
    {
    }
    virtual StringHash GetModuleType() const { return type_; }
    /// Get the attribute. Executed from ModuleComponent::HandleModuleRegistered.
    virtual void Add(ModuleComponent* module) { recordedEvents_.push({EventType::Add, module, container_}); }
    /// Set the attribute. Executed from ModuleComponent::HandleModuleRemoved.
    virtual void Remove(ModuleComponent* module) { recordedEvents_.push({EventType::Remove, module, container_}); }
    /// Set the attribute. Executed from ModuleComponent::SetContainer.
    virtual void SetContainer(const ContainerComponent* container)
    {
        recordedEvents_.push({EventType::SetContainer, nullptr, this->container_ = container});
    }

    enum class EventType
    {
        Add,
        Remove,
        SetContainer
    };

    struct Event
    {
        EventType type_;
        ModuleComponent* module_;
        const ContainerComponent* container_;
    };

    ea::queue<Event> recordedEvents_;

private:
    StringHash type_;
    const ContainerComponent* container_{};
};

class ModuleAObservesB: public ModuleComponent
{
    URHO3D_OBJECT(ModuleAObservesB, ModuleComponent);

    explicit ModuleAObservesB(Context* context);
    ~ModuleAObservesB() override = default;

    SharedPtr<TestModuleObserver> observer_;
};

class ModuleBObservesA : public ModuleComponent
{
    URHO3D_OBJECT(ModuleBObservesA, ModuleComponent);

    ModuleBObservesA(Context* context);
    ~ModuleBObservesA() override = default;

    SharedPtr<TestModuleObserver> observer_;
};

ModuleAObservesB::ModuleAObservesB(Context* context)
    : BaseClassName(context)
{
    RegisterAs<ModuleAObservesB>();
    observer_ = MakeShared<TestModuleObserver>(ModuleBObservesA::GetTypeStatic());
    ObserveModule(observer_);
}

ModuleBObservesA::ModuleBObservesA(Context* context)
    : BaseClassName(context)
{
    RegisterAs<ModuleBObservesA>();
    observer_ = MakeShared<TestModuleObserver>(ModuleAObservesB::GetTypeStatic());
    ObserveModule(observer_);
}

void RequireEvent(TestModuleObserver* observer, const TestModuleObserver::Event& event)
{
    REQUIRE(!observer->recordedEvents_.empty());
    auto& actualEvent = observer->recordedEvents_.front();
    REQUIRE(event.type_ == actualEvent.type_);
    REQUIRE(event.container_ == actualEvent.container_);
    REQUIRE(event.module_ == actualEvent.module_);
    observer->recordedEvents_.pop();
}

void RequireNoMoreEvents(TestModuleObserver* observer)
{
    REQUIRE(observer->recordedEvents_.empty());
}

}


TEST_CASE("ModuleObserver executed for late join")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    context->RegisterFactory<ModuleAObservesB>();
    context->RegisterFactory<ModuleBObservesA>();

    auto scene = MakeShared<Scene>(context);
    auto child = scene->CreateChild("Child");
    auto container = child->CreateComponent<ContainerComponent>();
    auto moduleA = child->CreateComponent<ModuleAObservesB>();

    RequireEvent(moduleA->observer_, {TestModuleObserver::EventType::SetContainer, nullptr, container});
    RequireNoMoreEvents(moduleA->observer_);

    auto moduleB = child->CreateComponent<ModuleBObservesA>();

    RequireEvent(moduleA->observer_, {TestModuleObserver::EventType::Add, moduleB, container});
    RequireNoMoreEvents(moduleA->observer_);

    RequireEvent(moduleB->observer_, {TestModuleObserver::EventType::SetContainer, nullptr, container});
    //RequireEvent(moduleB->observer_, {TestModuleObserver::EventType::Add, moduleA, container});
    RequireNoMoreEvents(moduleB->observer_);
}

TEST_CASE("ContainerComponent tracks ModuleComponent")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    {
        //Create container first, then module
        auto scene = MakeShared<Scene>(context);
        auto child = scene->CreateChild("Child");
        auto container = child->CreateComponent<ContainerComponent>();
        auto module = child->CreateComponent<ModuleComponent>();

        auto resolvedModule = container->GetSingleModule<ModuleComponent>();
        CHECK(module == resolvedModule);
    }

    {
        // Create module first, then container
        auto scene = MakeShared<Scene>(context);
        auto child = scene->CreateChild("Child");
        auto module = child->CreateComponent<ModuleComponent>();
        auto container = child->CreateComponent<ContainerComponent>();

        auto resolvedModule = container->GetSingleModule<ModuleComponent>();
        CHECK(module == resolvedModule);
    }

    {
        // Add container first, then module
        auto scene = MakeShared<Scene>(context);
        auto containerNode = MakeShared<Node>(context);
        auto container = containerNode->CreateComponent<ContainerComponent>();
        auto moduleNode = MakeShared<Node>(context);
        auto module = moduleNode->CreateComponent<ModuleComponent>();

        scene->AddChild(containerNode);
        containerNode->AddChild(moduleNode);

        auto resolvedModule = container->GetSingleModule<ModuleComponent>();
        CHECK(module == resolvedModule);
    }

    {
        // Add module first, then container
        auto scene = MakeShared<Scene>(context);
        auto containerNode = MakeShared<Node>(context);
        auto container = containerNode->CreateComponent<ContainerComponent>();
        auto moduleNode = MakeShared<Node>(context);
        auto module = moduleNode->CreateComponent<ModuleComponent>();

        containerNode->AddChild(moduleNode);
        scene->AddChild(containerNode);

        auto resolvedModule = container->GetSingleModule<ModuleComponent>();
        CHECK(module == resolvedModule);
    }

     {
        // Add container first, then module component
        auto scene = MakeShared<Scene>(context);
        auto containerNode = MakeShared<Node>(context);
        auto container = containerNode->CreateComponent<ContainerComponent>();
        auto moduleNode = MakeShared<Node>(context);

        scene->AddChild(containerNode);
        containerNode->AddChild(moduleNode);
        auto module = moduleNode->CreateComponent<ModuleComponent>();

        auto resolvedModule = container->GetSingleModule<ModuleComponent>();
        CHECK(module == resolvedModule);
    }

    {
        // Add module first, then container component
        auto scene = MakeShared<Scene>(context);
        auto containerNode = MakeShared<Node>(context);
        auto moduleNode = MakeShared<Node>(context);
        auto module = moduleNode->CreateComponent<ModuleComponent>();

        containerNode->AddChild(moduleNode);
        scene->AddChild(containerNode);
        auto container = containerNode->CreateComponent<ContainerComponent>();

        auto resolvedModule = container->GetSingleModule<ModuleComponent>();
        CHECK(module == resolvedModule);
    }
};

TEST_CASE("ContainerComponent with multiple ModuleComponents")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    // Create container first, then module
    auto scene = MakeShared<Scene>(context);
    auto child = scene->CreateChild("Child");
    auto container = child->CreateComponent<ContainerComponent>();
    CHECK(0 == container->GetNumModules<ModuleComponent>());
    auto module1 = child->CreateComponent<ModuleComponent>();
    CHECK(1 == container->GetNumModules<ModuleComponent>());
    auto module2 = child->CreateComponent<ModuleComponent>();

    {
        const auto resolvedModule = container->GetAnyModule<ModuleComponent>();
        const bool check = (module1 == resolvedModule) || (module2 == resolvedModule);
        CHECK(check);
    }
    {
        CHECK(2 == container->GetNumModules<ModuleComponent>());
        auto resolvedModule1 = container->GetModuleAtIndex<ModuleComponent>(0);
        const bool check1 = (module1 == resolvedModule1) || (module2 == resolvedModule1);
        CHECK(check1);
        auto resolvedModule2 = container->GetModuleAtIndex<ModuleComponent>(1);
        const bool check2 = (module1 == resolvedModule1) || (module2 == resolvedModule1);
        CHECK(check2);
    }
}
