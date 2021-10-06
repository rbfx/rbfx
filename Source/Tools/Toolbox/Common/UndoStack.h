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
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/UI.h>
#if URHO3D_SYSTEMUI
#include <Urho3D/SystemUI/SystemUI.h>
#endif

namespace Urho3D
{

class AttributeInspector;
class Gizmo;
class Scene;

/// A base class for undo actions.
class URHO3D_TOOLBOX_API UndoAction : public RefCounted
{
public:
    /// Go back in the state history. Returns false when undo action target is expired and nothing was done.
    virtual bool Undo(Context* context) = 0;
    /// Go forward in the state history. Returns false when undo action target is expired and nothing was done.
    virtual bool Redo(Context* context) = 0;
    /// Called when Undo() or Redo() execute successfully and return true.
    virtual void OnModified(Context* context) { }

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
    using Setter = ea::function<bool(Context* context, const ValueCopy&)>;
    /// Callback type.
    using Modified = ea::function<void(Context* context)>;

    /// Construct.
    UndoCustomAction(ValueRef oldValue, ValueRef newValue, Setter onUndo, Setter onRedo, Modified onModified={})
        : initial_(oldValue)
        , current_(newValue)
        , onUndo_(std::move(onUndo))
        , onRedo_(std::move(onRedo))
        , onModified_(std::move(onModified))
    {
    }
    /// Construct.
    UndoCustomAction(ValueRef oldValue, ValueRef newValue, Setter onUndo, Modified onModified={})
        : UndoCustomAction(oldValue, newValue, onUndo, onUndo, std::move(onModified))
    {
    }
    /// Construct.
    UndoCustomAction(ValueRef value, Setter onUndo, Setter onRedo, Modified onModified={})
        : UndoCustomAction(value, value, onUndo, onRedo, std::move(onModified))
    {
    }
    /// Construct.
    UndoCustomAction(ValueRef value, Setter onUndo, Modified onModified={})
        : UndoCustomAction(value, value, onUndo, {}, std::move(onModified))
    {
    }

    /// Go back in the state history.
    bool Undo(Context* context) override { return onUndo_(context, initial_); }
    /// Go forward in the state history.
    bool Redo(Context* context) override
    {
        if ((bool)onRedo_)
            return onRedo_(context, current_);
        else
            return onUndo_(context, current_);         // Undo and redo code may be same for simple cases.
    }
    /// Execute onModified callback.
    void OnModified(Context* context) override { if (onModified_) onModified_(context); }

    /// Initial value.
    ValueCopy initial_;
    /// Latest value.
    ValueCurrent current_;
    /// Flag indicating this action was explicitly modified by the user.
    bool modified_ = false;
    /// Callback that commits old value.
    Setter onUndo_;
    /// Callback that commits new value.
    Setter onRedo_;
    /// Callback that commits new value.
    Modified onModified_;
};

class URHO3D_TOOLBOX_API UndoCreateNode : public UndoAction
{
    unsigned parentID_;
    VectorBuffer nodeData_;
    WeakPtr<Scene> scene_;

public:
    explicit UndoCreateNode(Node* node)
        : scene_(node->GetScene())
    {
        parentID_ = node->GetParent()->GetID();
        node->Save(nodeData_);
    }

    bool Undo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        nodeData_.Seek(0);
        auto nodeID = nodeData_.ReadUInt();
        Node* parent = scene_->GetNode(parentID_);
        Node* node = scene_->GetNode(nodeID);
        if (parent != nullptr && node != nullptr)
            parent->RemoveChild(node);

        return true;
    }

    bool Redo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        Node* parent = scene_->GetNode(parentID_);
        if (parent != nullptr)
        {
            nodeData_.Seek(0);
            auto nodeID = nodeData_.ReadUInt();
            nodeData_.Seek(0);

            Node* node = parent->CreateChild(EMPTY_STRING, nodeID < FIRST_LOCAL_ID ? REPLICATED : LOCAL, nodeID);
            node->Load(nodeData_);
            // FocusNode(node);
        }

        return true;
    }
};

class URHO3D_TOOLBOX_API UndoDeleteNode : public UndoAction
{
    unsigned parentID_;
    unsigned parentIndex_;
    VectorBuffer nodeData_;
    WeakPtr<Scene> scene_;

public:
    explicit UndoDeleteNode(Node* node)
        : scene_(node->GetScene())
    {
        parentID_ = node->GetParent()->GetID();
        parentIndex_ = node->GetParent()->GetChildren().index_of(SharedPtr<Node>(node));
        node->Save(nodeData_);
    }

