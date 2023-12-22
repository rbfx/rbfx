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

#include "../Precompiled.h"

#include "../Utility/SceneSelection.h"

#include <EASTL/sort.h>

namespace Urho3D
{

void PackedSceneSelection::Clear()
{
    nodeIds_.clear();
    componentIds_.clear();
    activeNodeOrSceneId_ = 0;
    activeNodeId_ = 0;
    activeComponentId_ = 0;
}

void PackedSceneSelection::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "NodeIds", nodeIds_);
    SerializeValue(archive, "ComponentIds", componentIds_);
    SerializeValue(archive, "ActiveNodeOrSceneId", activeNodeOrSceneId_);
    SerializeValue(archive, "ActiveNodeId", activeNodeId_);
    SerializeValue(archive, "ActiveComponentId", activeComponentId_);
}

bool PackedSceneSelection::operator==(const PackedSceneSelection& other) const
{
    return nodeIds_ == other.nodeIds_
        && componentIds_ == other.componentIds_
        && activeNodeOrSceneId_ == other.activeNodeOrSceneId_
        && activeNodeId_ == other.activeNodeId_
        && activeComponentId_ == other.activeComponentId_;
}

bool SceneSelection::IsSelected(Component* component) const
{
    return components_.contains(WeakPtr<Component>(component));
}

bool SceneSelection::IsSelected(Node* node, bool effectively) const
{
    return effectively
        ? effectiveNodesAndScenes_.contains(WeakPtr<Node>(node))
        : nodesAndScenes_.contains(WeakPtr<Node>(node));
}

bool SceneSelection::IsSelected(Object* object) const
{
    return objects_.contains(WeakPtr<Object>(object));
}

void SceneSelection::Update()
{
    const unsigned componentsSize = components_.size();
    const unsigned numNodes = nodesAndScenes_.size();

    ea::erase_if(components_, [](Component* component) { return component == nullptr; });
    ea::erase_if(nodesAndScenes_, [](Node* node) { return node == nullptr; });
    ea::erase_if(nodes_, [](Node* node) { return node == nullptr; });

    // No need to check nodes_ because it is a subset of nodesAndScenes_
    if (components_.size() != componentsSize || nodesAndScenes_.size() != numNodes)
    {
        UpdateEffectiveNodes();
        NotifyChanged();
    }
}

void SceneSelection::Save(PackedSceneSelection& packedSelection) const
{
    packedSelection.Clear();

    for (const WeakPtr<Node>& node : nodesAndScenes_)
    {
        if (node)
            packedSelection.nodeIds_.push_back(node->GetID());
    }

    for (const WeakPtr<Component>& component : components_)
    {
        if (component)
            packedSelection.componentIds_.push_back(component->GetID());
    }

    ea::sort(packedSelection.nodeIds_.begin(), packedSelection.nodeIds_.end());
    ea::sort(packedSelection.componentIds_.begin(), packedSelection.componentIds_.end());

    packedSelection.activeNodeOrSceneId_ = activeNodeOrScene_ ? activeNodeOrScene_->GetID() : 0;
    packedSelection.activeNodeId_ = activeNode_ ? activeNode_->GetID() : 0;
    const auto activeComponent = dynamic_cast<Component*>(activeObject_.Get());
    packedSelection.activeComponentId_ = activeComponent ? activeComponent->GetID() : 0;
}

void SceneSelection::Load(Scene* scene, const PackedSceneSelection& packedSelection)
{
    ClearInternal();

    for (unsigned nodeId : packedSelection.nodeIds_)
    {
        if (WeakPtr<Node> node{scene->GetNode(nodeId)})
        {
            nodesAndScenes_.emplace(node);
            if (node != scene)
                nodes_.emplace(node);
        }
    }

    for (unsigned componentId : packedSelection.componentIds_)
    {
        if (WeakPtr<Component> component{scene->GetComponent(componentId)})
            components_.emplace(component);
    }

    activeNodeOrScene_ = packedSelection.activeNodeOrSceneId_ != 0
        ? scene->GetNode(packedSelection.activeNodeOrSceneId_)
        : nullptr;
    activeNode_ = packedSelection.activeNodeId_ != 0
        ? scene->GetNode(packedSelection.activeNodeId_)
        : nullptr;
    activeObject_ = packedSelection.activeComponentId_ != 0
        ? scene->GetComponent(packedSelection.activeComponentId_)
        : nullptr;

    if (activeNode_ && activeNode_->GetScene() == activeNode_)
        activeNode_ = nullptr;
    if (!activeObject_)
        activeObject_ = activeNode_;

    UpdateEffectiveNodes();
    NotifyChanged();
}

PackedSceneSelection SceneSelection::Pack() const
{
    PackedSceneSelection packedSelection;
    Save(packedSelection);
    return packedSelection;
}

void SceneSelection::Clear()
{
    ClearInternal();
    NotifyChanged();
}

