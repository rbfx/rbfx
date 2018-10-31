//
// Copyright (c) 2018 Rokas Kupstys
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

SceneClipboard::SceneClipboard(Context* context, Undo::Manager& undo)
    : Object(context)
    , undo_(undo)
{
}

void SceneClipboard::Clear()
{
    nodes_.Clear();
    components_.Clear();
}

void SceneClipboard::Copy(Node* node)
{
    nodes_.Resize(nodes_.Size() + 1);
    node->Save(nodes_.Back());
}

void SceneClipboard::Copy(Component* component)
{
    components_.Resize(components_.Size() + 1);
    component->Save(components_.Back());
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
            result.components_.Push(component);
        }
        else
            component->Remove();
    }

    for (VectorBuffer& nodeData : nodes_)
    {
        nodeData.Seek(0);
        auto nodeID = nodeData.ReadUInt();
        SharedPtr<Node> newNode(node->CreateChild(String::EMPTY, nodeID < FIRST_LOCAL_ID ? REPLICATED : LOCAL));
        nodeData.Seek(0);
        if (newNode->Load(nodeData))
        {
            newNode->ApplyAttributes();
            if (!newNode->GetName().Empty())
                undo_.Track<Undo::EditAttributeAction>(newNode, "Name", String::EMPTY, newNode->GetName());
            result.nodes_.Push(newNode);
        }
        else
            newNode->Remove();
    }

    return result;
}

PasteResult SceneClipboard::Paste(const PODVector<Node*>& nodes)
{
    PasteResult result;

    for (auto& node : nodes)
        result.Merge(Paste(node));

    return result;
}

PasteResult SceneClipboard::Paste(const Vector<WeakPtr<Node>>& nodes)
{
    PasteResult result;

    for (auto& node : nodes)
        result.Merge(Paste(node));

    return result;
}

void SceneClipboard::Copy(const PODVector<Node*>& nodes)
{
    for (auto& node : nodes)
        Copy(node);
}

void SceneClipboard::Copy(const PODVector<Component*>& components)
{
    for (auto& node : components)
        Copy(node);
}

void SceneClipboard::Copy(const Vector<WeakPtr<Node>>& nodes)
{
    for (auto& node : nodes)
        Copy(node);
}

void SceneClipboard::Copy(const HashSet<WeakPtr<Component>>& components)
{
    for (auto& node : components)
        Copy(node);
}

}
