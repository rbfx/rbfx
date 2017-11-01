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


#include <Urho3D/Container/Vector.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/UI/UI.h>


namespace Urho3D
{

/// Abstract class for implementing various trackable states.
class UndoableState : public RefCounted
{
public:
    /// Apply state saved in this object.
    virtual bool Apply() = 0;
    /// Return true if current state matches state saved in this object.
    virtual bool IsCurrent() = 0;
    /// Return true if state of this object matches state of specified object.
    virtual bool Equals(UndoableState* other) = 0;
    /// Return string representation of current state.
    virtual String ToString() const { return "UndoableState"; }
};

/// Tracks attribute values of Serializable item.
class UndoableAttributesState : public UndoableState
{
public:
    /// Construct state consisting of single attribute.
    UndoableAttributesState(Serializable* item, const String& name, const Variant& value);
    /// Construct state consisting of multiple attributes.
    UndoableAttributesState(Serializable* item, const HashMap <String, Variant>& values);
    /// Apply attributes if they are different and return true if operation was carried out.
    bool Apply() override;
    /// Return true if attributes saved in this state match attributes set to a serializable.
    bool IsCurrent() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override;
    /// Return string representation of current state.
    String ToString() const override;

    /// Object that was modified.
    SharedPtr <Serializable> item_;
    /// Changed attributes.
    HashMap <String, Variant> attributes_;
};

/// Tracks item parent state. Used for tracking adding and removing UIElements.
class UndoableItemParentState : public UndoableState
{
public:
    /// Construct item from the state and parent
    UndoableItemParentState(UIElement* item, UIElement* parent);
    /// Set parent of the item if it is different and return true if operation was carried out.
    bool Apply() override;
    /// Return true if item parent matches and item is at recorded index.
    bool IsCurrent() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override;
    /// Return string representation of current state.
    String ToString() const override;

    /// UIElement whose state is saved.
    SharedPtr <UIElement> item_;
    /// Parent of item_ at the time when state was saved.
    SharedPtr <UIElement> parent_;
    /// Position at which item was inserted to parent's children list.
    unsigned index_ = M_MAX_UNSIGNED;
};

/// Tracks xml parent state. Used for tracking adding and removing xml elements to and from data files.
class UndoableXMLVariantState : public UndoableState
{
public:
    /// Construct item from the state and parent
    UndoableXMLVariantState(const XMLElement& item, const Variant& value);
    /// Set parent of the item if it is different and return true if operation was carried out.
    bool Apply() override;
    /// Return true if item parent matches and item is at recorded index.
    bool IsCurrent() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override;
    /// Return string representation of current state.
    String ToString() const override;

    /// XMLElement whose state is saved.
    XMLElement item_;
    /// Value of XMLElement item.
    Variant value_;
};

/// Tracks item parent state. Used for tracking adding and removing UIElements.
class UndoableXMLParentState : public UndoableState
{
public:
    /// Construct item from the state and parent.
    explicit UndoableXMLParentState(const XMLElement& item, const XMLElement& parent=XMLElement());
    /// Set parent of the item if it is different and return true if operation was carried out.
    bool Apply() override;
    /// Return true if item parent matches and item is at recorded index.
    bool IsCurrent() override;
    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override;
    /// Return string representation of current state.
    String ToString() const override;

    /// XMLElement whose state is saved.
    XMLElement item_;
    /// Parent of XMLElement item.
    XMLElement parent_;
};

class UndoManager : public Object
{
    URHO3D_OBJECT(UndoManager, Object);
public:
    /// Construct.
    explicit UndoManager(Context* ctx);
    /// Go back in the state history.
    void Undo();
    /// Go forward in the state history.
    void Redo();
    /// Track item state consisting of single attribute.
    void TrackState(Serializable* item, const String& name, const Variant& value);
    /// Track item state consisting of multiple attributes.
    void TrackState(Serializable* item, const HashMap <String, Variant>& values);
    /// Track UIElement creation.
    void TrackCreation(UIElement* item);
    /// Track UIElement removal.
    void TrackRemoval(UIElement* item);
    /// Track XMLElement creation.
    void TrackCreation(const XMLElement& element);
    /// Track XMLElement creation.
    void TrackRemoval(const XMLElement& element);
    /// Track XMLElement state.
    void TrackState(const XMLElement& element, const Variant& value);

protected:
    /// Track add undoable state to the state stack.
    void Track(UndoableState* state);

    /// State stack
    Vector<SharedPtr<UndoableState> > stack_;
    /// Current state index, -1 when stack is empty.
    int32_t index_ = -1;
};

}
