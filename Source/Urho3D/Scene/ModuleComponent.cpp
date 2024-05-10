// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/ContainerComponent.h>
#include <Urho3D/Scene/ModuleComponent.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

ModuleComponent::ModuleComponent(Context* context)
    : BaseClassName(context)
{
    RegisterAs<ModuleComponent>();
}

ModuleComponent::~ModuleComponent()
{
}

void ModuleComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ModuleComponent>();
}

void ModuleComponent::OnSetEnabled()
{
    UpdateRegistrations();
}

ContainerComponent* ModuleComponent::GetContainer() const
{
    return container_.Get();
}

StringHash ModuleComponent::GetModuleType(unsigned index) const
{
    return (registeredTypes_.size() < index) ? registeredTypes_.at(index) : StringHash{};
}

void ModuleComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    AutodetectContainer();
}

void ModuleComponent::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        AutodetectContainer();
    }
    else
    {
        UnregisterModule();
    }
}

void ModuleComponent::UpdateRegistrations()
{

    if (IsEnabledEffective() && !container_.Expired())
    {
        RegisterModule();
    }
    else
    {
        UnregisterModule();
    }
}

void ModuleComponent::RegisterAs(StringHash type)
{
    for (auto& registeredType: registeredTypes_)
    {
        if (registeredType == type)
            return;
    }

    registeredTypes_.push_back(type);

    if (IsEnabledEffective() && !container_.Expired())
    {
        container_->RegisterModule(type, this);
    }
}

void ModuleComponent::AutodetectContainer()
{
    ContainerComponent* newContainer = nullptr;
    if (node_)
    {
        newContainer = node_->GetDerivedComponent<ContainerComponent>();
        if (!newContainer)
            newContainer = node_->GetParentDerivedComponent<ContainerComponent>(true);
    }

    if (container_ != newContainer)
    {
        UnregisterModule();

        container_ = newContainer;

        UpdateRegistrations();
    }
}

void ModuleComponent::SetContainer(ContainerComponent* container)
{
    UnregisterModule();

    container_ = container;

    RegisterModule();
}

void ModuleComponent::RegisterModule()
{
    if (isRegistered_ || !container_)
        return;

    isRegistered_ = true;

    for (const auto& registeredType : registeredTypes_)
    {
        container_->RegisterModule(registeredType, this);
    }
}

void ModuleComponent::UnregisterModule()
{
    if (!isRegistered_)
        return;

    isRegistered_ = false;

    if (!container_.Expired())
    {
        for (const auto& registeredType : registeredTypes_)
        {
            container_->RemoveModule(registeredType, this);
        }
    }
}

}
