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

#include "../Scene/TrackedComponent.h"

#include "../Core/Assert.h"

namespace Urho3D
{

BaseComponentRegistry::BaseComponentRegistry(Context* context, StringHash componentType)
    : Component(context)
    , componentType_(componentType)
{
}

BaseTrackedComponent* BaseComponentRegistry::GetTrackedComponentByIndex(unsigned index) const
{
    return index < componentsArray_.size() ? componentsArray_[index] : nullptr;
}

void BaseComponentRegistry::AddTrackedComponent(BaseTrackedComponent* component)
{
    const unsigned index = component->GetIndexInArray();
    if (index != M_MAX_UNSIGNED)
    {
        URHO3D_ASSERTLOG(GetTrackedComponentByIndex(index) == component, "Component array is corrupted");
        URHO3D_ASSERTLOG(0, "Component is already tracked at #{}", index);
        return;
    }

    const unsigned newPackedIndex = componentsArray_.size();
    componentsArray_.push_back(component);
    component->SetIndexInArray(newPackedIndex);
    OnComponentAdded(component);
}

void BaseComponentRegistry::RemoveTrackedComponent(BaseTrackedComponent* component)
{
    const unsigned index = component->GetIndexInArray();
    if (index == M_MAX_UNSIGNED)
    {
        URHO3D_ASSERTLOG(0, "Component is not tracked");
        return;
    }

    OnComponentRemoved(component);

    // Do swap-erase trick if possible
    const unsigned numComponents = componentsArray_.size();
    if (numComponents > 1 && index + 1 != numComponents)
    {
        BaseTrackedComponent* replacement = componentsArray_.back();
        componentsArray_[index] = replacement;
        replacement->SetIndexInArray(index);
        OnComponentMoved(replacement, numComponents - 1);
    }

    componentsArray_.pop_back();
    component->SetIndexInArray(M_MAX_UNSIGNED);
}

void BaseComponentRegistry::OnSceneSet(Scene* scene)
{
    if (scene)
        InitializeTrackedComponents();
    else
        DeinitializeTrackedComponents();
}

void BaseComponentRegistry::InitializeTrackedComponents()
{
    Scene* scene = GetScene();
    if (!scene)
        return;

    if (!componentsArray_.empty())
    {
        URHO3D_ASSERTLOG(0, "Invalid call to InitializeTrackedComponents()");
        componentsArray_.clear();
    }

    ea::vector<Component*> components;
    scene->GetComponents(components, componentType_, true);
    for (Component* component : components)
    {
        auto trackedComponent = static_cast<BaseTrackedComponent*>(component);
        if (trackedComponent->ShouldBeTrackedInRegistry())
            AddTrackedComponent(trackedComponent);
    }
}

void BaseComponentRegistry::DeinitializeTrackedComponents()
{
    for (BaseTrackedComponent* component : componentsArray_)
    {
        OnComponentRemoved(component);
        component->SetIndexInArray(M_MAX_UNSIGNED);
    }

    componentsArray_.clear();
}

}
