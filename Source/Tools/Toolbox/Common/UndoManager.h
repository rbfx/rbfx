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

#include "ToolboxAPI.h"
#include <Urho3D/Container/ValueCache.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#if URHO3D_SYSTEMUI
#include <Urho3D/SystemUI/SystemUI.h>
#endif

namespace Urho3D
{

class AttributeInspector;
class Gizmo;
class Scene;

/// Notify undo managers that state is about to be undone.
URHO3D_EVENT(E_UNDO, UndoEvent)
{
    URHO3D_PARAM(P_FRAME, Frame);                     // unsigned
    URHO3D_PARAM(P_MANAGER, Manager);                 // UndoStack pointer
}

/// Notify undo managers that state is about to be redone.
URHO3D_EVENT(E_REDO, RedoEvent)
{
    URHO3D_PARAM(P_FRAME, Frame);                     // unsigned
    URHO3D_PARAM(P_MANAGER, Manager);                 // UndoStack pointer
}

/// A base class for undo actions.
class URHO3D_TOOLBOX_API UndoAction : public RefCounted
{
public:
    /// Go back in the state history.
    virtual void Undo(Context* context) = 0;
    /// Go forward in the state history.
    virtual void Redo(Context* context) = 0;

    /// Frame when action was recorded.
    unsigned long long frame_ = 0;
};

/// A custom undo action that manages application state using lambdas. Used in cases where tracked undo action is very
/// specific and is not expected to be tracked again in another place in same program.
template<typename Value>
class UndoCustomAction : public UndoAction
{
public:
    /// Type of tracked value.
    using ValueType = Value;
    /// Copy value type.
    using ValueCopy = ea::remove_cvref_t<Value>;
    /// Reference value type.
    using ValueRef = std::conditional_t<!std::is_reference_v<Value>, Value&, Value>;
    /// Reference or a copy, depending on whether Value is const.
    using ValueCurrent = std::conditional_t<!std::is_const_v<Value> && std::is_reference_v<Value>, ValueCopy&, ValueCopy>;
    /// Callback type.
    using Setter = ea::function<void(Context* context, const ValueCopy&)>;

    /// Construct.
    UndoCustomAction(ValueRef value, Setter onUndo, Setter onRedo={})
        : initial_(value)
          , current_(value)
          , onUndo_(std::move(onUndo))
          , onRedo_(std::move(onRedo))
    {
    }

    /// Go back in the state history.
    void Undo(Context* context) override { onUndo_(context, initial_); }
    /// Go forward in the state history.
    void Redo(Context* context) override
    {
        if ((bool)onRedo_)
            onRedo_(context, current_);
        else
            onUndo_(context, current_);         // Undo and redo code may be same for simple cases.
    }

    /// Initial value.
    ValueCopy initial_;
    /// Latest value.
    ValueCurrent current_;
    /// Callback that commits old value.
    Setter onUndo_;
    /// Callback that commits new value.
    Setter onRedo_;
};

class URHO3D_TOOLBOX_API UndoCreateNode : public UndoAction
{
    unsigned parentID;
    VectorBuffer nodeData;
    WeakPtr<Scene> editorScene;

public:
    explicit UndoCreateNode(Node* node)
        : editorScene(node->GetScene())
    {
        parentID = node->GetParent()->GetID();
        node->Save(nodeData);
    }

    void Undo(Context* context) override
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

    void Redo(Context* context) override
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

class URHO3D_TOOLBOX_API UndoDeleteNode : public UndoAction
{
    unsigned parentID;
    unsigned parentIndex;
    VectorBuffer nodeData;
    WeakPtr<Scene> editorScene;

public:
    explicit UndoDeleteNode(Node* node)
        : editorScene(node->GetScene())
    {
        parentID = node->GetParent()->GetID();
        parentIndex = node->GetParent()->GetChildren().index_of(SharedPtr<Node>(node));
        node->Save(nodeData);
    }

    void Undo(Context* context) override
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

