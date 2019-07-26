//
// Copyright (c) 2017-2019 the rbfx project.
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

#include "ToolboxAPI.h"
#include <Urho3D/Core/Object.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>


namespace Urho3D
{

class AttributeInspector;
class Gizmo;
class Scene;

/// Notify undo managers that state is about to be undone.
URHO3D_EVENT(E_UNDO, Undo)
{
    URHO3D_PARAM(P_TIME, Time);                       // unsigned.
    URHO3D_PARAM(P_MANAGER, Manager);                 // Undo::Manager pointer.
}

/// Notify undo managers that state is about to be redone.
URHO3D_EVENT(E_REDO, Redo)
{
    URHO3D_PARAM(P_TIME, Time);                       // unsigned.
    URHO3D_PARAM(P_MANAGER, Manager);                 // Undo::Manager pointer.
}

namespace Undo
{

class URHO3D_TOOLBOX_API EditAction : public RefCounted
{
public:
    virtual void Undo() = 0;
    virtual void Redo() = 0;

    /// Time when action was recorded.
    unsigned time_ = Time::GetSystemTime();
};

class URHO3D_TOOLBOX_API CreateNodeAction : public EditAction
{
    unsigned parentID;
    VectorBuffer nodeData;
    WeakPtr<Scene> editorScene;

public:
    explicit CreateNodeAction(Node* node)
        : editorScene(node->GetScene())
    {
        parentID = node->GetParent()->GetID();
        node->Save(nodeData);
    }

    void Undo() override
    {
        nodeData.Seek(0);
        auto nodeID = nodeData.ReadUInt();
        Node* parent = editorScene->GetNode(parentID);
        Node* node = editorScene->GetNode(nodeID);
        if (parent != nullptr && node != nullptr)
        {
            parent->RemoveChild(node);
        }
    }

    void Redo() override
    {
        Node* parent = editorScene->GetNode(parentID);
        if (parent != nullptr)
        {
            nodeData.Seek(0);
            auto nodeID = nodeData.ReadUInt();
            nodeData.Seek(0);

            Node* node = parent->CreateChild(EMPTY_STRING, nodeID < FIRST_LOCAL_ID ? REPLICATED : LOCAL, nodeID);
            node->Load(nodeData);
            // FocusNode(node);
        }
    }
};

class URHO3D_TOOLBOX_API DeleteNodeAction : public EditAction
{
    unsigned parentID;
    unsigned parentIndex;
    VectorBuffer nodeData;
    WeakPtr<Scene> editorScene;

public:
    explicit DeleteNodeAction(Node* node)
        : editorScene(node->GetScene())
    {
        parentID = node->GetParent()->GetID();
        parentIndex = node->GetParent()->GetChildren().index_of(SharedPtr<Node>(node));
        node->Save(nodeData);
    }

    void Undo() override
    {
        Node* parent = editorScene->GetNode(parentID);
        if (parent != nullptr)
        {
            nodeData.Seek(0);
            auto nodeID = nodeData.ReadUInt();
            SharedPtr<Node> node(new Node(parent->GetContext()));
            node->SetID(nodeID);
            parent->AddChild(node, parentIndex);
            nodeData.Seek(0);
            node->Load(nodeData);
        }
    }

    void Redo() override
    {
        nodeData.Seek(0);
        auto nodeID = nodeData.ReadUInt();

        Node* parent = editorScene->GetNode(parentID);
        Node* node = editorScene->GetNode(nodeID);
        if (parent != nullptr && node != nullptr)
        {
            parent->RemoveChild(node);
        }
    }
};

class URHO3D_TOOLBOX_API ReparentNodeAction : public EditAction
{
    unsigned nodeID;
    unsigned oldParentID;
    unsigned newParentID;
    ea::vector<unsigned> nodeList; // 2 uints get inserted per node (node, node->GetParent())
    bool multiple;
    WeakPtr<Scene> editorScene;

public:
    ReparentNodeAction(Node* node, Node* newParent)
        : editorScene(node->GetScene())
    {
        multiple = false;
        nodeID = node->GetID();
        oldParentID = node->GetParent()->GetID();
        newParentID = newParent->GetID();
    }

