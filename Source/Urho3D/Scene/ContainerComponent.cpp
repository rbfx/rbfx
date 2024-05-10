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
    RemoveAllModules();
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

ModuleComponent* ContainerComponent::GetModuleAtIndex(StringHash type, unsigned index) const
{
    const auto range = moduleByType_.equal_range(type);
    auto iterator = range.first;
    ea::advance(iterator, index);
    return iterator->second;
}

void ContainerComponent::GetModulesComponents(ea::vector<ModuleComponent*>& dest, StringHash type) const
{
    dest.clear();
    const auto range = moduleByType_.equal_range(type);
    for (auto iterator = range.first; iterator != range.second; ++iterator)
    {
        if (!iterator->second.Expired())
            dest.push_back(iterator->second);
    }
}

void ContainerComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (previousNode)
    {
        RemoveAllModules();
    }
    if (currentNode)
    {
        RegisterAllModules(currentNode);
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

void ContainerComponent::RemoveAllModules()
{
    while (moduleByType_.begin() != moduleByType_.end())
    {
        const auto module = moduleByType_.begin()->second;
        if (!module.Expired())
            module->SetContainer(nullptr);
    }
}

void ContainerComponent::RegisterAllModules(Node* node)
{
    // Check if node has it's own container. If it does - all modules in this part of tree belongs to that component.
    for (const auto& component : node->GetComponents())
    {
        if (const auto derivedComponent = dynamic_cast<ContainerComponent*>(component.Get()))
        {
            if (derivedComponent != this)
            {
                return;
            }
        }
    }

    // Register all found modules.
    for (const auto& component : node->GetComponents())
    {
        if (const auto derivedComponent = dynamic_cast<ModuleComponent*>(component.Get()))
        {
            if (derivedComponent->IsEnabledEffective())
            {
                derivedComponent->SetContainer(this);
            }
        }
    }

    // Register all modules in child nodes too.
    for (const auto& child : node->GetChildren())
    {
        RegisterAllModules(child);
    }
}

} // namespace Urho3D
