// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <EASTL/unordered_map.h>

#include <Urho3D/Urho3D.h>
#include "../Container/Ptr.h"

namespace Urho3D
{

class Component;
class Node;

/// Utility class that resolves node & component IDs after a scene or partial scene load.
class URHO3D_API SceneResolver
{
public:
    /// Construct.
    SceneResolver();
    /// Destruct.
    ~SceneResolver();

    /// Reset. Clear all remembered nodes and components.
    void Reset();
    /// Remember a created node.
    void AddNode(unsigned oldID, Node* node);
    /// Remember a created component.
    void AddComponent(unsigned oldID, Component* component);
    /// Resolve component and node ID attributes and reset.
    void Resolve();

private:
    /// Node by old Id.
    ea::unordered_map<unsigned, WeakPtr<Node> > nodeLookup_;
    /// Component by old Id.
    ea::unordered_map<unsigned, WeakPtr<Component> > componentLookup_;
    /// Components to resolve.
    ea::vector<WeakPtr<Component>> components_;
};

}
