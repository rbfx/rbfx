// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Signal.h"
#include "../Scene/Scene.h"
#include "../Utility/PackedSceneData.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

/// Packed selected nodes and components.
struct URHO3D_API PackedSceneSelection
{
    ea::vector<unsigned> nodeIds_;
    ea::vector<unsigned> componentIds_;

    unsigned activeNodeOrSceneId_{};
    unsigned activeNodeId_{};
    unsigned activeComponentId_{};

    void Clear();
    void SerializeInBlock(Archive& archive);

    bool operator==(const PackedSceneSelection& other) const;
    bool operator!=(const PackedSceneSelection& other) const { return !(*this == other); }
};

/// Selected nodes and components in the Scene.
class URHO3D_API SceneSelection
{
public:
    Signal<void(), SceneSelection> OnChanged;

    using WeakNodeSet = ea::unordered_set<WeakPtr<Node>>;
    using WeakComponentSet = ea::unordered_set<WeakPtr<Component>>;
    using WeakObjectSet = ea::unordered_set<WeakPtr<Object>>;

    /// Return current state
    /// @{
    unsigned GetRevision() const { return revision_; }
    bool IsEmpty() const { return nodesAndScenes_.empty() && components_.empty(); }
    bool IsSelected(Component* component) const;
    bool IsSelected(Node* node, bool effectively = false) const;
    bool IsSelected(Object* object) const;

    Node* GetActiveNodeOrScene() const { return activeNodeOrScene_; }
    Node* GetActiveNode() const { return activeNode_; }
    Object* GetActiveObject() const { return activeObject_; }
    const WeakNodeSet& GetNodesAndScenes() const { return nodesAndScenes_; }
    const WeakNodeSet& GetNodes() const { return nodes_; }
    const WeakNodeSet& GetEffectiveNodesAndScenes() const { return effectiveNodesAndScenes_; }
    const WeakNodeSet& GetEffectiveNodes() const { return effectiveNodes_; }
    const WeakComponentSet& GetComponents() const { return components_; }

    ea::string GetSummary(Scene* scene) const;
    /// @}

    /// Cleanup expired selection.
    void Update();
    /// Save selection.
    void Save(PackedSceneSelection& packedSelection) const;
    /// Load selection.
    void Load(Scene* scene, const PackedSceneSelection& packedSelection);
    /// Return packed selection.
    PackedSceneSelection Pack() const;

    /// Clear selection.
    void Clear();
    /// Convert component selection to node selection.
    void ConvertToNodes();
    /// Set whether the component is selected.
    void SetSelected(Component* component, bool selected, bool activated = true);
    /// Set whether the node is selected.
    void SetSelected(Node* node, bool selected, bool activated = true);
    /// Set whether the node or component is selected.
    void SetSelected(Object* object, bool selected, bool activated = true);

private:
    void ClearInternal();
    void NotifyChanged();
    void UpdateActiveObject(const WeakPtr<Node>& node, const WeakPtr<Component>& component, bool forceUpdate);
    void UpdateEffectiveNodes();

    WeakObjectSet objects_;
    WeakNodeSet nodesAndScenes_;
    WeakNodeSet nodes_;
    WeakComponentSet components_;

    WeakPtr<Node> activeNodeOrScene_;
    WeakPtr<Node> activeNode_;
    WeakPtr<Object> activeObject_;

    WeakNodeSet effectiveNodesAndScenes_;
    WeakNodeSet effectiveNodes_;
    unsigned revision_{1};
};

}
