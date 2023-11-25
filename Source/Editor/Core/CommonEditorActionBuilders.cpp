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

#include "../Core/CommonEditorActionBuilders.h"

#include "../Core/CommonEditorActions.h"

namespace Urho3D
{

namespace
{

AttributeScopeHint GetScopeHint(Context* context, const StringHash& componentType)
{
    ObjectReflection* reflection = context->GetReflection(componentType);
    return reflection ? reflection->GetEffectiveScopeHint() : AttributeScopeHint::Serializable;
}

AttributeScopeHint GetScopeHint(Context* context, Node* node, const AttributeInfo& attr)
{
    // For nodes, "Is Enabled" is special because it effectively propagates to children components.
    // Other attributes always have the smallest scope.
    if (attr.name_ != "Is Enabled")
        return AttributeScopeHint::Attribute;

    AttributeScopeHint result = AttributeScopeHint::Attribute;
    for (Component* component : node->GetComponents())
        result = ea::max(result, component->GetEffectiveScopeHint());
    return result;
}

AttributeScopeHint GetScopeHint(Context* context, const ea::vector<Node*>& nodes, const AttributeInfo& attr)
{
    AttributeScopeHint result = AttributeScopeHint::Attribute;
    for (Node* node : nodes)
        result = ea::max(result, GetScopeHint(context, node, attr));
    return result;
}

template <class T>
ea::vector<WeakPtr<T>> ToWeakPtr(const ea::vector<T*>& source)
{
    ea::vector<WeakPtr<T>> result;
    result.reserve(source.size());
    for (T* item : source)
        result.emplace_back(item);
    return result;
}

}

CreateNodeActionBuilder::CreateNodeActionBuilder(Scene* scene, AttributeScopeHint scopeHint)
    : scene_(scene)
    , scopeHint_(scopeHint)
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    case AttributeScopeHint::Node:
    {
        // No need to prepare
        break;
    }
    case AttributeScopeHint::Scene:
    {
        oldSceneData_ = PackedSceneData::FromScene(scene);
        break;
    }
    };
}

SharedPtr<EditorAction> CreateNodeActionBuilder::Build(Node* node) const
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    case AttributeScopeHint::Node:
    {
        return MakeShared<CreateRemoveNodeAction>(node, false);
    }
    case AttributeScopeHint::Scene:
    {
        return MakeShared<ChangeSceneAction>(scene_, oldSceneData_);
    }
    default: return nullptr;
    }
}

RemoveNodeActionBuilder::RemoveNodeActionBuilder(Node* node)
    : scene_(node->GetScene())
    , scopeHint_(node->GetEffectiveScopeHint())
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    case AttributeScopeHint::Node:
    {
        action_ = MakeShared<CreateRemoveNodeAction>(node, true);
        break;
    }
    case AttributeScopeHint::Scene:
    {
        oldSceneData_ = PackedSceneData::FromScene(scene_);
        break;
    }
    };
}

SharedPtr<EditorAction> RemoveNodeActionBuilder::Build() const
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    case AttributeScopeHint::Node:
    {
        return action_;
    }
    case AttributeScopeHint::Scene:
    {
        return MakeShared<ChangeSceneAction>(scene_, oldSceneData_);
    }
    default: return nullptr;
    }
}

CreateComponentActionBuilder::CreateComponentActionBuilder(Node* node, StringHash componentType)
    : scene_(node->GetScene())
    , scopeHint_(GetScopeHint(scene_->GetContext(), componentType))
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    {
        // No need to prepare
        break;
    }
    case AttributeScopeHint::Node:
    {
        oldNodeData_ = PackedNodeData{node};
        break;
    }
    case AttributeScopeHint::Scene:
    {
        oldSceneData_ = PackedSceneData::FromScene(scene_);
        break;
    }
    };
}

SharedPtr<EditorAction> CreateComponentActionBuilder::Build(Component* component) const
{
    URHO3D_ASSERTLOG(scopeHint_ == GetScopeHint(scene_->GetContext(), component->GetType()));

    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    {
        return MakeShared<CreateRemoveComponentAction>(component, false);
    }
    case AttributeScopeHint::Node:
    {
        Node* node = component->GetNode();
        return MakeShared<ChangeNodeSubtreeAction>(scene_, oldNodeData_, node);
    }
    case AttributeScopeHint::Scene:
    {
        return MakeShared<ChangeSceneAction>(scene_, oldSceneData_);
    }
    default: return nullptr;
    }
}

RemoveComponentActionBuilder::RemoveComponentActionBuilder(Component* component)
    : scene_(component->GetScene())
    , node_(component->GetNode())
    , scopeHint_(GetScopeHint(scene_->GetContext(), component->GetType()))
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    {
        action_ = MakeShared<CreateRemoveComponentAction>(component, true);
        break;
    }
    case AttributeScopeHint::Node:
    {
        oldNodeData_ = PackedNodeData{node_};
        break;
    }
    case AttributeScopeHint::Scene:
    {
        oldSceneData_ = PackedSceneData::FromScene(scene_);
        break;
    }
    };
}

SharedPtr<EditorAction> RemoveComponentActionBuilder::Build() const
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    {
        return action_;
    }
    case AttributeScopeHint::Node:
    {
        return MakeShared<ChangeNodeSubtreeAction>(scene_, oldNodeData_, node_);
    }
    case AttributeScopeHint::Scene:
    {
        return MakeShared<ChangeSceneAction>(scene_, oldSceneData_);
    }
    default: return nullptr;
    }
}

