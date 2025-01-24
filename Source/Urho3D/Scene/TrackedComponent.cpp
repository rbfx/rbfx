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

#include <Urho3D/Precompiled.h>

#include <Urho3D/Core/Assert.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/TrackedComponent.h>

namespace Urho3D
{

namespace
{

constexpr unsigned maxIndex = 0xffffff;
constexpr unsigned maxVersion = 0xff;
constexpr unsigned indexOffset = 0;
constexpr unsigned versionOffset = 24;

} // namespace

TrackedComponentRegistryBase::TrackedComponentRegistryBase(Context* context, StringHash componentType)
    : Component(context)
    , componentType_(componentType)
{
}

TrackedComponentBase* TrackedComponentRegistryBase::GetTrackedComponentByIndex(unsigned index) const
{
    return index < componentsArray_.size() ? componentsArray_[index] : nullptr;
}

void TrackedComponentRegistryBase::AddTrackedComponent(TrackedComponentBase* component)
{
    const unsigned oldIndex = component->GetIndexInArray();
    if (oldIndex != M_MAX_UNSIGNED)
    {
        URHO3D_ASSERTLOG(GetTrackedComponentByIndex(oldIndex) == component, "Component array is corrupted");
        URHO3D_ASSERTLOG(0, "Component is already tracked at #{}", oldIndex);
        return;
    }

    const unsigned index = componentsArray_.size();
    componentsArray_.push_back(component);
    component->SetIndexInArray(index);
    OnComponentAdded(component);
}

void TrackedComponentRegistryBase::RemoveTrackedComponent(TrackedComponentBase* component)
{
    const unsigned index = component->GetIndexInArray();
    if (index == M_MAX_UNSIGNED)
    {
        URHO3D_ASSERTLOG(0, "Component is not tracked");
        return;
    }
    if (GetTrackedComponentByIndex(index) != component)
    {
        URHO3D_ASSERTLOG(0, "Component array is corrupted");
        return;
    }

    OnComponentRemoved(component);

    // Do swap-erase trick if possible
    const unsigned numComponents = componentsArray_.size();
    if (numComponents > 1 && index + 1 != numComponents)
    {
        TrackedComponentBase* replacement = componentsArray_.back();
        componentsArray_[index] = replacement;
        replacement->SetIndexInArray(index);
        OnComponentMoved(replacement, numComponents - 1);
    }

    componentsArray_.pop_back();
    component->SetIndexInArray(M_MAX_UNSIGNED);
}

void TrackedComponentRegistryBase::OnSceneSet(Scene* scene)
{
    if (scene)
    {
        InitializeTrackedComponents();
        OnAddedToScene(scene);
    }
    else
    {
        OnRemovedFromScene();
        DeinitializeTrackedComponents();
    }
}

void TrackedComponentRegistryBase::InitializeTrackedComponents()
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    if (!componentsArray_.empty())
    {
        URHO3D_ASSERTLOG(0, "Invalid call to InitializeTrackedComponents()");
        componentsArray_.clear();
    }

    ea::vector<TrackedComponentBase*> components;
    scene->FindComponents(components, ComponentSearchFlag::SelfOrChildrenRecursive | ComponentSearchFlag::Derived);
    for (TrackedComponentBase* component : components)
    {
        if (!component->IsInstanceOf(componentType_))
            continue;

        if (component->ShouldBeTrackedInRegistry())
        {
            component->ReconnectToRegistry();
            AddTrackedComponent(component);
        }
    }
}

void TrackedComponentRegistryBase::DeinitializeTrackedComponents()
{
    for (TrackedComponentBase* component : componentsArray_)
    {
        OnComponentRemoved(component);
        component->SetIndexInArray(M_MAX_UNSIGNED);
    }

    componentsArray_.clear();
}

ComponentReference ConstructComponentReference(unsigned index, unsigned version)
{
    unsigned result = 0;
    result |= (index & maxIndex) << indexOffset;
    result |= (version & maxVersion) << versionOffset;
    return static_cast<ComponentReference>(result);
}

ea::pair<unsigned, unsigned> DeconstructComponentReference(ComponentReference componentId)
{
    const auto value = static_cast<unsigned>(componentId);
    return {(value >> indexOffset) & maxIndex, (value >> versionOffset) & maxVersion};
}