void SceneSelection::ClearInternal()
{
    objects_.clear();
    nodesAndScenes_.clear();
    nodes_.clear();
    components_.clear();

    activeNodeOrScene_ = nullptr;
    activeNode_ = nullptr;
    activeObject_ = nullptr;

    effectiveNodesAndScenes_.clear();
    effectiveNodes_.clear();
}

void SceneSelection::ConvertToNodes()
{
    if (components_.empty())
        return;

    for (Component* component : components_)
    {
        if (Node* node = component->GetNode())
            SetSelected(node, true);
    }
    components_.clear();

    NotifyChanged();
}

void SceneSelection::SetSelected(Component* component, bool selected, bool activated)
{
    if (!component)
        return;

    const WeakPtr<Component> weakComponent{component};

    if (selected)
    {
        if (const WeakPtr<Node> weakNode{component->GetNode()})
            UpdateActiveObject(weakNode, weakComponent, activated);
        objects_.insert(weakComponent);
        components_.insert(weakComponent);
    }
    else
    {
        objects_.erase(weakComponent);
        components_.erase(weakComponent);
    }

    UpdateEffectiveNodes();

    NotifyChanged();
}

void SceneSelection::SetSelected(Node* node, bool selected, bool activated)
{
    if (!node)
        return;

    const WeakPtr<Node> weakNode{node};

    if (selected)
    {
        UpdateActiveObject(weakNode, nullptr, activated);
        objects_.insert(weakNode);
        nodesAndScenes_.insert(weakNode);
        if (node->GetParent() != nullptr)
            nodes_.insert(weakNode);
    }
    else
    {
        objects_.erase(weakNode);
        nodesAndScenes_.erase(weakNode);
        nodes_.erase(weakNode);
    }

    UpdateEffectiveNodes();

    NotifyChanged();
}

void SceneSelection::SetSelected(Object* object, bool selected, bool activated)
{
    if (auto node = dynamic_cast<Node*>(object))
        SetSelected(node, selected, activated);
    else if (auto component = dynamic_cast<Component*>(object))
        SetSelected(component, selected, activated);
    else
        URHO3D_ASSERT(0, "SceneSelection::SetSelected received unexpected object");
}

void SceneSelection::NotifyChanged()
{
    revision_ = ea::max(1u, revision_ + 1);
    OnChanged(this);
}

void SceneSelection::UpdateActiveObject(const WeakPtr<Node>& node, const WeakPtr<Component>& component, bool forceUpdate)
{
    if (activeNodeOrScene_ == nullptr || forceUpdate)
    {
        activeNodeOrScene_ = node;
    }

    if (activeNode_ == nullptr || forceUpdate)
    {
        if (node->GetParent() != nullptr)
            activeNode_ = node;
    }

    if (activeObject_ == nullptr || forceUpdate)
    {
        if (component)
            activeObject_ = component;
        else
            activeObject_ = node;
    }
}

void SceneSelection::UpdateEffectiveNodes()
{
    effectiveNodesAndScenes_.clear();
    effectiveNodes_.clear();

    for (Node* node : nodes_)
    {
        if (const WeakPtr<Node> weakNode{node})
        {
            if (weakNode->GetParent() != nullptr)
                effectiveNodesAndScenes_.insert(weakNode);
            effectiveNodes_.insert(weakNode);
        }
    }
    for (Component* component : components_)
    {
        if (component)
        {
            if (const WeakPtr<Node> weakNode{component->GetNode()})
            {
                if (weakNode->GetParent() != nullptr)
                    effectiveNodesAndScenes_.insert(weakNode);
                effectiveNodes_.insert(weakNode);
            }
        }
    }

    if (!effectiveNodesAndScenes_.contains(activeNodeOrScene_))
        activeNodeOrScene_ = !effectiveNodesAndScenes_.empty() ? *effectiveNodesAndScenes_.begin() : nullptr;

    if (!effectiveNodes_.contains(activeNode_))
        activeNode_ = !effectiveNodes_.empty() ? *effectiveNodes_.begin() : nullptr;

    if (!objects_.contains(activeObject_))
        activeObject_ = !objects_.empty() ? *objects_.begin() : nullptr;
}

ea::string SceneSelection::GetSummary(Scene* scene) const
{
    const unsigned numNodes = nodes_.size();
    const unsigned numComponents = components_.size();
    const bool hasScene = nodesAndScenes_.size() != numNodes;

    StringVector elements;

    if (hasScene)
        elements.push_back("Scene");

    if (numNodes == 1)
    {
        Node* node = *nodes_.begin();
        if (!node->GetName().empty())
            elements.push_back(Format("Node '{}'", node->GetName()));
        else
            elements.push_back(Format("Node {}", node->GetID()));
    }
    else if (numNodes > 1)
        elements.push_back(Format("{} Nodes", numNodes));

    if (numComponents == 1)
    {
        Component* component = *components_.begin();
        elements.push_back(component->GetTypeName());
    }
    else if (numComponents > 1)
        elements.push_back(Format("{} Components", numComponents));

    return ea::string::joined(elements, ", ");
}

}
