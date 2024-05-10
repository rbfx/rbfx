// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/IO/Log.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/ContainerComponent.h>
#include <Urho3D/Scene/ContainerComponentEvents.h>
#include <Urho3D/Scene/ModuleComponent.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

ContainerComponent::ContainerComponent(Context* context)
    : BaseClassName(context)
{
}

ContainerComponent::~ContainerComponent()
{
}

void ContainerComponent::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ContainerComponent>();
}

ModuleComponent* ContainerComponent::GetAnyModule(StringHash type) const
{
    const auto range = moduleByType_.equal_range(type);
    if (range.first == range.second)
    {
        return nullptr;
    }
    return range.first->second;
}

ModuleComponent* ContainerComponent::GetSingleModule(StringHash type) const
{
    const auto range = moduleByType_.equal_range(type);
    if (range.first == range.second)
    {
        return nullptr;
    }
    auto first = range.first;
    if (++first != range.second)
    {
        URHO3D_LOGERROR("More than one module registered");
    }
    return range.first->second;
}

unsigned ContainerComponent::GetNumModules(StringHash type) const
{
    const auto range = moduleByType_.equal_range(type);
    return static_cast<unsigned>(ea::distance(range.first, range.second));
}

void ContainerComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (previousNode)
    {
        while (moduleByType_.begin() != moduleByType_.end())
        {
            const auto module = moduleByType_.begin()->second;
            module->SetContainer(nullptr);
        }
    }
    if (currentNode)
    {
        ea::vector<Component*> modules;
        node_->GetDerivedComponents(modules, ModuleComponent::GetTypeStatic(), true);
        for (const auto component : modules)
        {
            if (component->IsEnabledEffective())
            {
                auto* module = dynamic_cast<ModuleComponent*>(component);
                module->SetContainer(this);
            }
        }
    }
}

bool ContainerComponent::RegisterModule(StringHash type, ModuleComponent* module)
{
    const auto range = moduleByType_.equal_range(type);
    for (auto iterator = range.first; iterator != range.second; ++iterator)
    {
        if (iterator->second == module)
        {
            return false;
        }
    }
    moduleByType_.insert({type, WeakPtr(module)});

    {
        using namespace ModuleRegistered;
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[P_CONTAINER] = this;
        eventData[P_MODULE] = module;
        eventData[P_TYPE] = type;
        SendEvent(E_MODULEREGISTERED, eventData);
    }

    return true;
}

bool ContainerComponent::RemoveModule(StringHash type, ModuleComponent* module)
{
    const auto range = moduleByType_.equal_range(type);
    for (auto iterator = range.first; iterator != range.second; ++iterator)
    {
        if (iterator->second == module)
        {
            iterator = moduleByType_.erase(iterator);

            {
                using namespace ModuleRemoved;
                VariantMap& eventData = context_->GetEventDataMap();
                eventData[P_CONTAINER] = this;
                eventData[P_MODULE] = module;
                eventData[P_TYPE] = type;
                SendEvent(E_MODULEREMOVED, eventData);
            }

            return true;
        }
    }
    return false;
}

} // namespace Urho3D
