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

#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include "SceneClipboard.h"


namespace Urho3D
{

SceneClipboard::SceneClipboard(Context* context)
    : Object(context)
{
}

void SceneClipboard::Clear()
{
    nodes_.clear();
    components_.clear();
}

void SceneClipboard::Copy(Node* node)
{
    nodes_.resize(nodes_.size() + 1);
    node->Save(nodes_.back());
}

void SceneClipboard::Copy(Component* component)
{
    components_.resize(components_.size() + 1);
    component->Save(components_.back());
}

PasteResult SceneClipboard::Paste(Node* node)
{
    PasteResult result;

    for (VectorBuffer& data : components_)
    {
        data.Seek(0);
        StringHash componentType = data.ReadStringHash();
        unsigned componentID = data.ReadUInt();
        Component* component = node->CreateComponent(componentType, componentID < FIRST_LOCAL_ID ? REPLICATED : LOCAL);
        if (component->Load(data))
        {
            component->ApplyAttributes();
            result.components_.push_back(component);
        }
        else
            component->Remove();
    }

    for (VectorBuffer& nodeData : nodes_)
    {
        nodeData.Seek(0);
        auto nodeID = nodeData.ReadUInt();
        SharedPtr<Node> newNode(node->CreateChild(EMPTY_STRING, nodeID < FIRST_LOCAL_ID ? REPLICATED : LOCAL));
        nodeData.Seek(0);
        if (newNode->Load(nodeData))
        {
            newNode->ApplyAttributes();
            if (!newNode->GetName().empty())
            {
                auto undo = GetSubsystem<UndoStack>();
                undo->Add<UndoEditAttribute>(newNode, "Name", EMPTY_STRING, newNode->GetName());
            }
            result.nodes_.push_back(newNode);
        }
        else
            newNode->Remove();
    }

    return result;
}

PasteResult SceneClipboard::Paste(const ea::hash_set<WeakPtr<Node>>& nodes)
{
    PasteResult result;

    for (auto& node : nodes)
        result.Merge(Paste(node));

    return result;
}

void SceneClipboard::Copy(const ea::hash_set<WeakPtr<Node>>& nodes)
{
    for (auto& node : nodes)
        Copy(node);
}

void SceneClipboard::Copy(const ea::hash_set<WeakPtr<Component>>& components)
{
    for (auto& component : components)
        Copy(component);
}

}