    bool Undo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        Node* parent = scene_->GetNode(parentID_);
        if (parent != nullptr)
        {
            nodeData_.Seek(0);
            auto nodeID = nodeData_.ReadUInt();
            SharedPtr<Node> node(new Node(parent->GetContext()));
            node->SetID(nodeID);
            parent->AddChild(node, parentIndex_);
            nodeData_.Seek(0);
            node->Load(nodeData_);
        }

        return true;
    }

    bool Redo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        nodeData_.Seek(0);
        auto nodeID = nodeData_.ReadUInt();

        Node* parent = scene_->GetNode(parentID_);
        Node* node = scene_->GetNode(nodeID);
        if (parent != nullptr && node != nullptr)
            parent->RemoveChild(node);

        return true;
    }
};

class URHO3D_TOOLBOX_API UndoReparentNode : public UndoAction
{
    unsigned nodeID_;
    unsigned oldParentID_;
    unsigned newParentID_;
    ea::vector<unsigned> nodeList_; // 2 uints get inserted per node (node, node->GetParent())
    bool multiple_;
    WeakPtr<Scene> scene_;

public:
    UndoReparentNode(Node* node, Node* newParent)
        : scene_(node->GetScene())
    {
        multiple_ = false;
        nodeID_ = node->GetID();
        oldParentID_ = node->GetParent()->GetID();
        newParentID_ = newParent->GetID();
    }

    UndoReparentNode(const ea::vector<Node*>& nodes, Node* newParent)
        : scene_(newParent->GetScene())
    {
        multiple_ = true;
        newParentID_ = newParent->GetID();
        for(Node* node : nodes)
        {
            nodeList_.push_back(node->GetID());
            nodeList_.push_back(node->GetParent()->GetID());
        }
    }

    bool Undo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        if (multiple_)
        {
            for (unsigned i = 0; i < nodeList_.size(); i+=2)
            {
                unsigned nodeID = nodeList_[i];
                unsigned oldParentID = nodeList_[i + 1];
                Node* parent = scene_->GetNode(oldParentID);
                Node* node = scene_->GetNode(nodeID);
                if (parent != nullptr && node != nullptr)
                    node->SetParent(parent);
            }
        }
        else
        {
            Node* parent = scene_->GetNode(oldParentID_);
            Node* node = scene_->GetNode(nodeID_);
            if (parent != nullptr && node != nullptr)
                node->SetParent(parent);
        }
        return true;
    }

    bool Redo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        if (multiple_)
        {
            Node* parent = scene_->GetNode(newParentID_);
            if (parent == nullptr)
                return false;

            for (unsigned i = 0; i < nodeList_.size(); i += 2)
            {
                unsigned nodeID = nodeList_[i];
                Node* node = scene_->GetNode(nodeID);
                if (node != nullptr)
                    node->SetParent(parent);
            }
        }
        else
        {
            Node* parent = scene_->GetNode(newParentID_);
            Node* node = scene_->GetNode(nodeID_);
            if (parent != nullptr && node != nullptr)
                node->SetParent(parent);
        }
        return true;
    }
};

class URHO3D_TOOLBOX_API UndoCreateComponent : public UndoAction
{
    unsigned nodeID_;
    VectorBuffer componentData_;
    WeakPtr<Scene> scene_;

public:
    explicit UndoCreateComponent(Component* component)
        : scene_(component->GetScene())
    {
        nodeID_ = component->GetNode()->GetID();
        component->Save(componentData_);
    }

    bool Undo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        componentData_.Seek(sizeof(StringHash));
        auto componentID = componentData_.ReadUInt();
        Node* node = scene_->GetNode(nodeID_);
        Component* component = scene_->GetComponent(componentID);
        if (node != nullptr && component != nullptr)
            node->RemoveComponent(component);
        return true;
    }

    bool Redo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        Node* node = scene_->GetNode(nodeID_);
        if (node != nullptr)
        {
            componentData_.Seek(0);
            auto componentType = componentData_.ReadStringHash();
            auto componentID = componentData_.ReadUInt();

            Component* component = node->CreateComponent(componentType,
                componentID < FIRST_LOCAL_ID ? REPLICATED : LOCAL, componentID);
            component->Load(componentData_);
            component->ApplyAttributes();
            // FocusComponent(component);
        }
        return true;
    }

};

class URHO3D_TOOLBOX_API UndoDeleteComponent : public UndoAction
{
    unsigned nodeID_;
    VectorBuffer componentData_;
    WeakPtr<Scene> scene_;

public:
    explicit UndoDeleteComponent(Component* component)
        : scene_(component->GetScene())
    {
        nodeID_ = component->GetNode()->GetID();
        component->Save(componentData_);
    }

