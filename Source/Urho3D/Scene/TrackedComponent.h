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
#include "../Scene/Component.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

/// Base class for all tracked components.
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

private:
    unsigned indexInArray_{M_MAX_UNSIGNED};
};

/// Base class for component registry that keeps continuous array of all components in the Scene.
class URHO3D_API BaseComponentRegistry : public Component
{
public:
    BaseComponentRegistry(Context* context, StringHash componentType);

    BaseTrackedComponent* GetTrackedComponentByIndex(unsigned index) const;
    unsigned GetNumTrackedComponents() const { return componentsArray_.size(); }

    /// Internal. Manage tracked components.
    /// @{
    void AddTrackedComponent(BaseTrackedComponent* component);
    void RemoveTrackedComponent(BaseTrackedComponent* component);
    /// @}

protected:
    void OnSceneSet(Scene* scene) override;

    virtual void OnComponentAdded(BaseTrackedComponent* component) {}
    virtual void OnComponentMoved(BaseTrackedComponent* component, unsigned oldIndex) {}
    virtual void OnComponentRemoved(BaseTrackedComponent* component) {}

private:
    void InitializeTrackedComponents();
    void DeinitializeTrackedComponents();

    const StringHash componentType_{};
    ea::vector<BaseTrackedComponent*> componentsArray_;
};

/// Indicates that object should remove self from registry if disabled.
struct EnabledOnlyTag {};

/// Template version of BaseTrackedComponent that automatically registers self in registry.
template <class RegistryComponentType, class ... Tags>
class TrackedComponent : public BaseTrackedComponent, public Tags...
{
public:
    explicit TrackedComponent(Context* context) : BaseTrackedComponent(context) {}

    bool ShouldBeTrackedInRegistry() const override
    {
        if constexpr (IsOnlyEnabledTracked)
            return IsEnabledEffective();
        else
            return false;
    }

    void OnSetEnabled() override
    {
        if constexpr (IsOnlyEnabledTracked)
        {
            const bool wasEnabled = IsTrackedInRegistry();
            const bool isEnabled = ShouldBeTrackedInRegistry();
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

        if (registry_ && IsTrackedInRegistry())
            registry_->RemoveTrackedComponent(this);

        registry_ = newRegistry;

        if (registry_ && ShouldBeTrackedInRegistry())
            registry_->AddTrackedComponent(this);
    }

private:
    static constexpr bool IsOnlyEnabledTracked = TupleHasTypeV<EnabledOnlyTag, ea::tuple<Tags...>>;

    WeakPtr<RegistryComponentType> registry_;
};

}