    void Redo(Context* context) override
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

class URHO3D_TOOLBOX_API UndoReparentNode : public UndoAction
{
    unsigned nodeID;
    unsigned oldParentID;
    unsigned newParentID;
    ea::vector<unsigned> nodeList; // 2 uints get inserted per node (node, node->GetParent())
    bool multiple;
    WeakPtr<Scene> editorScene;

public:
    UndoReparentNode(Node* node, Node* newParent)
        : editorScene(node->GetScene())
    {
        multiple = false;
        nodeID = node->GetID();
        oldParentID = node->GetParent()->GetID();
        newParentID = newParent->GetID();
    }

    UndoReparentNode(const ea::vector<Node*>& nodes, Node* newParent)
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

    void Undo(Context* context) override
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

    void Redo(Context* context) override
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

class URHO3D_TOOLBOX_API UndoCreateComponent : public UndoAction
{
    unsigned nodeID;
    VectorBuffer componentData;
    WeakPtr<Scene> editorScene;

public:
    explicit UndoCreateComponent(Component* component)
        : editorScene(component->GetScene())
    {
        nodeID = component->GetNode()->GetID();
        component->Save(componentData);
    }

    void Undo(Context* context) override
    {
        componentData.Seek(sizeof(StringHash));
        auto componentID = componentData.ReadUInt();
        Node* node = editorScene->GetNode(nodeID);
        Component* component = editorScene->GetComponent(componentID);
        if (node != nullptr && component != nullptr)
            node->RemoveComponent(component);
    }

    void Redo(Context* context) override
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

class URHO3D_TOOLBOX_API UndoDeleteComponent : public UndoAction
{
    unsigned nodeID;
    VectorBuffer componentData;
    WeakPtr<Scene> editorScene;

public:
    UndoDeleteComponent(Component* component)
        : editorScene(component->GetScene())
    {
        nodeID = component->GetNode()->GetID();
        component->Save(componentData);
    }

    void Undo(Context* context) override
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

    void Redo(Context* context) override
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

class URHO3D_TOOLBOX_API UndoEditAttribute : public UndoAction
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
    UndoEditAttribute(Serializable* target, const ea::string& name, const Variant& oldValue, const Variant& newValue)
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

    void Undo(Context* context) override
    {
        Serializable* target = GetTarget();
        if (target != nullptr)
        {
            target->SetAttribute(attrName_, undoValue_);
            target->ApplyAttributes();
        }
    }

    void Redo(Context* context) override
    {
        Serializable* target = GetTarget();
        if (target != nullptr)
        {
            target->SetAttribute(attrName_, redoValue_);
            target->ApplyAttributes();
        }
    }
};

class URHO3D_TOOLBOX_API UndoCreateUIElement : public UndoAction
{
    UIElementPath elementPath_;
    UIElementPath parentPath_;
    XMLFile elementData_;
    SharedPtr<XMLFile> styleFile_;
    WeakPtr<UIElement> root_;

public:
    explicit UndoCreateUIElement(UIElement* element)
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

    void Undo(Context* context) override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        UIElement* element = GetUIElementByPath(root_, elementPath_);
        if (parent != nullptr && element != nullptr)
            parent->RemoveChild(element);
    }

    void Redo(Context* context) override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        if (parent != nullptr)
            parent->LoadChildXML(elementData_.GetRoot(), styleFile_);
    }
};

class URHO3D_TOOLBOX_API UndoDeleteUIElement : public UndoAction
{
    UIElementPath elementPath_;
    UIElementPath parentPath_;
    XMLFile elementData_;
    SharedPtr<XMLFile> styleFile_;
    WeakPtr<UIElement> root_;

public:
    explicit UndoDeleteUIElement(UIElement* element)
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

    void Undo(Context* context) override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        if (parent != nullptr)
            parent->LoadChildXML(elementData_.GetRoot(), styleFile_);
    }

    void Redo(Context* context) override
    {
        UIElement* parent = GetUIElementByPath(root_, parentPath_);
        UIElement* element = GetUIElementByPath(root_, elementPath_);
        if (parent != nullptr && element != nullptr)
            parent->RemoveChild(element);
    }
};

class URHO3D_TOOLBOX_API UndoReparentUIElement : public UndoAction
{
    UIElementPath elementPath_;
    UIElementPath oldParentPath_;
    unsigned oldChildIndex_;
    UIElementPath newParentPath_;
    WeakPtr<UIElement> root;

public:
    UndoReparentUIElement(UIElement* element, UIElement* newParent)
    : root(element->GetRoot())
    {
        elementPath_ = GetUIElementPath(element);
        oldParentPath_ = GetUIElementPath(element->GetParent());
        oldChildIndex_ = element->GetParent()->FindChild(element);
        newParentPath_ = GetUIElementPath(newParent);
    }

