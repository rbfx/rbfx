//
// Copyright (c) 2017-2020 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/UndoManager.h"

#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Utility/PackedSceneData.h>

namespace Urho3D
{

/// Empty action.
class EmptyEditorAction : public EditorAction
{
public:
    /// Implement EditorAction.
    /// @{
    bool RemoveOnUndo() const override { return true; }
    void Redo() const override {}
    void Undo() const override {}
    /// @}
};

/// Create or remove node.
class CreateRemoveNodeAction : public EditorAction
{
public:
    CreateRemoveNodeAction(Node* node, bool removed);

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    /// @}

private:
    void AddNode() const;
    void RemoveNode() const;

    const bool removed_{};
    WeakPtr<Scene> scene_;
    PackedNodeData data_;
    unsigned indexInParent_{};
};

/// Create or remove component.
class CreateRemoveComponentAction : public EditorAction
{
public:
    CreateRemoveComponentAction(Component* component, bool removed);

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    /// @}

private:
    void AddComponent() const;
    void RemoveComponent() const;

    const bool removed_{};
    WeakPtr<Scene> scene_;
    PackedComponentData data_;
    unsigned indexInParent_{};
};

/// Change node transform.
class ChangeNodeTransformAction : public EditorAction
{
public:
    ChangeNodeTransformAction(Node* node, const Transform& oldTransform);

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    struct NodeData
    {
        Transform oldTransform_;
        Transform newTransform_;
    };
    WeakPtr<Scene> scene_;
    ea::unordered_map<unsigned, NodeData> nodes_;
};

/// Change attribute values of nodes.
class ChangeNodeAttributesAction : public EditorAction
{
public:
    template <class T>
    ChangeNodeAttributesAction(Scene* scene, const ea::string& attributeName,
        const T& nodes, const VariantVector& oldValues, const VariantVector& newValues)
        : scene_(scene)
        , attributeName_(attributeName)
        , oldValues_(oldValues)
        , newValues_(newValues)
    {
        using namespace ea;
        ea::transform(begin(nodes), end(nodes), std::back_inserter(nodeIds_),
            [](Node* node) { return node->GetID(); });

        URHO3D_ASSERT(nodeIds_.size() == oldValues_.size());
        URHO3D_ASSERT(nodeIds_.size() == newValues_.size());
    }

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    void SetAttributeValues(const VariantVector& values) const;

    const WeakPtr<Scene> scene_;
    const ea::string attributeName_;
    ea::vector<unsigned> nodeIds_;
    VariantVector oldValues_;
    VariantVector newValues_;
};

/// Change attribute values of a components.
class ChangeComponentAttributesAction : public EditorAction
{
public:
    template <class T>
    ChangeComponentAttributesAction(Scene* scene, const ea::string& attributeName,
        const T& components, const VariantVector& oldValues, const VariantVector& newValues)
        : scene_(scene)
        , attributeName_(attributeName)
        , oldValues_(oldValues)
        , newValues_(newValues)
    {
        using namespace ea;
        ea::transform(begin(components), end(components), std::back_inserter(componentIds_),
            [](Component* component) { return component->GetID(); });

        URHO3D_ASSERT(componentIds_.size() == oldValues_.size());
        URHO3D_ASSERT(componentIds_.size() == newValues_.size());
    }

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    void SetAttributeValues(const VariantVector& values) const;

    const WeakPtr<Scene> scene_;
    const ea::string attributeName_;
    ea::vector<unsigned> componentIds_;
    VariantVector oldValues_;
    VariantVector newValues_;
};

/// Reorder node in the parent node.
class ReorderNodeAction : public EditorAction
{
public:
    ReorderNodeAction(Node* node, unsigned oldIndex, unsigned newIndex);

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    void Reorder(unsigned index) const;

    const WeakPtr<Scene> scene_;
    const unsigned parentNodeId_{};
    const unsigned nodeId_{};
    const unsigned oldIndex_{};
    unsigned newIndex_{};
};

/// Reorder component in the node.
class ReorderComponentAction : public EditorAction
{
public:
    ReorderComponentAction(Component* component, unsigned oldIndex, unsigned newIndex);

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    void Reorder(unsigned index) const;

    const WeakPtr<Scene> scene_;
    const unsigned nodeId_{};
    const unsigned componentId_{};
    const unsigned oldIndex_{};
    unsigned newIndex_{};
};

/// Reparent node.
class ReparentNodeAction : public EditorAction
{
public:
    ReparentNodeAction(Node* node, Node* oldParent);

    /// Implement EditorAction.
    /// @{
    bool CanUndoRedo() const override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    void Reparent(unsigned parentId) const;

    const WeakPtr<Scene> scene_;
    const unsigned nodeId_{};
    const unsigned oldParentId_{};
    unsigned newParentId_{};
};

}
