// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
class ModuleComponent;

/// Helper base class for user-defined container that tracks module components.
class URHO3D_API ContainerComponent : public Component
{
    URHO3D_OBJECT(ContainerComponent, Component);

    /// Construct.
    explicit ContainerComponent(Context* context);
    /// Destruct.
    ~ContainerComponent() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Get any module registered as type.
    ModuleComponent* GetAnyModule(StringHash type) const;
    /// Get first module registered as type.
    template <class T> T* GetAnyModule() const;

    /// Get first module registered as type and write and error if more than one module registered.
    ModuleComponent* GetSingleModule(StringHash type) const;
    /// Get first module registered as type and write and error if more than one module registered.
    template <class T> T* GetSingleModule() const;

    /// Get number of modules registered as type.
    unsigned GetNumModules(StringHash type) const;
    /// Get number of modules registered as type.
    template <class T> unsigned GetNumModules() const;

    /// Get n-th module registered as type.
    ModuleComponent* GetModuleAtIndex(StringHash type, unsigned index) const;
    /// Get n-th module registered as type.
    template <class T> T* GetModuleAtIndex(unsigned index) const;

    /// Return all modules registered by type.
    void GetModulesComponents(ea::vector<ModuleComponent*>& dest, StringHash type) const;
    /// Return all modules registered by type.
    template <class T> void GetModulesComponents(ea::vector<T*>& dest) const;
    

protected:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;

    /// Register module in container. Executed by ModuleComponent.
    bool RegisterModule(StringHash type, ModuleComponent* module);
    /// Remove module from container. Executed by ModuleComponent.
    bool RemoveModule(StringHash type, ModuleComponent* module);

private:
    void RemoveAllModules();
    void RegisterAllModules(Node* node);

    /// Is container initialized.
    bool containerInitialized_;

    ea::unordered_multimap<StringHash, WeakPtr<ModuleComponent>> moduleByType_;

    friend class ModuleComponent;
};

template <class T> T* ContainerComponent::GetAnyModule() const
{
    return GetAnyModule(T::GetTypeStatic());
}

template <class T> T* ContainerComponent::GetSingleModule() const
{
    return GetSingleModule(T::GetTypeStatic());
}

template <class T> unsigned ContainerComponent::GetNumModules() const
{
    return GetNumModules(T::GetTypeStatic());
}

template <class T> T* ContainerComponent::GetModuleAtIndex(unsigned index) const
{
    return GetModuleAtIndex(T::GetTypeStatic(), index);
}

template <class T> void ContainerComponent::GetModulesComponents(ea::vector<T*>& dest) const
{
    dest.clear();
    const auto range = moduleByType_.equal_range(T::GetTypeStatic());
    for (auto iterator = range.first; iterator != range.second; ++iterator)
    {
        if (!iterator->second.Expired())
        {
            if (T* module = dynamic_cast<T*>(iterator->second))
                dest.push_back(module);
        }
    }
}

} // namespace Urho3D