ChangeNodeAttributesActionBuilder::ChangeNodeAttributesActionBuilder(
    ChangeAttributeBuffer& buffer, Scene* scene, const ea::vector<Node*>& nodes, const AttributeInfo& attr)
    : buffer_(buffer)
    , scene_(scene)
    , attributeName_(attr.name_)
    , scopeHint_(GetScopeHint(scene_->GetContext(), nodes, attr))
    , nodes_(scopeHint_ <= AttributeScopeHint::Serializable ? ToWeakPtr(nodes) : ToWeakPtr(Node::GetParentNodes(nodes)))
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    {
        buffer_.oldValues_.clear();
        for (Node* node : nodes)
            buffer_.oldValues_.push_back(node->GetAttribute(attributeName_));
        break;
    }
    case AttributeScopeHint::Node:
    {
        buffer_.oldNodes_.clear();
        for (Node* node : nodes)
            buffer_.oldNodes_.emplace_back(node);
        break;
    }
    case AttributeScopeHint::Scene:
    {
        buffer_.oldScene_ = PackedSceneData::FromScene(scene_);
        break;
    }
    default: break;
    };
}

SharedPtr<EditorAction> ChangeNodeAttributesActionBuilder::Build() const
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    case AttributeScopeHint::Serializable:
    {
        buffer_.newValues_.clear();
        for (Node* node : nodes_)
            buffer_.newValues_.push_back(node->GetAttribute(attributeName_));

        return MakeShared<ChangeNodeAttributesAction>(
            scene_, attributeName_, nodes_, buffer_.oldValues_, buffer_.newValues_);
    }
    case AttributeScopeHint::Node:
    {
        buffer_.newNodes_.clear();
        for (Node* node : nodes_)
            buffer_.newNodes_.emplace_back(node);

        auto compositeAction = MakeShared<CompositeEditorAction>();
        for (unsigned index = 0; index < nodes_.size(); ++index)
        {
            compositeAction->EmplaceAction<ChangeNodeSubtreeAction>(
                scene_, buffer_.oldNodes_[index], buffer_.newNodes_[index]);
        }
        return compositeAction;
    }
    case AttributeScopeHint::Scene:
    {
        buffer_.newScene_ = PackedSceneData::FromScene(scene_);
        return MakeShared<ChangeSceneAction>(scene_, buffer_.oldScene_, buffer_.newScene_);
    }
    default: return nullptr;
    }
}

ChangeComponentAttributesActionBuilder::ChangeComponentAttributesActionBuilder(
    ChangeAttributeBuffer& buffer, Scene* scene, const ea::vector<Component*>& components, const AttributeInfo& attr)
    : buffer_(buffer)
    , scene_(scene)
    , attributeName_(attr.name_)
    , scopeHint_(attr.scopeHint_)
    , components_(ToWeakPtr(components))
    , nodes_(ToWeakPtr(Node::GetParentNodes(Node::GetNodes(components))))
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    {
        buffer_.oldValues_.clear();
        for (Component* component : components)
            buffer_.oldValues_.push_back(component->GetAttribute(attributeName_));
        break;
    }

    case AttributeScopeHint::Serializable:
    {
        buffer_.oldComponents_.clear();
        for (Component* component : components)
            buffer_.oldComponents_.emplace_back(component);
        break;
    }

    case AttributeScopeHint::Node:
    {
        buffer_.oldNodes_.clear();
        for (Node* node : nodes_)
            buffer_.oldNodes_.emplace_back(node);
        break;
    }

    case AttributeScopeHint::Scene:
    {
        buffer_.oldScene_ = PackedSceneData::FromScene(scene_);
        break;
    }

    default: break;
    }
}

SharedPtr<EditorAction> ChangeComponentAttributesActionBuilder::Build() const
{
    switch (scopeHint_)
    {
    case AttributeScopeHint::Attribute:
    {
        buffer_.newValues_.clear();
        for (Component* component : components_)
            buffer_.newValues_.push_back(component->GetAttribute(attributeName_));

        return MakeShared<ChangeComponentAttributesAction>(
            scene_, attributeName_, components_, buffer_.oldValues_, buffer_.newValues_);
    }

    case AttributeScopeHint::Serializable:
    {
        buffer_.newComponents_.clear();
        for (Component* component : components_)
            buffer_.newComponents_.emplace_back(component);

        auto compositeAction = MakeShared<CompositeEditorAction>();
        for (unsigned index = 0; index < components_.size(); ++index)
        {
            compositeAction->EmplaceAction<ChangeComponentAction>(
                scene_, buffer_.oldComponents_[index], buffer_.newComponents_[index]);
        }
        return compositeAction;
    }

    case AttributeScopeHint::Node:
    {
        buffer_.newNodes_.clear();
        for (Node* node : nodes_)
            buffer_.newNodes_.emplace_back(node);

        auto compositeAction = MakeShared<CompositeEditorAction>();
        for (unsigned index = 0; index < nodes_.size(); ++index)
        {
            compositeAction->EmplaceAction<ChangeNodeSubtreeAction>(
                scene_, buffer_.oldNodes_[index], buffer_.newNodes_[index]);
        }
        return compositeAction;
    }

    case AttributeScopeHint::Scene:
    {
        buffer_.newScene_ = PackedSceneData::FromScene(scene_);

        return MakeShared<ChangeSceneAction>(scene_, buffer_.oldScene_, buffer_.newScene_);
    }

    default: return nullptr;
    }
}

}
