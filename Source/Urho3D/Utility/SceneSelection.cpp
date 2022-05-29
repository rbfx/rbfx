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

#include "../Utility/SceneSelection.h"

namespace Urho3D
{

bool SceneSelection::IsSelected(Component* component) const
{
    return components_.contains(WeakPtr<Component>(component));
}

bool SceneSelection::IsSelected(Node* node) const
{
    return nodesAndScenes_.contains(WeakPtr<Node>(node));
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
        UpdateRevision();
        UpdateEffectiveNodes();
    }
}

void SceneSelection::Clear()
{
    UpdateRevision();

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
    UpdateRevision();

    for (Component* component : components_)
    {
        if (Node* node = component->GetNode())
            SetSelected(node, true);
    }
    components_.clear();
}

void SceneSelection::SetSelected(Component* component, bool selected, bool activated)
{
    if (!component)
        return;

    UpdateRevision();

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
}

void SceneSelection::SetSelected(Node* node, bool selected, bool activated)
{
    if (!node)
        return;

    UpdateRevision();

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

}
