// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/Component.h"
#include "Urho3D/IO/Log.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

class ModuleComponent;
class ContainerComponent;

/// Abstract base class for invoking module observers.
class URHO3D_API ModuleObserver : public RefCounted
{
public:
    /// Construct.
    ModuleObserver() = default;
    /// Get observed module type.
    virtual StringHash GetModuleType() const = 0;
    /// Get the attribute. Executed from ModuleComponent::HandleModuleRegistered.
    virtual void Add(ModuleComponent* module) = 0;
    /// Set the attribute. Executed from ModuleComponent::HandleModuleRemoved.
    virtual void Remove(ModuleComponent* module) = 0;
    /// Set the attribute. Executed from ModuleComponent::SetContainer.
    virtual void SetContainer(const ContainerComponent* container) = 0;
};

namespace Detail
{
/// Template implementation of the variant attribute accessor.
template <class TModuleType> class SingleModuleObserverImpl : public ModuleObserver
{
public:
    /// Construct.
    SingleModuleObserverImpl(
        ea::function<void(TModuleType*)> setFunction)
        : setFunction_(setFunction)
    {
    }

    Urho3D::StringHash GetModuleType() const override { return TModuleType::TypeId; }

    /// Invoke add function.
    void Add(ModuleComponent* module) override
    {
        assert(module);
        if (module_)
        {
            URHO3D_LOGERROR("Observer expects single module {} but more than one found", TModuleType::GetTypeNameStatic());
        }
        else
        {
            module_ = static_cast<TModuleType*>(module);
            setFunction_(module_);
        }
    }

    /// Invoke remove function.
    void Remove(ModuleComponent* module) override
    {
        assert(module);
        const auto classPtr = static_cast<TModuleType*>(module);
        if (classPtr == module_)
        {
            module_ = nullptr;
            setFunction_(nullptr);
        }
    }

    void SetContainer(const ContainerComponent* container) override
    {
        if (container)
        {
            unsigned num = container->GetNumModules(TModuleType::TypeId);
            if (num > 1)
                URHO3D_LOGERROR("Observer expects single module {} but found {} modules", TModuleType::GetTypeNameStatic(), num);
            if (num > 0)
            {
                module_ = static_cast<TModuleType*>(container->GetModuleAtIndex(TModuleType::TypeId, 0));
                setFunction_(module_);
            }
        }
        else
        {
            if (module_)
            {
                module_ = nullptr;
                setFunction_(nullptr);
            }
        }
    }

    TModuleType* Get() const { return module_; }

private:
    /// Add functor.
    ea::function<void(TModuleType*)> setFunction_;
    /// Module.
    WeakPtr<TModuleType> module_;
};


/// Template implementation of the variant attribute accessor.
template <class TModuleType> class ModulesObserverImpl : public ModuleObserver
{
public:
    static inline constexpr Urho3D::StringHash ModuleTypeId{TModuleType::TypeId};

    /// Construct.
    ModulesObserverImpl(
        ea::function<void(TModuleType*)> addFunction, ea::function<void(TModuleType*)> removeFunction)
        : addFunction_(addFunction)
        , removeFunction_(removeFunction)
    {
    }

    Urho3D::StringHash GetModuleType() const override { return ModuleTypeId; }

    /// Invoke add function.
    void Add(ModuleComponent* module) override
    {
        assert(module);
        const auto classPtr = static_cast<TModuleType*>(module);
        addFunction_(classPtr);
    }

    /// Invoke remove function.
    void Remove(ModuleComponent* module) override
    {
        assert(module);
        const auto classPtr = static_cast<TModuleType*>(module);
        removeFunction_(classPtr);
    }

    void SetContainer(const ContainerComponent* container) override
    {
        if (container)
        {
            const unsigned num = container->GetNumModules(ModuleTypeId);
            for (unsigned index=0; index<num;++index)
            {
                if (auto module = static_cast<TModuleType*>(container->GetModuleAtIndex(ModuleTypeId, 0)))
                {
                    modules_.emplace_back(module);
                    addFunction_(module);
                }
            }
        }
        else
        {
            for (auto& module: modules_)
                removeFunction_(module);
            modules_.clear();
        }
    }

private:
    /// Add functor.
    ea::function<void(TModuleType*)> addFunction_;
    /// Remove functor.
    ea::function<void(TModuleType*)> removeFunction_;
    /// Module.
    ea::vector<WeakPtr<TModuleType>> modules_;
};
} // namespace Detail

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
    /// Handle changes in IsEnabledEffective. This takes into account SetScene and OnSetEnabled callbacks.
    virtual void OnEffectiveEnabled(bool enabled);

    /// Register current module as type in container.
    void RegisterAs(StringHash type);

    /// Find container and update it if necessary.
    void AutodetectContainer();

    /// Set module's container. Executed from container.
    void SetContainer(ContainerComponent* container);

        /// Register module into current container.
    void RegisterModule();

    /// Remove module from current container.
    void RemoveModule();

    /// Register current module as type in container.
    template <typename T> void RegisterAs();

    /// Add module observer. This automatically enables subscription for container events.
    /// \tparam TModuleType ModuleComponent class type.
    template <class TModuleType>
    void ObserveModule(
        ea::function<void(TModuleType*)> setFunction);

    /// Add module observer. This automatically enables subscription for container events.
    /// \tparam TModuleType ModuleComponent class type.
    template <class TModuleType>
    void ObserveModules(
        ea::function<void(TModuleType*)> addFunction,
        ea::function<void(TModuleType*)> removeFunction);

    /// Add module observer. This automatically enables subscription for container events.
    void ObserveModule(ModuleObserver * observer);

private:
    void UpdateContainerSubscription();
    void HandleModuleRegistered(StringHash eventType, VariantMap& eventData);
    void HandleModuleRemoved(StringHash eventType, VariantMap& eventData);
    void UpdateRegistrations();
    void UpdateEnabledEffective();


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

    /// Last known IsEffectiveEnabled result.
    bool effectiveEnabled_{};

    /// Observed modules.
    ea::vector<SharedPtr<ModuleObserver>> observers_;

    friend class ContainerComponent;
};

template <typename T> void ModuleComponent::RegisterAs()
{
    RegisterAs(T::GetTypeStatic());
}

template <class TModuleType>
void ModuleComponent::ObserveModule(
    ea::function<void(TModuleType*)> setFunction)
{
    auto observer =
        SharedPtr<ModuleObserver>(new Detail::SingleModuleObserverImpl<TModuleType>(setFunction));
    ObserveModule(observer);
}

template <class TModuleType>
void ModuleComponent::ObserveModules(
    ea::function<void(TModuleType*)> addFunction, ea::function<void(TModuleType*)> removeFunction)
{
    auto observer = SharedPtr<ModuleObserver>(new Detail::ModulesObserverImpl<TModuleType>(addFunction, removeFunction));
    ObserveModule(observer);
}

#define URHO3D_OBSERVE_SINGLE_MODULE(typeName, setFunction) \
    ObserveModule<typeName>( \
    [=](typeName* module) { this->setFunction(module); })

#define URHO3D_OBSERVE_MODULES(typeName, addFunction, removeFunction) \
    ObserveModules<typeName>( \
    [=](typeName* module) { this->addFunction(module); }, \
    [=](typeName* module) { this->removeFunction(module); })

} // namespace Urho3D
