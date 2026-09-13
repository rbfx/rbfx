// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/IteratorRange.h"
#include "../Core/Signal.h"
#include "../IO/VectorBuffer.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"

namespace Urho3D
{

/// Packed node data.
class URHO3D_API PackedNodeData
{
public:
    PackedNodeData() = default;
    /// Create from existing node.
    explicit PackedNodeData(Node* node);

    /// Spawn exact node in the scene. May fail.
    Node* SpawnExact(Scene* scene) const;
    /// Spawn similar node at the parent.
    Node* SpawnCopy(Node* parent) const;

    /// Return node ID.
    unsigned GetId() const { return id_; }
    /// Return whether the node spawn would affect the entire scene.
    /// Used to correctly handle undo/redo of node creation.
    AttributeScopeHint GetEffectiveScopeHint() const { return scopeHint_; }

private:
    unsigned id_{};
    unsigned parentId_{};
    unsigned indexInParent_{};
    ea::string name_;
    VectorBuffer data_;
    AttributeScopeHint scopeHint_{};
};

/// Packed component data.
class URHO3D_API PackedComponentData
{
public:
    PackedComponentData() = default;
    /// Create from existing component.
    explicit PackedComponentData(Component* component);

    /// Spawn exact component in the scene. May fail.
    Component* SpawnExact(Scene* scene) const;
    /// Spawn similar component at the node. May fail only if component type is unknown.
    Component* SpawnCopy(Node* node) const;
    /// Update attributes of existing component.
    void Update(Component* component) const;

    /// Return component ID.
    unsigned GetId() const { return id_; }
    StringHash GetType() const { return type_; }

private:
    unsigned id_{};
    unsigned nodeId_{};
    unsigned indexInParent_{};
    StringHash type_;
    VectorBuffer data_;
};

/// Packed nodes and components.
class URHO3D_API PackedNodeComponentData
{
public:
    PackedNodeComponentData() = default;

    /// Pack nodes.
    template <class Iter>
    static PackedNodeComponentData FromNodes(Iter begin, Iter end)
    {
        PackedNodeComponentData result;
        for (Node* node : MakeIteratorRange(begin, end))
        {
            if (node)
                result.nodes_.emplace_back(node);
        }
        return result;
    }

    /// Pack components.
    template <class Iter>
    static PackedNodeComponentData FromComponents(Iter begin, Iter end)
    {
        PackedNodeComponentData result;
        for (Component* component : MakeIteratorRange(begin, end))
        {
            if (component)
                result.components_.emplace_back(component);
        }
        return result;
    }

    /// Return contents
    /// @{
    bool HasNodes() const { return !nodes_.empty(); }
    const ea::vector<PackedNodeData>& GetNodes() const { return nodes_; }
    bool HasComponents() const { return !components_.empty(); }
    const ea::vector<PackedComponentData>& GetComponents() const { return components_; }
    bool HasNodesOrComponents() const { return HasNodes() || HasComponents(); }
    /// @}

private:
    ea::vector<PackedNodeData> nodes_;
    ea::vector<PackedComponentData> components_;
};

/// Packed Scene as whole.
class URHO3D_API PackedSceneData
{
public:
    PackedSceneData() = default;

    /// Load into scene.
    void ToScene(Scene* scene, PrefabLoadFlags loadFlags = PrefabLoadFlag::None) const;

    /// Pack whole scene.
    static PackedSceneData FromScene(Scene* scene);

    /// Return contents
    /// @{
    const VectorBuffer& GetSceneData() const { return sceneData_; }
    bool HasSceneData() const { return sceneData_.GetSize() > 0; }
    /// @}

private:
    VectorBuffer sceneData_;
};

}
