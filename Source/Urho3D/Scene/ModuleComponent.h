// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Scene/Component.h>

#include <EASTL/fixed_vector.h>

namespace Urho3D
{
class ContainerComponent;

/// Helper base class for user-defined components tracked by ContainerComponent.
class URHO3D_API ModuleComponent : public Component
{
    URHO3D_OBJECT(ModuleComponent, Component);

    /// Construct.
    explicit ModuleComponent(Context* context);
    /// Destruct.
    ~ModuleComponent() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Handle enabled/disabled state change.
    void OnSetEnabled() override;

    /// Get current container the module is associated with.
    ContainerComponent* GetContainer() const;

    /// Get number of module types.
    unsigned GetNumModuleTypes() const { return static_cast<unsigned>(registeredTypes_.size()); }

    StringHash GetModuleType(unsigned index) const;

protected:
    /// Handle scene node being assigned at creation.
    void OnNodeSet(Node* previousNode, Node* currentNode) override;
    /// Handle scene being assigned.
    void OnSceneSet(Scene* scene) override;

    /// Does module need to track other modules being registered in container.
    bool GetSubscribeToContainerEnabled() const { return subscribeToContainer_; }
    /// Set module to track other modules being registered in container.
    void SetSubscribeToContainerEnabled(bool enable);

    /// Handle module being registered in container. Call SetSubscribeToContainerEnabled to enable subscription.
    virtual void OnModuleRegistered(StringHash type, ModuleComponent* module);
    /// Handle module being removed from container.  Call SetSubscribeToContainerEnabled to enable subscription.
    virtual void OnModuleRemoved(StringHash type, ModuleComponent* module);
    /// Handle module being registered in or removed from container.
    virtual void OnContainerSet(ContainerComponent* container);

    /// Register current module as type in container.
    void RegisterAs(StringHash type);

    /// Find container and update it if necessary.
    void AutodetectContainer();

    /// Set module's container. Executed from container.
    void SetContainer(ContainerComponent* container);

    /// Register module into current container.
    void RegisterModule();

    /// Unregister module from current container.
    void RemoveModule();

    /// Register current module as type in container.
    template <typename T> void RegisterAs();

private:
    void UpdateContainerSubscription();
    void HandleModuleRegistered(StringHash eventType, VariantMap& eventData);
    void HandleModuleRemoved(StringHash eventType, VariantMap& eventData);

    void UpdateRegistrations();

    /// Container reference.
    WeakPtr<ContainerComponent> container_;

    /// List of types registered at container.
    /// Small number of types is expected so there is no reason to use more complex container than simple preallocated vector.
    ea::fixed_vector<StringHash,4> registeredTypes_;

    /// Is registered at container.
    bool isRegistered_{};

    /// Is subscribed to container.
    bool isSubscribed_{};

    /// Does module need to track modules in container.
    bool subscribeToContainer_{};

    friend class ContainerComponent;
};

template <typename T> void ModuleComponent::RegisterAs()
{
    RegisterAs(T::GetTypeStatic());
}

} // namespace Urho3D