    void Undo(Context* context) override
    {
        UIElement* parent = GetUIElementByPath(root, oldParentPath_);
        UIElement* element = GetUIElementByPath(root, elementPath_);
        if (parent != nullptr && element != nullptr)
            element->SetParent(parent, oldChildIndex_);
    }

    void Redo(Context* context) override
    {
        UIElement* parent = GetUIElementByPath(root, newParentPath_);
        UIElement* element = GetUIElementByPath(root, elementPath_);
        if (parent != nullptr && element != nullptr)
            element->SetParent(parent);
    }
};

class URHO3D_TOOLBOX_API UndoApplyUIElementStyle : public UndoAction
{
    UIElementPath elementPath_;
    UIElementPath parentPath_;
    XMLFile elementData_;
    XMLFile* styleFile_;
    ea::string elementOldStyle_;
    ea::string elementNewStyle_;
    WeakPtr<UIElement> root_;

public:
    UndoApplyUIElementStyle(UIElement* element, const ea::string& newStyle)
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

    void Undo(Context* context) override
    {
        ApplyStyle(elementOldStyle_);
    }

    void Redo(Context* context) override
    {
        ApplyStyle(elementNewStyle_);
    }
};

class URHO3D_TOOLBOX_API UndoEditUIStyle : public UndoAction
{
    XMLFile oldStyle_;
    XMLFile newStyle_;
    UIElementPath elementId_;
    WeakPtr<UIElement> root_;
    Variant oldValue_;
    Variant newValue_;
    ea::string attributeName_;

public:
    UndoEditUIStyle(UIElement* element, XMLElement& styleElement, const Variant& newValue)
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

    void Undo(Context* context) override
    {
        UIElement* element = GetUIElementByPath(root_, elementId_);
        element->SetInstanceDefault(attributeName_, oldValue_);
        XMLElement root = element->GetDefaultStyle()->GetRoot();
        root.RemoveChildren();
        for (auto child = oldStyle_.GetRoot().GetChild(); !child.IsNull(); child = child.GetNext())
            root.AppendChild(child, true);
    }

    void Redo(Context* context) override
    {
        UIElement* element = GetUIElementByPath(root_, elementId_);
        element->SetInstanceDefault(attributeName_, newValue_);
        XMLElement root = element->GetDefaultStyle()->GetRoot();
        root.RemoveChildren();
        for (auto child = newStyle_.GetRoot().GetChild(); !child.IsNull(); child = child.GetNext())
            root.AppendChild(child, true);
    }
};

template<typename T> class UndoValueScope;

class URHO3D_TOOLBOX_API UndoStack : public Object
{
    URHO3D_OBJECT(UndoStack, Object);
public:
    /// Construct.
    explicit UndoStack(Context* context);

    /// Go back in the state history.
    void Undo();
    /// Go forward in the state history.
    void Redo();
    /// Clear all tracked state.
    void Clear();
    /// Enables or disables tracking changes.
    void SetTrackingEnabled(bool enabled) { trackingEnabled_ = enabled; }
    /// Return true if manager is tracking undoable changes.
    bool IsTrackingEnabled() const { return trackingEnabled_; }
    /// Return current index in undo stack.
    int Index() const { return index_; }
#if URHO3D_SYSTEMUI
    /// Track a continuous modification and record it to undo stack when value is no longer modified. Should be used
    /// with sliders, draggable widgets and similar. T type must define ValueType type and store initial_ and current_
    /// members that/ can be compared for equality. T will be recorded into undo stack when value is modified and no
    /// widget is active. Note that modifications are applied to the program state each time they happen as undo action
    /// knows how to do that. You do not have to do anything when widget returns true indicating that value was modified.
    /// Usage:
    /// if (auto value = undo.Track<UndoCustomAction<float>>(value, ...))
    ///     ui::DragFloat(..., &value.current_, ...);
    template<typename T, typename... Args>
    UndoValueScope<T> Track(typename T::ValueType current, Args&&... args);
#endif
    /// Record action into undo stack. Should be used for cases where continuous change does not span multiple frames.
    /// For example with text input widgets that are committed with [Enter] key, combo boxes, checkboxes and similar.
    template<typename T, typename... Args>
    void Add(Args&&... args) { Add(new T(std::forward<Args>(args)...)); }
    /// Record action into undo stack. Should be used for cases where continuous change does not span multiple frames.
    /// For example with text input widgets that are committed with [Enter] key, combo boxes, checkboxes and similar.
    void Add(UndoAction* action) { currentFrameActions_.push_back(SharedPtr(action)); }

