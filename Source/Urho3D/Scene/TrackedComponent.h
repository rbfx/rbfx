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

#include "../Core/TupleUtils.h"
#include "../Container/IndexAllocator.h"
#include "../Scene/Component.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

/// Base class for simple tracked component.
/// It maintains up-to-date 0-based index in the registry.
/// Index may change during the lifetime of the component!
class URHO3D_API BaseTrackedComponent : public Component
{
public:
    explicit BaseTrackedComponent(Context* context) : Component(context) {}

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

/// Base class for component registry that keeps components derived from BaseTrackedComponent.
class URHO3D_API BaseComponentRegistry : public Component
{
public:
    BaseComponentRegistry(Context* context, StringHash componentType);

    BaseTrackedComponent* GetTrackedComponentByIndex(unsigned index) const;
    unsigned GetNumTrackedComponents() const { return componentsArray_.size(); }
    ea::span<BaseTrackedComponent* const> GetTrackedComponents() const { return componentsArray_; }

    /// Internal. Manage tracked components.
    /// @{
    void AddTrackedComponent(BaseTrackedComponent* component);
    void RemoveTrackedComponent(BaseTrackedComponent* component);
    /// @}

protected:
    void OnSceneSet(Scene* scene) override;

    virtual void OnComponentAdded(BaseTrackedComponent* baseComponent) {}
    virtual void OnComponentMoved(BaseTrackedComponent* baseComponent, unsigned oldIndex) {}
    virtual void OnComponentRemoved(BaseTrackedComponent* baseComponent) {}

private:
    void InitializeTrackedComponents();
    void DeinitializeTrackedComponents();

    const StringHash componentType_{};
    ea::vector<BaseTrackedComponent*> componentsArray_;
};

/// Strongly typed component ID. Default value is considered invalid.
enum StableComponentId : unsigned {};
URHO3D_API StableComponentId ConstructStableComponentId(unsigned index, unsigned version);
URHO3D_API ea::pair<unsigned, unsigned> DeconstructStableComponentId(StableComponentId componentId);
URHO3D_API ea::string ToString(StableComponentId value);

/// Base class for tracked component with ID that is stable during object lifetime.
class URHO3D_API BaseStableTrackedComponent : public BaseTrackedComponent
{
public:
    explicit BaseStableTrackedComponent(Context* context) : BaseTrackedComponent(context) {}

    /// Manage stable ID of the component.
    /// @{
    void SetStableId(StableComponentId id) { stableId_ = id; }
    StableComponentId GetStableId() const { return stableId_; }
    /// @}

private:
    StableComponentId stableId_{};
};

/// Base class for component registry that keeps components derived from BaseStableTrackedComponent.
class URHO3D_API BaseStableComponentRegistry : public BaseComponentRegistry
{
public:
    BaseStableComponentRegistry(Context* context, StringHash componentType);

    BaseStableTrackedComponent* GetTrackedComponentByStableId(StableComponentId id, bool checkVersion = true) const;
    BaseStableTrackedComponent* GetTrackedComponentByStableIndex(unsigned index) const;

protected:
    void OnComponentAdded(BaseTrackedComponent* baseComponent) override;
    void OnComponentRemoved(BaseTrackedComponent* baseComponent) override;

private:
    struct RegistryEntry
    {
        BaseStableTrackedComponent* component_{};
        unsigned version_{};
    };

    unsigned AllocateStableIndex();
    void EnsureIndex(unsigned index);

    IndexAllocator<> stableIndexAllocator_;
    ea::vector<RegistryEntry> stableIndexToEntry_;
};

/// Indicates that object should remove self from registry if disabled.
struct EnabledOnlyTag {};

/// Template version of BaseTrackedComponent that automatically registers self in registry.
template <class TrackedComponentType, class RegistryComponentType, class ... Tags>
class TrackedComponent : public TrackedComponentType, public Tags...
{
public:
    explicit TrackedComponent(Context* context) : TrackedComponentType(context) {}

    RegistryComponentType* GetRegistry() const { return registry_; }

    bool ShouldBeTrackedInRegistry() const override
    {
        if constexpr (IsOnlyEnabledTracked)
            return this->IsEnabledEffective();
        else
            return false;
    }

    void ReconnectToRegistry() override
    {
        Scene* scene = this->GetScene();
        registry_ = scene ? scene->GetComponent<RegistryComponentType>() : nullptr;
    }

    void OnSetEnabled() override
    {
        if constexpr (IsOnlyEnabledTracked)
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
        auto newRegistry = scene ? scene->GetComponent<RegistryComponentType>() : nullptr;
        if (newRegistry == registry_)
            return;

        if (registry_ && this->IsTrackedInRegistry())
            registry_->RemoveTrackedComponent(this);

        registry_ = newRegistry;

        if (registry_ && this->ShouldBeTrackedInRegistry())
            registry_->AddTrackedComponent(this);
    }

private:
    static constexpr bool IsOnlyEnabledTracked = TupleHasTypeV<EnabledOnlyTag, ea::tuple<Tags...>>;

    WeakPtr<RegistryComponentType> registry_;
};

}