    bool Undo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        Node* node = scene_->GetNode(nodeID_);
        if (node != nullptr)
        {
            componentData_.Seek(0);
            auto componentType = componentData_.ReadStringHash();
            unsigned componentID = componentData_.ReadUInt();
            Component* component = node->CreateComponent(componentType, componentID < FIRST_LOCAL_ID ? REPLICATED : LOCAL, componentID);

            if (component->Load(componentData_))
            {
                component->ApplyAttributes();
                // FocusComponent(component);
            }
        }
        return true;
    }

    bool Redo(Context* context) override
    {
        if (scene_.Expired())
            return false;

        componentData_.Seek(sizeof(StringHash));
        unsigned componentID = componentData_.ReadUInt();

        Node* node = scene_->GetNode(nodeID_);
        Component* component = scene_->GetComponent(componentID);
        if (node != nullptr && component != nullptr)
        {
            node->RemoveComponent(component);
        }
        return true;
    }
};

class URHO3D_TOOLBOX_API UndoEditAttribute : public UndoAction
{
    unsigned targetID;
    ea::string attrName_;
    Variant undoValue_;
    Variant redoValue_;
    StringHash targetType_;
    WeakPtr<Scene> editorScene_;
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
    }

    Serializable* GetTarget()
    {
        if (targetType_ == Node::GetTypeStatic())
            return editorScene_->GetNode(targetID);
        else if (targetType_ == Component::GetTypeStatic())
            return editorScene_->GetComponent(targetID);

        return target_.Get();
    }

    bool IsExpired()
    {
        if (targetType_ == Node::GetTypeStatic() || targetType_ == Component::GetTypeStatic())
            return editorScene_.Expired();

        return target_.Expired();
    }

    bool Undo(Context* context) override
    {
        if (IsExpired())
            return false;

        Serializable* target = GetTarget();
        if (target != nullptr)
        {
            target->SetAttribute(attrName_, undoValue_);
            target->ApplyAttributes();
        }
        return true;
    }

    bool Redo(Context* context) override
    {
        if (IsExpired())
            return false;

        Serializable* target = GetTarget();
        if (target != nullptr)
        {
            target->SetAttribute(attrName_, redoValue_);
            target->ApplyAttributes();
        }
        return true;
    }
};

template<typename Resource, typename Value>
class UndoResourceSetter : public UndoAction
{
    ea::string name_;
    Value oldValue_;
    Value newValue_;
    void (Resource::*setter_)(Value value);

public:
    UndoResourceSetter(ea::string_view name, Value oldValue, Value newValue, void (Resource::*setter)(Value))
        : name_(name)
        , oldValue_(oldValue)
        , newValue_(newValue)
        , setter_(setter)
    {
    }

    ~UndoResourceSetter() = default;

    bool Undo(Context* context) override
    {
        auto* cache = context->GetSubsystem<ResourceCache>();
        if (auto* resource = cache->template GetResource<Resource>(name_))
        {
            (resource->*setter_)(oldValue_);
            return true;
        }
        return false;
    }

    bool Redo(Context* context) override
    {
        auto* cache = context->GetSubsystem<ResourceCache>();
        if (auto* resource = cache->template GetResource<Resource>(name_))
        {
            (resource->*setter_)(newValue_);
            return true;
        }
        return false;
    }
    /// Auto-save resource.
    void OnModified(Context* context) override
    {
        auto* cache = context->GetSubsystem<ResourceCache>();
        if (auto* resource = cache->template GetResource<Resource>(name_))
        {
            cache->IgnoreResourceReload(name_);
            resource->SaveFile(cache->GetResourceFileName(name_));
        }
    }
};

class URHO3D_TOOLBOX_API UndoNodeReorder : public UndoAction
{
    WeakPtr<Scene> scene_;
    unsigned nodeId_ = M_MAX_UNSIGNED;
    unsigned oldPos_ = M_MAX_UNSIGNED;
    unsigned newPos_ = M_MAX_UNSIGNED;
public:
    UndoNodeReorder(Node* node, unsigned oldPos)
    {
        assert(node != nullptr);
        assert(node->GetParent() != nullptr);
        scene_ = node->GetScene();
        nodeId_ = node->GetID();
        oldPos_ = oldPos;
        newPos_ = node->GetParent()->GetChildIndex(node);
    }

    bool Undo(Context* context) override
    {
        Node* node = scene_ ? scene_->GetNode(nodeId_) : nullptr;
        Node* parent = node ? node->GetParent() : nullptr;
        if (node == nullptr || parent == nullptr)
            return false;
        parent->ReorderChild(node, oldPos_);
        return true;
    }

    bool Redo(Context* context) override
    {
        Node* node = scene_ ? scene_->GetNode(nodeId_) : nullptr;
        Node* parent = node ? node->GetParent() : nullptr;
        if (node == nullptr || parent == nullptr)
            return false;
        parent->ReorderChild(node, newPos_);
        return true;
    }
};