    ReparentNodeAction(const ea::vector<Node*>& nodes, Node* newParent)
        : editorScene(newParent->GetScene())
    {
        multiple = true;
        newParentID = newParent->GetID();
        for(unsigned i = 0; i < nodes.size(); ++i)
        {
            Node* node = nodes[i];
            nodeList.push_back(node->GetID());
            nodeList.push_back(node->GetParent()->GetID());
        }
    }

    void Undo() override
    {
        if (multiple)
        {
            for (unsigned i = 0; i < nodeList.size(); i+=2)
            {
                unsigned nodeID_ = nodeList[i];
                unsigned oldParentID_ = nodeList[i+1];
                Node* parent = editorScene->GetNode(oldParentID_);
                Node* node = editorScene->GetNode(nodeID_);
                if (parent != nullptr && node != nullptr)
                node->SetParent(parent);
            }
        }
        else
        {
            Node* parent = editorScene->GetNode(oldParentID);
            Node* node = editorScene->GetNode(nodeID);
            if (parent != nullptr && node != nullptr)
            node->SetParent(parent);
        }
    }

    void Redo() override
    {
        if (multiple)
        {
            Node* parent = editorScene->GetNode(newParentID);
            if (parent == nullptr)
                return;

            for (unsigned i = 0; i < nodeList.size(); i+=2)
            {
                unsigned nodeID_ = nodeList[i];
                Node* node = editorScene->GetNode(nodeID_);
                if (node != nullptr)
                node->SetParent(parent);
            }
        }
        else
        {
            Node* parent = editorScene->GetNode(newParentID);
            Node* node = editorScene->GetNode(nodeID);
            if (parent != nullptr && node != nullptr)
            node->SetParent(parent);
        }
    }
};

class URHO3D_TOOLBOX_API CreateComponentAction : public EditAction
{
    unsigned nodeID;
    VectorBuffer componentData;
    WeakPtr<Scene> editorScene;

public:
    explicit CreateComponentAction(Component* component)
        : editorScene(component->GetScene())
    {
        nodeID = component->GetNode()->GetID();
        component->Save(componentData);
    }

    void Undo() override
    {
        componentData.Seek(sizeof(StringHash));
        auto componentID = componentData.ReadUInt();
        Node* node = editorScene->GetNode(nodeID);
        Component* component = editorScene->GetComponent(componentID);
        if (node != nullptr && component != nullptr)
            node->RemoveComponent(component);
    }

    void Redo() override
    {
        Node* node = editorScene->GetNode(nodeID);
        if (node != nullptr)
        {
            componentData.Seek(0);
            auto componentType = componentData.ReadStringHash();
            auto componentID = componentData.ReadUInt();

            Component* component = node->CreateComponent(componentType,
                componentID < FIRST_LOCAL_ID ? REPLICATED : LOCAL, componentID);
            component->Load(componentData);
            component->ApplyAttributes();
            // FocusComponent(component);
        }
    }

};

class URHO3D_TOOLBOX_API DeleteComponentAction : public EditAction
{
    unsigned nodeID;
    VectorBuffer componentData;
    WeakPtr<Scene> editorScene;

public:
    DeleteComponentAction(Component* component)
        : editorScene(component->GetScene())
    {
        nodeID = component->GetNode()->GetID();
        component->Save(componentData);
    }

    void Undo() override
    {
        Node* node = editorScene->GetNode(nodeID);
        if (node != nullptr)
        {
            componentData.Seek(0);
            auto componentType = componentData.ReadStringHash();
            unsigned componentID = componentData.ReadUInt();
            Component* component = node->CreateComponent(componentType, componentID < FIRST_LOCAL_ID ? REPLICATED : LOCAL, componentID);

            if (component->Load(componentData))
            {
                component->ApplyAttributes();
                // FocusComponent(component);
            }
        }
    }

