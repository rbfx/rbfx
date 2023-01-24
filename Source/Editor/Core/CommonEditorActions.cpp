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

#include <EASTL/bonus/adaptors.h>

namespace Urho3D
{

bool CompositeEditorAction::CanRedo() const
{
    const auto canRedo = [](const SharedPtr<EditorAction>& action) { return action->CanRedo(); };
    return ea::all_of(actions_.begin(), actions_.end(), canRedo);
}

bool CompositeEditorAction::CanUndo() const
{
    const auto canUndo = [](const SharedPtr<EditorAction>& action) { return action->CanUndo(); };
    return ea::all_of(actions_.begin(), actions_.end(), canUndo);
}

void CompositeEditorAction::Redo() const
{
    for (const auto& action : actions_)
        action->Redo();
}

void CompositeEditorAction::Undo() const
{
    for (const auto& action : ea::reverse(actions_))
        action->Undo();
}

CreateRemoveNodeAction::CreateRemoveNodeAction(Node* node, bool removed)
    : removed_(removed)
    , scene_(node->GetScene())
    , data_(node)
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

    Node* node = data_.SpawnExact(scene_);
    if (!node)
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

    Component* component = data_.SpawnExact(scene_);
    if (!component)
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

    const bool allNodesFound = ea::all_of(nodeIds_.begin(), nodeIds_.end(),
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
        {
            node->SetAttribute(attributeName_, values[index]);
            node->ApplyAttributes();
        }
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

    const bool allComponentsFound = ea::all_of(componentIds_.begin(), componentIds_.end(),
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
        {
            component->SetAttribute(attributeName_, values[index]);
            component->ApplyAttributes();
        }
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

ReorderNodeAction::ReorderNodeAction(Node* node, unsigned oldIndex, unsigned newIndex)
    : scene_(node->GetScene())
    , parentNodeId_(node->GetParent()->GetID())
    , nodeId_(node->GetID())
    , oldIndex_(oldIndex)
    , newIndex_(newIndex)
{
}

bool ReorderNodeAction::CanUndoRedo() const
{
    return scene_ && scene_->GetNode(parentNodeId_) && scene_->GetNode(nodeId_);
}

void ReorderNodeAction::Redo() const
{
    Reorder(newIndex_);
}

void ReorderNodeAction::Undo() const
{
    Reorder(oldIndex_);
}

void ReorderNodeAction::Reorder(unsigned index) const
{
    Node* parentNode = scene_->GetNode(parentNodeId_);
    Node* node = scene_->GetNode(nodeId_);
    if (!parentNode)
        throw UndoException("Cannot find parent node with id {}", parentNodeId_);
    else if (!node)
        throw UndoException("Cannot find node with id {}", nodeId_);
    else
        parentNode->ReorderChild(node, index);
}

bool ReorderNodeAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ReorderNodeAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_ || nodeId_ != otherAction->nodeId_ || parentNodeId_ != otherAction->parentNodeId_)
        return false;

    newIndex_ = otherAction->newIndex_;
    return true;
}

ReorderComponentAction::ReorderComponentAction(Component* component, unsigned oldIndex, unsigned newIndex)
    : scene_(component->GetScene())
    , nodeId_(component->GetNode()->GetID())
    , componentId_(component->GetID())
    , oldIndex_(oldIndex)
    , newIndex_(newIndex)
{
}

bool ReorderComponentAction::CanUndoRedo() const
{
    return scene_ && scene_->GetNode(nodeId_) && scene_->GetComponent(componentId_);
}

void ReorderComponentAction::Redo() const
{
    Reorder(newIndex_);
}

void ReorderComponentAction::Undo() const
{
    Reorder(oldIndex_);
}

void ReorderComponentAction::Reorder(unsigned index) const
{
    Node* node = scene_->GetNode(nodeId_);
    Component* component = scene_->GetComponent(componentId_);
    if (!node)
        throw UndoException("Cannot find node with id {}", nodeId_);
    else if (!component)
        throw UndoException("Cannot find component with id {}", componentId_);
    else
        node->ReorderComponent(component, index);
}

bool ReorderComponentAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ReorderComponentAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_ || nodeId_ != otherAction->nodeId_ || componentId_ != otherAction->componentId_)
        return false;

    newIndex_ = otherAction->newIndex_;
    return true;
}

