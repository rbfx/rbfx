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

/// Helper factory classes to create actions.
/// These classes should be created before the action is started, and cooked after the action is finished.
/// @{

class CreateNodeActionBuilder
{
public:
    CreateNodeActionBuilder(Scene* scene, AttributeScopeHint scopeHint);

    SharedPtr<EditorAction> Build(Node* node) const;

private:
    const WeakPtr<Scene> scene_;
    const AttributeScopeHint scopeHint_;

    PackedSceneData oldSceneData_;
};

class RemoveNodeActionBuilder
{
public:
    explicit RemoveNodeActionBuilder(Node* node);

    SharedPtr<EditorAction> Build() const;

private:
    const WeakPtr<Scene> scene_;
    const AttributeScopeHint scopeHint_;

    SharedPtr<EditorAction> action_;
    PackedSceneData oldSceneData_;
};

class CreateComponentActionBuilder
{
public:
    CreateComponentActionBuilder(Node* node, StringHash componentType);

    SharedPtr<EditorAction> Build(Component* component) const;

private:
    const WeakPtr<Scene> scene_;
    const AttributeScopeHint scopeHint_;

    PackedNodeData oldNodeData_;
    PackedSceneData oldSceneData_;
};

class RemoveComponentActionBuilder
{
public:
    explicit RemoveComponentActionBuilder(Component* component);

    SharedPtr<EditorAction> Build() const;

private:
    const WeakPtr<Scene> scene_;
    const WeakPtr<Node> node_;
    const AttributeScopeHint scopeHint_;

    SharedPtr<EditorAction> action_;
    PackedNodeData oldNodeData_;
    PackedSceneData oldSceneData_;
};

struct ChangeAttributeBuffer
{
    VariantVector oldValues_;
    VariantVector newValues_;

    ea::vector<PackedComponentData> oldComponents_;
    ea::vector<PackedComponentData> newComponents_;

    ea::vector<PackedNodeData> oldNodes_;
    ea::vector<PackedNodeData> newNodes_;

    PackedSceneData oldScene_;
    PackedSceneData newScene_;
};

class ChangeNodeAttributesActionBuilder
{
public:
    ChangeNodeAttributesActionBuilder(ChangeAttributeBuffer& buffer, Scene* scene,
        const ea::vector<Node*>& nodes, const AttributeInfo& attr);

    template <class T>
    ChangeNodeAttributesActionBuilder(
        ChangeAttributeBuffer& buffer, Scene* scene, const T& nodes, const AttributeInfo& attr)
        : ChangeNodeAttributesActionBuilder(buffer, scene, ToNodeVector(nodes), attr)
    {
    }

    SharedPtr<EditorAction> Build() const;

private:
    template <class T>
    static ea::vector<Node*> ToNodeVector(const T& components)
    {
        using namespace ea;
        return {begin(components), end(components)};
    }

    ChangeAttributeBuffer& buffer_;
    const WeakPtr<Scene> scene_;

    const ea::string attributeName_;
    const AttributeScopeHint scopeHint_;

    const ea::vector<WeakPtr<Node>> nodes_;
};

class ChangeComponentAttributesActionBuilder
{
public:
    ChangeComponentAttributesActionBuilder(ChangeAttributeBuffer& buffer, Scene* scene,
        const ea::vector<Component*>& components, const AttributeInfo& attr);

    template <class T>
    ChangeComponentAttributesActionBuilder(
        ChangeAttributeBuffer& buffer, Scene* scene, const T& components, const AttributeInfo& attr)
        : ChangeComponentAttributesActionBuilder(buffer, scene, ToComponentsVector(components), attr)
    {
    }

    SharedPtr<EditorAction> Build() const;

private:
    template <class T>
    static ea::vector<Component*> ToComponentsVector(const T& components)
    {
        using namespace ea;
        return {begin(components), end(components)};
    }

    ChangeAttributeBuffer& buffer_;
    const WeakPtr<Scene> scene_;

    const ea::string attributeName_;
    const AttributeScopeHint scopeHint_;

    const ea::vector<WeakPtr<Component>> components_;
    const ea::vector<WeakPtr<Node>> nodes_;
};

/// @}

}
