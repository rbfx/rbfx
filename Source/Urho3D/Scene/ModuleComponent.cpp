// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/ContainerComponent.h>
#include <Urho3D/Scene/ContainerComponentEvents.h>
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
    RemoveModule();
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
        RemoveModule();
    }
}

void ModuleComponent::SetSubscribeToContainerEnabled(bool enable)
{
    if (subscribeToContainer_ == enable)
        return;

    subscribeToContainer_ = enable;

    UpdateContainerSubscription();
}

void ModuleComponent::OnModuleRegistered(StringHash type, ModuleComponent* module)
{
}

void ModuleComponent::OnModuleRemoved(StringHash type, ModuleComponent* module)
{
}

void ModuleComponent::OnContainerSet(ContainerComponent* container)
{
}

void ModuleComponent::UpdateRegistrations()
{

    if (IsEnabledEffective() && !container_.Expired())
    {
        RegisterModule();
    }
    else
    {
        RemoveModule();
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
        RemoveModule();

        container_ = newContainer;

        UpdateRegistrations();
    }
}

void ModuleComponent::SetContainer(ContainerComponent* container)
{
    if (container_ == container)
        return;

    RemoveModule();

    if (container_ && isSubscribed_)
    {
        isSubscribed_ = false;
        UnsubscribeFromEvent(container_, E_MODULEREGISTERED);
        UnsubscribeFromEvent(container_, E_MODULEREMOVED);
    }

    container_ = container;

    RegisterModule();

    UpdateContainerSubscription();

    OnContainerSet(container);
}

void ModuleComponent::RegisterModule()
{
    if (isRegistered_ || container_.Expired())
        return;

    isRegistered_ = true;

    for (const auto& registeredType : registeredTypes_)
    {
        container_->RegisterModule(registeredType, this);
    }
}

void ModuleComponent::RemoveModule()
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

void ModuleComponent::UpdateContainerSubscription()
{
    if (container_.Expired())
        return;

    if (subscribeToContainer_ != isSubscribed_)
    {
        if (subscribeToContainer_)
        {
            isSubscribed_ = true;
            SubscribeToEvent(container_, E_MODULEREGISTERED, &ModuleComponent::HandleModuleRegistered);
            SubscribeToEvent(container_, E_MODULEREMOVED, &ModuleComponent::HandleModuleRemoved);
        }
        else
        {
            isSubscribed_ = false;
            UnsubscribeFromEvent(container_, E_MODULEREGISTERED);
            UnsubscribeFromEvent(container_, E_MODULEREMOVED);
        }
    }
}

void ModuleComponent::HandleModuleRegistered(StringHash eventType, VariantMap& eventData)
{
    using namespace  ModuleRegistered;

    const auto module = dynamic_cast<ModuleComponent*>(eventData[P_MODULE].GetPtr());
    const auto type = eventData[P_TYPE].GetStringHash();

    if (module != this)
        OnModuleRegistered(type, module);
}

void ModuleComponent::HandleModuleRemoved(StringHash eventType, VariantMap& eventData)
{
    using namespace ModuleRemoved;

    const auto module = dynamic_cast<ModuleComponent*>(eventData[P_MODULE].GetPtr());
    const auto type = eventData[P_TYPE].GetStringHash();

    if (module != this)
        OnModuleRegistered(type, module);
}

}
