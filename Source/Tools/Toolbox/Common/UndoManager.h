//
// Copyright (c) 2008-2017 the Urho3D project.
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


#include <Urho3D/Core/Object.h>


namespace Urho3D
{

class AttributeInspector;
class Gizmo;
class Scene;

namespace Undo
{

/// Abstract class for implementing various trackable states.
class State : public RefCounted
{
public:
    /// Apply state saved in this object.
    virtual void Apply() = 0;
    /// Return true if state of this object matches state of specified object.
    virtual bool Equals(State* other) const = 0;
    /// Return string representation of current state.
    virtual String ToString() const { return "State"; }
};

/// Tracks attribute values of Serializable item.
class AttributeState : public State
{
public:
    /// Construct state consisting of single attribute.
    AttributeState(Serializable* item, const String& name, const Variant& value);
    /// Apply attributes if they are different and return true if operation was carried out.
    void Apply() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(State* other) const override;
    /// Return string representation of current state.
    String ToString() const override;

    /// Object that was modified.
    SharedPtr<Serializable> item_;
    /// Changed attribute name.
    String name_;
    /// Changed attribute value.
    Variant value_;
};

/// Tracks UIElement parent state. Used for tracking adding and removing UIElements.
class ElementParentState : public State
{
public:
    /// Construct item from the state and parent
    ElementParentState(UIElement* item, UIElement* parent);
    /// Set parent of the item if it is different and return true if operation was carried out.
    void Apply() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(State* other) const override;
    /// Return string representation of current state.
    String ToString() const override;

    /// UIElement whose state is saved.
    SharedPtr<UIElement> item_;
    /// Parent of item_ at the time when state was saved.
    SharedPtr<UIElement> parent_;
    /// Position at which item was inserted to parent's children list.
    unsigned index_ = M_MAX_UNSIGNED;
};

/// Tracks Node parent state. Used for tracking adding and removing scene nodes.
class NodeParentState : public State
{
public:
    /// Construct item from the state and parent
    NodeParentState(Node* item, Node* parent);
    /// Set parent of the item if it is different and return true if operation was carried out.
    void Apply() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(State* other) const override;
    /// Return string representation of current state.
    String ToString() const override;

    /// UIElement whose state is saved.
    SharedPtr<Node> item_;
    /// Parent of item_ at the time when state was saved.
    SharedPtr<Node> parent_;
    /// Position at which item was inserted to parent's children list.
    unsigned index_ = M_MAX_UNSIGNED;
};

/// Tracks Component parent state. Used for tracking adding and removing components.
class ComponentParentState : public State
{
public:
    /// Construct item from the state and parent
    ComponentParentState(Component* item, Node* parent);
    /// Set parent of the item if it is different and return true if operation was carried out.
    void Apply() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(State* other) const override;
    /// Return string representation of current state.
    String ToString() const override;

    /// UIElement whose state is saved.
    SharedPtr<Component> item_;
    /// Parent of item_ at the time when state was saved.
    SharedPtr<Node> parent_;
    /// ID of the component.
    unsigned id_;
};

/// Tracks xml parent state. Used for tracking adding and removing xml elements to and from data files.
class XMLVariantState : public State
{
public:
    /// Construct item from the state and parent
    XMLVariantState(const XMLElement& item, const Variant& value);
    /// Set parent of the item if it is different and return true if operation was carried out.
    void Apply() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(State* other) const override;
    /// Return string representation of current state.
    String ToString() const override;

    /// XMLElement whose state is saved.
    XMLElement item_;
    /// Value of XMLElement item.
    Variant value_;
};

/// Tracks item parent state. Used for tracking adding and removing UIElements.
class XMLParentState : public State
{
public:
    /// Construct item from the state and parent.
    explicit XMLParentState(const XMLElement& item, const XMLElement& parent=XMLElement());
    /// Set parent of the item if it is different and return true if operation was carried out.
    void Apply() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(State* other) const override;
    /// Return string representation of current state.
    String ToString() const override;

    /// XMLElement whose state is saved.
    XMLElement item_;
    /// Parent of XMLElement item.
    XMLElement parent_;
};

/// A collection of states that are applied together.
class StateCollection
{
public:
    /// Set parent of the item if it is different and return true if operation was carried out.
    void Apply();
    /// Return true if state of this object matches state of specified object.
    bool Contains(State* other) const;
    /// Append state to the collection if such state does not already exist.
    bool PushUnique(const SharedPtr<State>& state);

    /// List of states that should be applied together.
    Vector<SharedPtr<State>> states_;
};

class Manager : public Object
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

    /// Track item state consisting of single attribute. Old value must be specified manually for tracking. Use when
    /// item state has changed already, but old value is known. Use this only when it is not possible to make changes
    /// tracked automatically.
    void TrackState(Serializable* item, const String& name, const Variant& value, const Variant& oldValue);

    /// Tracked XMLElement creation.
    XMLElement XMLCreate(XMLElement& parent, const String& name);
    /// Tracked XMLElement removal.
    void XMLRemove(XMLElement& element);
    /// Track XMLElement state.
    void XMLSetVariantValue(XMLElement& element, const Variant& value);

protected:
    /// Apply state going to specified direction in the state stack.
    void ApplyStateFromStack(bool forward);
    /// Track object state as old.
    template<typename T, typename... Args>
    void TrackBefore(Args...);
    /// Track object state as new.
    template<typename T, typename... Args>
    void TrackAfter(Args...);

    /// State stack
    Vector<StateCollection> stack_;
    /// Current state index, -1 when stack is empty.
    int32_t index_ = -1;
    /// Flag indicating that state tracking is suspended. For example when undo manager is restoring states.
    bool trackingSuspended_ = false;
    /// List of old object states.
    Vector<SharedPtr<State>> previous_;
    /// List of new object states.
    Vector<SharedPtr<State>> next_;
};

}

}