ea::string ToString(ComponentReference value)
{
    const auto [index, version] = DeconstructComponentReference(value);
    return value == ComponentReference::None ? "(null)" : Format("{}:{}", index, version);
}

ReferencedComponentRegistryBase::ReferencedComponentRegistryBase(Context* context, StringHash componentType)
    : TrackedComponentRegistryBase(context, componentType)
{
}

ReferencedComponentBase* ReferencedComponentRegistryBase::GetTrackedComponentByReference(
    ComponentReference id, bool checkVersion) const
{
    const auto [index, version] = DeconstructComponentReference(id);
    if (index >= referenceIndexToEntry_.size())
        return nullptr;

    const RegistryEntry& entry = referenceIndexToEntry_[index];
    return (!checkVersion || entry.version_ == version) ? entry.component_ : nullptr;
}

ReferencedComponentBase* ReferencedComponentRegistryBase::GetTrackedComponentByReferenceIndex(unsigned index) const
{
    return index < referenceIndexToEntry_.size() ? referenceIndexToEntry_[index].component_ : nullptr;
}

void ReferencedComponentRegistryBase::OnComponentAdded(TrackedComponentBase* baseComponent)
{
    auto component = static_cast<ReferencedComponentBase*>(baseComponent);
    ComponentReference reference = component->GetReference();

    // Try to reuse existing ID if possible
    if (reference != ComponentReference::None)
    {
        if (GetTrackedComponentByReference(reference) == component)
        {
            URHO3D_ASSERTLOG(0, "Component is already tracked as {}", ToString(reference));
            return;
        }

        const auto [index, version] = DeconstructComponentReference(reference);
        ReferencedComponentBase* otherComponent = GetTrackedComponentByReferenceIndex(index);
        if (otherComponent)
        {
            URHO3D_LOGWARNING("Another component is already tracked as {}");
            reference = ComponentReference::None;
        }
        else
        {
            EnsureIndex(index);
            referenceIndexToEntry_[index].version_ = version;
        }
    }

    // Allocate new ID if needed
    if (reference == ComponentReference::None)
    {
        const unsigned index = AllocateReferenceIndex();
        reference = ConstructComponentReference(index, referenceIndexToEntry_[index].version_);
    }

    const auto [index, version] = DeconstructComponentReference(reference);

    RegistryEntry& entry = referenceIndexToEntry_[index];
    entry.component_ = component;

    component->SetReference(reference);
}

void ReferencedComponentRegistryBase::OnComponentRemoved(TrackedComponentBase* baseComponent)
{
    auto component = static_cast<ReferencedComponentBase*>(baseComponent);
    const ComponentReference reference = component->GetReference();

    if (reference == ComponentReference::None)
    {
        URHO3D_ASSERTLOG(0, "Component is not tracked");
        return;
    }
    if (GetTrackedComponentByReference(reference) != component)
    {
        URHO3D_ASSERTLOG(0, "Component array is corrupted at {}", ToString(reference));
        return;
    }

    component->SetReference(ComponentReference::None);

    const auto [index, version] = DeconstructComponentReference(reference);

    RegistryEntry& entry = referenceIndexToEntry_[index];
    entry.component_ = nullptr;
    entry.version_ = (entry.version_ + 1) % maxVersion;

    referenceIndexAllocator_.Release(index);
}

unsigned ReferencedComponentRegistryBase::AllocateReferenceIndex()
{
    // May need more than one attept if some indices are taken bypassing IndexAllocator
    for (unsigned i = 0; i <= maxIndex; ++i)
    {
        const unsigned index = referenceIndexAllocator_.Allocate();
        EnsureIndex(index);
        if (!referenceIndexToEntry_[index].component_)
            return index;
    }

    URHO3D_LOGERROR("Failed to allocate reference for component");
    assert(0);
    return 0;
}

void ReferencedComponentRegistryBase::EnsureIndex(unsigned index)
{
    if (index >= referenceIndexToEntry_.size())
        referenceIndexToEntry_.resize(index + 1);
}

} // namespace Urho3D