class URHO3D_TOOLBOX_API UndoComponentReorder : public UndoAction
{
    WeakPtr<Scene> scene_;
    unsigned nodeId_ = M_MAX_UNSIGNED;
    unsigned componentId_ = M_MAX_UNSIGNED;
    unsigned oldPos_ = M_MAX_UNSIGNED;
    unsigned newPos_ = M_MAX_UNSIGNED;
public:
    UndoComponentReorder(Component* component, unsigned oldPos)
    {
        assert(component != nullptr);
        assert(component->GetNode() != nullptr);
        scene_ = component->GetScene();
        nodeId_ = component->GetNode()->GetID();
        componentId_ = component->GetID();
        oldPos_ = oldPos;
        newPos_ = component->GetNode()->GetComponentIndex(component);
    }

    bool Undo(Context* context) override
    {
        Component* component = scene_ ? scene_->GetComponent(componentId_) : nullptr;
        Node* parent = component ? component->GetNode() : nullptr;
        if (component == nullptr || parent == nullptr)
            return false;
        parent->ReorderComponent(component, oldPos_);
        return true;
    }

    bool Redo(Context* context) override
    {
        Component* component = scene_ ? scene_->GetComponent(componentId_) : nullptr;
        Node* parent = component ? component->GetNode() : nullptr;
        if (component == nullptr || parent == nullptr)
            return false;
        parent->ReorderComponent(component, newPos_);
        return true;
    }
};

/// Event sent at the end of frame when document has created undoable action that modifies said document. User should
/// handle this event by calling `undo_->Add<UndoModifiedState>(this, true)` if current document state is "not modified".
URHO3D_EVENT(E_DOCUMENTMODIFIEDREQUEST, DocumentModifiedRequest)
{
}

/// Event sent when document "modified" state is changed by executing undo/redo actions. User should handle this event
/// by setting internal "modified" flat to value specified by P_MODIFIED.
URHO3D_EVENT(E_DOCUMENTMODIFIED, DocumentModified)
{
    URHO3D_PARAM(P_MODIFIED, Modified);         // bool
}

class URHO3D_TOOLBOX_API UndoModifiedState : public UndoAction
{
    /// Object that tracks it's modified state.
    WeakPtr<Object> object_;
    /// Flag indicating whether object was modified or saved.
    bool isModified_ = false;

public:
    /// Construct.
    UndoModifiedState(Object* object, bool isModified)
        : object_(object)
        , isModified_(isModified)
    {
    }

    bool Undo(Context* context) override
    {
        if (object_.Expired())
            return false;

        using namespace DocumentModified;
        object_->SendEvent(E_DOCUMENTMODIFIED, P_MODIFIED, !isModified_);
        return true;
    }

    bool Redo(Context* context) override
    {
        if (object_.Expired())
            return false;

        using namespace DocumentModified;
        object_->SendEvent(E_DOCUMENTMODIFIED, P_MODIFIED, isModified_);
        return true;
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
    T* Add(Args&&... args) { return static_cast<T*>(Add(new T(std::forward<Args>(args)...))); }
    /// Record action into undo stack. Should be used for cases where continuous change does not span multiple frames.
    /// For example with text input widgets that are committed with [Enter] key, combo boxes, checkboxes and similar.
    UndoAction* Add(UndoAction* action)
    {
        if (trackingEnabled_)
        {
            currentFrameActions_.push_back(SharedPtr(action));
            return action;
        }
        return nullptr;
    }

    /// Track changes performed by this scene. If \param modified is specified then any modification will cause
    /// \param modified send E_DOCUMENTMODIFIEDREQUEST event.
    void Connect(Scene* scene, Object* modified = nullptr);
    /// Track changes performed by this object. It usually is instance of AttributeInspector or Serializable.
    void Connect(Object* inspector, Object* modified = nullptr);
    /// Track changes performed by this gizmo.
    void Connect(Gizmo* gizmo, Object* modified = nullptr);
    /// Set an object which enters "modified" state as a consequence of creating undoable actions on this frame.
    void SetModifiedObject(Object* modifed);

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
    /// Object which was modified on current frame.
    WeakPtr<Object> modifiedThisFrame_{};

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
                if (action_->modified_)
                {
                    // User modifications are promoted to undo stack.
                    SharedPtr<UndoAction> actionPtr(stack_->workingValueCache_.template Detach<T>(hash_));
                    stack_->currentFrameActions_.push_back(actionPtr);
                }
                else
                {
                    // External modifications are ignored.
                    action_->initial_ = action_->current_;
                }
            }
        }
    }
    /// Allow use of object in if ().
    operator bool() { return true; }
    ///
    void SetModified(bool modified) { if (action_) action_->modified_ |= modified; }

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
