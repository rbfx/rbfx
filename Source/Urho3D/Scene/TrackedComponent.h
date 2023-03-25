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

#pragma once

#include <Urho3D/Container/IndexAllocator.h>
#include <Urho3D/Core/TupleUtils.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Scene.h>

namespace Urho3D
{

/// Base class for simple tracked component.
/// It maintains up-to-date 0-based index in the registry.
/// Index may change during the lifetime of the component!
class URHO3D_API TrackedComponentBase : public Component
{
    URHO3D_OBJECT(TrackedComponentBase, Component);

public:
    explicit TrackedComponentBase(Context* context)
        : Component(context)
    {
    }

    /// Manage index of the component in the registry.
    /// @{
    void SetIndexInArray(unsigned index) { indexInArray_ = index; }
    unsigned GetIndexInArray() const { return indexInArray_; }
    bool IsTrackedInRegistry() const { return indexInArray_ != M_MAX_UNSIGNED; }
    /// @}

    /// Returns whether the component should be tracked by the registry.
    virtual bool ShouldBeTrackedInRegistry() const { return false; }
    /// Reconnect component to registry of the current scene.
    virtual void ReconnectToRegistry() = 0;

private:
    unsigned indexInArray_{M_MAX_UNSIGNED};
};

/// Base class for component registry that keeps components derived from TrackedComponentBase.
class URHO3D_API TrackedComponentRegistryBase : public Component
{
    URHO3D_OBJECT(TrackedComponentRegistryBase, Component);

public:
    static constexpr bool IsOnlyEnabledTracked = false;

    TrackedComponentRegistryBase(Context* context, StringHash componentType);

    TrackedComponentBase* GetTrackedComponentByIndex(unsigned index) const;
    unsigned GetNumTrackedComponents() const { return componentsArray_.size(); }
    ea::span<TrackedComponentBase* const> GetTrackedComponents() const { return componentsArray_; }

    /// Internal. Manage tracked components.
    /// @{
    void AddTrackedComponent(TrackedComponentBase* component);
    void RemoveTrackedComponent(TrackedComponentBase* component);
    /// @}

protected:
    void OnSceneSet(Scene* scene) override;

    virtual void OnComponentAdded(TrackedComponentBase* baseComponent) {}
    virtual void OnComponentMoved(TrackedComponentBase* baseComponent, unsigned oldIndex) {}
    virtual void OnComponentRemoved(TrackedComponentBase* baseComponent) {}
    virtual void OnAddedToScene(Scene* scene) {}
    virtual void OnRemovedFromScene() {}

private:
    void InitializeTrackedComponents();
    void DeinitializeTrackedComponents();

    const StringHash componentType_{};
    ea::vector<TrackedComponentBase*> componentsArray_;
};

/// Strongly typed component ID. Default value is considered invalid.
enum class ComponentReference : unsigned
{
    None
};
URHO3D_API ComponentReference ConstructComponentReference(unsigned index, unsigned version);
URHO3D_API ea::pair<unsigned, unsigned> DeconstructComponentReference(ComponentReference componentId);
URHO3D_API ea::string ToString(ComponentReference value);

/// Base class for tracked component with reference that is stable during object lifetime.
class URHO3D_API ReferencedComponentBase : public TrackedComponentBase
{
    URHO3D_OBJECT(ReferencedComponentBase, TrackedComponentBase);

public:
    explicit ReferencedComponentBase(Context* context)
        : TrackedComponentBase(context)
    {
    }

    /// Manage reference to this component.
    /// @{
    void SetReference(ComponentReference reference) { componentReference_ = reference; }
    ComponentReference GetReference() const { return componentReference_; }
    /// @}

private:
    ComponentReference componentReference_{};
};

/// Base class for component registry that keeps components derived from ReferencedComponentBase.
class URHO3D_API ReferencedComponentRegistryBase : public TrackedComponentRegistryBase
{
    URHO3D_OBJECT(ReferencedComponentRegistryBase, TrackedComponentRegistryBase);

public:
    ReferencedComponentRegistryBase(Context* context, StringHash componentType);

    ReferencedComponentBase* GetTrackedComponentByReference(ComponentReference id, bool checkVersion = true) const;
    ReferencedComponentBase* GetTrackedComponentByReferenceIndex(unsigned index) const;

    unsigned GetReferenceIndexUpperBound() const { return referenceIndexToEntry_.size(); }

protected:
    void OnComponentAdded(TrackedComponentBase* baseComponent) override;
    void OnComponentRemoved(TrackedComponentBase* baseComponent) override;

private:
    struct RegistryEntry
    {
        ReferencedComponentBase* component_{};
        unsigned version_{};
    };

    unsigned AllocateReferenceIndex();
    void EnsureIndex(unsigned index);

    IndexAllocator<> referenceIndexAllocator_;
    ea::vector<RegistryEntry> referenceIndexToEntry_;
};

/// Template base of any TrackedComponent that automatically registers itself in registry.
template <class ComponentType, class RegistryComponentType> class TrackedComponent : public ComponentType
{
public:
    static_assert(ea::is_base_of_v<ReferencedComponentRegistryBase, RegistryComponentType>
            == ea::is_base_of_v<ReferencedComponentBase, ComponentType>,
        "Component inherited from ReferencedComponentBase should use registry inherited from "
        "ReferencedComponentRegistryBase, "
        "Component inherited from TrackedComponentBase should use registry inherited from "
        "TrackedComponentRegistryBase");

    explicit TrackedComponent(Context* context)
        : ComponentType(context)
    {
    }

    RegistryComponentType* GetRegistry() const { return registry_; }

    bool ShouldBeTrackedInRegistry() const override
    {
        if constexpr (RegistryComponentType::IsOnlyEnabledTracked)
            return this->IsEnabledEffective();
        else
            return true;
    }

    void ReconnectToRegistry() override
    {
        Scene* scene = this->GetScene();
        registry_ = scene ? scene->GetDerivedComponent<RegistryComponentType>() : nullptr;
    }

    void OnSetEnabled() override
    {
        if constexpr (RegistryComponentType::IsOnlyEnabledTracked)
        {
            const bool wasEnabled = this->IsTrackedInRegistry();
            const bool isEnabled = this->ShouldBeTrackedInRegistry();
            if (registry_ && wasEnabled != isEnabled)
            {
                if (isEnabled)
                    registry_->AddTrackedComponent(this);
                else
                    registry_->RemoveTrackedComponent(this);
            }
        }
    }

protected:
    void OnSceneSet(Scene* scene) override
    {
        auto newRegistry = scene ? scene->GetDerivedComponent<RegistryComponentType>() : nullptr;
        if (newRegistry == registry_)
            return;

        if (registry_ && this->IsTrackedInRegistry())
            registry_->RemoveTrackedComponent(this);

        registry_ = newRegistry;

        if (registry_ && this->ShouldBeTrackedInRegistry())
            registry_->AddTrackedComponent(this);
    }

private:
    WeakPtr<RegistryComponentType> registry_;
};

} // namespace Urho3D