    /// Track changes performed by this scene.
    void Connect(Scene* scene);
    /// Track changes performed by this object. It usually is instance of AttributeInspector or Serializable.
    void Connect(Object* inspector);
    /// Track changes performed to UI hierarchy of this root element.
    void Connect(UIElement* root);
    /// Track changes performed by this gizmo.
    void Connect(Gizmo* gizmo);

protected:
    using StateCollection = ea::vector<SharedPtr<UndoAction>>;

    /// State stack
    ea::vector<StateCollection> stack_;
    /// Current state index.
    int index_ = 0;
    /// Flag indicating that state tracking is suspended. For example when undo manager is restoring states.
    bool trackingEnabled_ = true;
    /// All actions performed on current frame. They will be applied together.
    StateCollection currentFrameActions_{};
    /// Cache of backup original values.
    ValueCache workingValueCache_{context_};

    template<typename> friend class UndoValueScope;
};

#if URHO3D_SYSTEMUI
template<typename T>
class UndoValueScope
{
public:
    /// Construct.
    UndoValueScope(UndoStack* stack, unsigned hash, T* action)
        : value_(action->current_)
          , stack_(stack)
          , hash_(hash)
          , action_(action)
    {
    }
    /// Destruct.
    ~UndoValueScope()
    {
        if (stack_ == nullptr || action_ == nullptr)
            // Noop. Undo tracking is not enabled.
            return;

        if (action_->initial_ != action_->current_)
        {
            // UI works with a copy value. Fake redo applies that value and user does not have to apply it manually.
            action_->Redo(stack_->GetContext());
            // This value was modified and user is no longer interacting with UI. Detach undo action from cache and
            // promote it to recorded undo actions.
            if (!ui::IsAnyItemActive())
            {
                SharedPtr<UndoAction> actionPtr(stack_->workingValueCache_.template Detach<T>(hash_));
                stack_->currentFrameActions_.push_back(actionPtr);
            }
        }
    }
    /// Allow use of object in if ().
    operator bool() { return true; }

    /// Current value. Should be used by UI.
    typename T::ValueType& value_;

protected:
    /// Undo stack value was created by.
    UndoStack* stack_ = nullptr;
    /// Hash pointing to currect action in undo stack value cache.
    unsigned hash_ = 0;
    /// Undo action that is pending to be recorded into undo stack.
    T* action_ = nullptr;
};
#endif

#if URHO3D_SYSTEMUI
template<typename T, typename... Args>
UndoValueScope<T> UndoStack::Track(typename T::ValueType current, Args&&... args)
{
    if (!trackingEnabled_)
        return UndoValueScope<T>(nullptr, 0, nullptr);
    unsigned hash = ui::GetCurrentWindow()->IDStack.back();
    auto action = workingValueCache_.Get<T>(hash, current, std::forward<Args>(args)...);
    action->current_ = current;
    return UndoValueScope<T>(this, hash, action);
}
#endif

/// Enables or disables undo tracking for the lifetime of the object. Restores original tracking state on destruction.
class URHO3D_TOOLBOX_API UndoTrackGuard
{
public:
    /// Construct.
    explicit UndoTrackGuard(UndoStack& stack, bool track)
        : stack_(stack)
          , tracking_(stack.IsTrackingEnabled())
    {
        stack_.SetTrackingEnabled(track);
    }
    /// Construct.
    explicit UndoTrackGuard(UndoStack* stack, bool track) : UndoTrackGuard(*stack, track) { }
    /// Destruct.
    ~UndoTrackGuard() { stack_.SetTrackingEnabled(tracking_); }

private:
    /// Undo stack that is being guarded.
    UndoStack& stack_;
    /// Initial trackingstate.
    bool tracking_ = false;
};

}