    void Redo() override
    {
        componentData.Seek(sizeof(StringHash));
        unsigned componentID = componentData.ReadUInt();

        Node* node = editorScene->GetNode(nodeID);
        Component* component = editorScene->GetComponent(componentID);
        if (node != nullptr && component != nullptr)
        {
            node->RemoveComponent(component);
        }
    }
};

using UIElementPath = ea::vector<unsigned>;

static UIElementPath GetUIElementPath(UIElement* element)
{
    ea::vector<unsigned> path;
    unsigned pathCount = 1;
    for (UIElement* el = element; el->GetParent() != nullptr; el = el->GetParent())
        pathCount++;
    path.reserve(pathCount);

    for (UIElement* el = element; el->GetParent() != nullptr; el = el->GetParent())
    {
        UIElement* parent = el->GetParent();
        unsigned index = parent->FindChild(el);
        assert(index != M_MAX_UNSIGNED);
        path.push_back(index);
    }
    ea::reverse(path.begin(), path.end());
    return path;
}

static UIElement* GetUIElementByPath(UIElement* el, const UIElementPath& path)
{
    for (unsigned index : path)
    {
        const ea::vector<SharedPtr<UIElement>>& children = el->GetChildren();
        if (index >= children.size())
            return nullptr;
        el = children[index].Get();
    }
    return el;
}

class URHO3D_TOOLBOX_API EditAttributeAction : public EditAction
{
    unsigned targetID;
    UIElementPath targetPath_;
    ea::string attrName_;
    Variant undoValue_;
    Variant redoValue_;
    StringHash targetType_;
    WeakPtr<Scene> editorScene_;
    WeakPtr<UIElement> root_;
    WeakPtr<Serializable> target_;

public:
    EditAttributeAction(Serializable* target, const ea::string& name, const Variant& oldValue, const Variant& newValue)
    {
        attrName_ = name;
        undoValue_ = oldValue;
        redoValue_ = newValue;
        targetType_ = target->GetType();
        target_ = target;

        if (auto* node = dynamic_cast<Node*>(target))
        {
            editorScene_ = node->GetScene();
            targetID = node->GetID();
        }
        else if (auto* component = dynamic_cast<Component*>(target))
        {
            editorScene_ = component->GetScene();
            targetID = component->GetID();
        }
        else if (auto* element = dynamic_cast<UIElement*>(target))
        {
            root_ = element->GetRoot();
            targetPath_ = GetUIElementPath(element);
        }
    }

    Serializable* GetTarget()
    {
        if (targetType_ == Node::GetTypeStatic())
            return editorScene_->GetNode(targetID);
        else if (targetType_ == Component::GetTypeStatic())
            return editorScene_->GetComponent(targetID);
        else if (targetType_ == UIElement::GetTypeStatic())
            return GetUIElementByPath(root_, targetPath_);

        return target_.Get();
    }

    void Undo() override
    {
        Serializable* target = GetTarget();
        if (target != nullptr)
        {
            target->SetAttribute(attrName_, undoValue_);
            target->ApplyAttributes();
        }
    }

    void Redo() override
    {
        Serializable* target = GetTarget();
        if (target != nullptr)
        {
            target->SetAttribute(attrName_, redoValue_);
            target->ApplyAttributes();
        }
    }
};

class URHO3D_TOOLBOX_API CreateUIElementAction : public EditAction
{
    UIElementPath elementPath_;
    UIElementPath parentPath_;
    XMLFile elementData_;
    SharedPtr<XMLFile> styleFile_;
    WeakPtr<UIElement> root_;

public:
    explicit CreateUIElementAction(UIElement* element)
        : elementData_(element->GetContext())
    {
        root_ = element->GetRoot();
        elementPath_ = GetUIElementPath(element);
        parentPath_ = GetUIElementPath(element->GetParent());
        XMLElement rootElem = elementData_.CreateRoot("element");
        element->SaveXML(rootElem);
        rootElem.SetUInt("index", element->GetParent()->FindChild(element));
        styleFile_ = element->GetDefaultStyle();
    }

    void Undo() override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        UIElement* element = GetUIElementByPath(root_, elementPath_);
        if (parent != nullptr && element != nullptr)
            parent->RemoveChild(element);
    }

