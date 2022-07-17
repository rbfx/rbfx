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

#include "../Core/CommonEditorActions.h"

namespace Urho3D
{

CreateRemoveNodeAction::CreateRemoveNodeAction(Node* node, bool removed)
    : removed_(removed)
    , scene_(node->GetScene())
    , data_(node)
    , indexInParent_(node->GetParent()->GetChildIndex(node))
{
}

bool CreateRemoveNodeAction::CanUndoRedo() const
{
    return scene_ != nullptr;
}

void CreateRemoveNodeAction::Redo() const
{
    if (removed_)
        RemoveNode();
    else
        AddNode();
}

void CreateRemoveNodeAction::Undo() const
{
    if (removed_)
        AddNode();
    else
        RemoveNode();
}

void CreateRemoveNodeAction::AddNode() const
{
    if (!scene_)
        return;

    if (Node* node = data_.SpawnExact(scene_))
        node->GetParent()->ReorderChild(node, indexInParent_);
    else
        throw UndoException("Cannot create node with id {}", data_.GetId());
}

void CreateRemoveNodeAction::RemoveNode() const
{
    if (!scene_)
        return;

    if (Node* node = scene_->GetNode(data_.GetId()))
        node->Remove();
    else
        throw UndoException("Cannot remove node with id {}", data_.GetId());
}

CreateRemoveComponentAction::CreateRemoveComponentAction(Component* component, bool removed)
    : removed_(removed)
    , scene_(component->GetScene())
    , data_(component)
    , indexInParent_(component->GetNode()->GetComponentIndex(component))
{
}

bool CreateRemoveComponentAction::CanUndoRedo() const
{
    return scene_ != nullptr;
}

void CreateRemoveComponentAction::Redo() const
{
    if (removed_)
        RemoveComponent();
    else
        AddComponent();
}

void CreateRemoveComponentAction::Undo() const
{
    if (removed_)
        AddComponent();
    else
        RemoveComponent();
}

void CreateRemoveComponentAction::AddComponent() const
{
    if (!scene_)
        return;

    if (Component* component = data_.SpawnExact(scene_))
        component->GetNode()->ReorderComponent(component, indexInParent_);
    else
        throw UndoException("Cannot create component with id {}", data_.GetId());
}

void CreateRemoveComponentAction::RemoveComponent() const
{
    if (!scene_)
        return;

    if (Component* component = scene_->GetComponent(data_.GetId()))
        component->Remove();
    else
        throw UndoException("Cannot remove component with id {}", data_.GetId());
}

ChangeNodeTransformAction::ChangeNodeTransformAction(Node* node, const Transform& oldTransform)
    : scene_(node->GetScene())
    , nodes_{{node->GetID(), NodeData{oldTransform, node->GetDecomposedTransform()}}}
{
}

bool ChangeNodeTransformAction::CanUndoRedo() const
{
    return scene_ != nullptr;
}

void ChangeNodeTransformAction::Redo() const
{
    if (!scene_)
        return;

    for (const auto& [nodeId, nodeData] : nodes_)
    {
        if (Node* node = scene_->GetNode(nodeId))
            node->SetTransform(nodeData.newTransform_);
        else
            throw UndoException("Cannot find node with id {}", nodeId);
    }
}

void ChangeNodeTransformAction::Undo() const
{
    if (!scene_)
        return;

    for (const auto& [nodeId, nodeData] : nodes_)
    {
        if (Node* node = scene_->GetNode(nodeId))
            node->SetTransform(nodeData.oldTransform_);
        else
            throw UndoException("Cannot find node with id {}", nodeId);
    }
}

bool ChangeNodeTransformAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ChangeNodeTransformAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_)
        return false;

    for (const auto& [nodeId, nodeData] : otherAction->nodes_)
    {
        if (nodes_.contains(nodeId))
            nodes_[nodeId].newTransform_ = nodeData.newTransform_;
        else
            nodes_[nodeId] = nodeData;
    }
    return true;
}

bool ChangeNodeAttributesAction::CanUndoRedo() const
{
    if (!scene_)
        return false;

    const bool allNodesFound = std::all_of(nodeIds_.begin(), nodeIds_.end(),
        [this](unsigned nodeId) { return scene_->GetNode(nodeId) != nullptr; });
    return allNodesFound;
}

void ChangeNodeAttributesAction::Redo() const
{
    SetAttributeValues(newValues_);
}

void ChangeNodeAttributesAction::Undo() const
{
    SetAttributeValues(oldValues_);
}

void ChangeNodeAttributesAction::SetAttributeValues(const VariantVector& values) const
{
    if (!scene_)
        return;

    for (unsigned index = 0; index < nodeIds_.size(); ++index)
    {
        if (Node* node = scene_->GetNode(nodeIds_[index]))
            node->SetAttribute(attributeName_, values[index]);
    }
}

bool ChangeNodeAttributesAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ChangeNodeAttributesAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_ || nodeIds_ != otherAction->nodeIds_)
        return false;

    newValues_ = otherAction->newValues_;
    return true;
}

bool ChangeComponentAttributesAction::CanUndoRedo() const
{
    if (!scene_)
        return false;

    const bool allComponentsFound = std::all_of(componentIds_.begin(), componentIds_.end(),
        [this](unsigned componentId) { return scene_->GetComponent(componentId) != nullptr; });
    return allComponentsFound;
}

void ChangeComponentAttributesAction::Redo() const
{
    SetAttributeValues(newValues_);
}

void ChangeComponentAttributesAction::Undo() const
{
    SetAttributeValues(oldValues_);
}

void ChangeComponentAttributesAction::SetAttributeValues(const VariantVector& values) const
{
    if (!scene_)
        return;

    for (unsigned index = 0; index < componentIds_.size(); ++index)
    {
        if (Component* component = scene_->GetComponent(componentIds_[index]))
            component->SetAttribute(attributeName_, values[index]);
    }
}

bool ChangeComponentAttributesAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ChangeComponentAttributesAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_ || componentIds_ != otherAction->componentIds_)
        return false;

    newValues_ = otherAction->newValues_;
    return true;
}

}
