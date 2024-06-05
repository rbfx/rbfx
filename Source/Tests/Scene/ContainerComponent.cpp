// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Scene/ContainerComponent.h>
#include <Urho3D/Scene/ModuleComponent.h>

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