    void Redo() override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        if (parent != nullptr)
            parent->LoadChildXML(elementData_.GetRoot(), styleFile_);
    }
};

class URHO3D_TOOLBOX_API DeleteUIElementAction : public EditAction
{
    UIElementPath elementPath_;
    UIElementPath parentPath_;
    XMLFile elementData_;
    SharedPtr<XMLFile> styleFile_;
    WeakPtr<UIElement> root_;

public:
    explicit DeleteUIElementAction(UIElement* element)
        : elementData_(element->GetContext())
    {
        root_ = element->GetRoot();
        elementPath_ = GetUIElementPath(element);
        parentPath_ = GetUIElementPath(element->GetParent());
        XMLElement rootElem = elementData_.CreateRoot("element");
        element->SaveXML(rootElem);
        rootElem.SetUInt("index", element->GetParent()->FindChild(element));
        styleFile_ = SharedPtr<XMLFile>(element->GetDefaultStyle());
    }

    void Undo() override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        if (parent != nullptr)
            parent->LoadChildXML(elementData_.GetRoot(), styleFile_);
    }

    void Redo() override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        UIElement* element = GetUIElementByPath(root_, elementPath_);
        if (parent != nullptr && element != nullptr)
            parent->RemoveChild(element);
    }
};

class URHO3D_TOOLBOX_API ReparentUIElementAction : public EditAction
{
    UIElementPath elementPath_;
    UIElementPath oldParentPath_;
    unsigned oldChildIndex_;
    UIElementPath newParentPath_;
    WeakPtr<UIElement> root;

public:
    ReparentUIElementAction(UIElement* element, UIElement* newParent)
    : root(element->GetRoot())
    {
        elementPath_ = GetUIElementPath(element);
        oldParentPath_ = GetUIElementPath(element->GetParent());
        oldChildIndex_ = element->GetParent()->FindChild(element);
        newParentPath_ = GetUIElementPath(newParent);
    }

    void Undo() override
    {
        UIElement* parent = GetUIElementByPath(root, oldParentPath_);
        UIElement* element = GetUIElementByPath(root, elementPath_);
        if (parent != nullptr && element != nullptr)
            element->SetParent(parent, oldChildIndex_);
    }

    void Redo() override
    {
        UIElement* parent = GetUIElementByPath(root, newParentPath_);
        UIElement* element = GetUIElementByPath(root, elementPath_);
        if (parent != nullptr && element != nullptr)
            element->SetParent(parent);
    }
};

class URHO3D_TOOLBOX_API ApplyUIElementStyleAction : public EditAction
{
    UIElementPath elementPath_;
    UIElementPath parentPath_;
    XMLFile elementData_;
    XMLFile* styleFile_;
    ea::string elementOldStyle_;
    ea::string elementNewStyle_;
    WeakPtr<UIElement> root_;

public:
    ApplyUIElementStyleAction(UIElement* element, const ea::string& newStyle)
        : elementData_(element->GetContext())
    {
        root_ = element->GetRoot();
        elementPath_ = GetUIElementPath(element);
        parentPath_ = GetUIElementPath(element->GetParent());
        XMLElement rootElem = elementData_.CreateRoot("element");
        element->SaveXML(rootElem);
        rootElem.SetUInt("index", element->GetParent()->FindChild(element));
        styleFile_ = element->GetDefaultStyle();
        elementOldStyle_ = element->GetAppliedStyle();
        elementNewStyle_ = newStyle;
    }

    void ApplyStyle(const ea::string& style)
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        UIElement* element = GetUIElementByPath(root_, elementPath_);
        if (parent != nullptr && element != nullptr)
        {
            // Apply the style in the XML data
            elementData_.GetRoot().SetAttribute("style", style);
            parent->RemoveChild(element);
            parent->LoadChildXML(elementData_.GetRoot(), styleFile_);
        }
    }

    void Undo() override
    {
        ApplyStyle(elementOldStyle_);
    }

    void Redo() override
    {
        ApplyStyle(elementNewStyle_);
    }
};