ReparentNodeAction::ReparentNodeAction(Node* node, Node* newParent)
    : scene_(node->GetScene())
    , nodeId_(node->GetID())
    , oldParentId_(node->GetParent()->GetID())
    , oldIndex_(node->GetIndexInParent())
    , newParentId_(newParent->GetID())
{
}

bool ReparentNodeAction::CanUndoRedo() const
{
    return scene_ && scene_->GetNode(nodeId_) && scene_->GetNode(oldParentId_) && scene_->GetNode(newParentId_);
}

void ReparentNodeAction::Redo() const
{
    Reparent(newParentId_, ea::nullopt);
}

void ReparentNodeAction::Undo() const
{
    Reparent(oldParentId_, oldIndex_);
}

void ReparentNodeAction::Reparent(unsigned parentId, ea::optional<unsigned> index) const
{
    Node* node = scene_->GetNode(nodeId_);
    Node* parent = scene_->GetNode(parentId);
    if (!node)
        throw UndoException("Cannot find node with id {}", nodeId_);
    else if (!parent)
        throw UndoException("Cannot find parent node with id {}", parentId);
    else
    {
        node->SetParent(parent);
        if (index)
            parent->ReorderChild(node, *index);
    }
}

bool ReparentNodeAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ReparentNodeAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_ || nodeId_ != otherAction->nodeId_)
        return false;

    newParentId_ = otherAction->newParentId_;
    return true;
}

ChangeNodeSubtreeAction::ChangeNodeSubtreeAction(Scene* scene, const PackedNodeData& oldData, Node* newData)
    : scene_(scene)
    , oldData_(oldData)
    , newData_(newData ? PackedNodeData{newData} : PackedNodeData{})
    , newRemoved_(newData == nullptr)
{
}

ChangeNodeSubtreeAction::ChangeNodeSubtreeAction(
    Scene* scene, const PackedNodeData& oldData, const PackedNodeData& newData)
    : scene_(scene)
    , oldData_(oldData)
    , newData_(newData)
    , newRemoved_(false)
{
}

bool ChangeNodeSubtreeAction::CanUndoRedo() const
{
    return scene_;
}

void ChangeNodeSubtreeAction::Redo() const
{
    UpdateSubtree(oldData_.GetId(), newRemoved_ ? nullptr : &newData_);
}

void ChangeNodeSubtreeAction::Undo() const
{
    UpdateSubtree(oldData_.GetId(), &oldData_);
}

void ChangeNodeSubtreeAction::UpdateSubtree(unsigned nodeId, const PackedNodeData* data) const
{
    if (Node* oldNode = scene_->GetNode(nodeId))
        oldNode->Remove();
    if (data)
        data->SpawnExact(scene_);
}

bool ChangeNodeSubtreeAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ChangeNodeSubtreeAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_)
        return false;

    newData_ = otherAction->newData_;
    newRemoved_ = otherAction->newRemoved_;
    return true;
}

ChangeSceneAction::ChangeSceneAction(Scene* scene, const PackedSceneData& oldData)
    : scene_(scene)
    , oldData_(oldData)
{
    newData_.FromScene(scene);
}

ChangeSceneAction::ChangeSceneAction(Scene* scene, const PackedSceneData& oldData, const PackedSceneData& newData)
    : scene_(scene)
    , oldData_(oldData)
    , newData_(newData)
{
}

bool ChangeSceneAction::CanUndoRedo() const
{
    return scene_;
}

void ChangeSceneAction::Redo() const
{
    UpdateScene(newData_);
}

void ChangeSceneAction::Undo() const
{
    UpdateScene(oldData_);
}

void ChangeSceneAction::UpdateScene(const PackedSceneData& data) const
{
    data.ToScene(scene_);
}

bool ChangeSceneAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ChangeSceneAction*>(&other);
    if (!otherAction)
        return false;

    if (scene_ != otherAction->scene_)
        return false;

    newData_ = otherAction->newData_;
    return true;
}

}