class URHO3D_TOOLBOX_API EditUIStyleAction : public EditAction
{
    XMLFile oldStyle_;
    XMLFile newStyle_;
    UIElementPath elementId_;
    WeakPtr<UIElement> root_;
    Variant oldValue_;
    Variant newValue_;
    ea::string attributeName_;

public:
    EditUIStyleAction(UIElement* element, XMLElement& styleElement, const Variant& newValue)
        : oldStyle_(element->GetContext())
        , newStyle_(element->GetContext())
    {
        attributeName_ = styleElement.GetAttribute("name");
        oldValue_ = element->GetInstanceDefault(attributeName_);
        newValue_ = newValue;

        root_ = element->GetRoot();
        elementId_ = GetUIElementPath(element);
        oldStyle_.CreateRoot("style").AppendChild(element->GetDefaultStyle()->GetRoot(), true);
        if (newValue.IsEmpty())
            styleElement.Remove();
        else
            styleElement.SetVariantValue(newValue);
        newStyle_.CreateRoot("style").AppendChild(element->GetDefaultStyle()->GetRoot(), true);
    }

    void Undo() override
    {
        UIElement* element = GetUIElementByPath(root_, elementId_);
        element->SetInstanceDefault(attributeName_, oldValue_);
        XMLElement root = element->GetDefaultStyle()->GetRoot();
        root.RemoveChildren();
        for (auto child = oldStyle_.GetRoot().GetChild(); !child.IsNull(); child = child.GetNext())
            root.AppendChild(child, true);
    }

    void Redo() override
    {
        UIElement* element = GetUIElementByPath(root_, elementId_);
        element->SetInstanceDefault(attributeName_, newValue_);
        XMLElement root = element->GetDefaultStyle()->GetRoot();
        root.RemoveChildren();
        for (auto child = newStyle_.GetRoot().GetChild(); !child.IsNull(); child = child.GetNext())
            root.AppendChild(child, true);
    }
};

using StateCollection = ea::vector<SharedPtr<EditAction>>;

class URHO3D_TOOLBOX_API Manager : public Object
{
URHO3D_OBJECT(Manager, Object);
public:
    /// Construct.
    explicit Manager(Context* ctx);
    /// Go back in the state history.
    void Undo();
    /// Go forward in the state history.
    void Redo();
    /// Clear all tracked state.
    void Clear();

    /// Track changes performed by this scene.
    void Connect(Scene* scene);
    /// Track changes performed by this attribute inspector.
    void Connect(AttributeInspector* inspector);
    /// Track changes performed to UI hierarchy of this root element.
    void Connect(UIElement* root);
    /// Track changes performed by this gizmo.
    void Connect(Gizmo* gizmo);

    template<typename T, typename... Args>
    void Track(Args... args)
    {
        if (trackingEnabled_)
            currentFrameStates_.push_back(SharedPtr<T>(new T(args...)));
    };

    /// Enables or disables tracking changes.
    void SetTrackingEnabled(bool enabled) { trackingEnabled_ = enabled; }
    /// Return true if manager is tracking undoable changes.
    bool IsTrackingEnabled() const { return trackingEnabled_; }
    ///
    int32_t Index() const { return index_; }

protected:
    /// State stack
    ea::vector<StateCollection> stack_;
    /// Current state index.
    int32_t index_ = 0;
    /// Flag indicating that state tracking is suspended. For example when undo manager is restoring states.
    bool trackingEnabled_ = true;
    /// All actions performed on current frame. They will be applied together.
    StateCollection currentFrameStates_{};
};

class URHO3D_TOOLBOX_API SetTrackingScoped
{
public:
    /// Set undo manager tracking in this scope. Restore to the old value on scope exit.
    explicit SetTrackingScoped(Manager& manager, bool tracking)
        : manager_(manager)
        , tracking_(manager.IsTrackingEnabled())
    {
        manager.SetTrackingEnabled(tracking);
    }

    ~SetTrackingScoped()
    {
        manager_.SetTrackingEnabled(tracking_);
    }

protected:
    /// Undo manager which is being operated upon.
    Manager& manager_;
    /// Old tracking value.
    bool tracking_;
};

}

}
