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
    virtual String ToString() const
    {
        return "UndoableState";
    }
};

/// Tracks attribute values of Serializable item.
class UndoableAttributesState : public UndoableState
{
public:
    /// Object that was modified.
    SharedPtr <Serializable> item_;
    /// Changed attributes.
    HashMap <String, Variant> attributes_;

    /// Construct state consisting of single attribute.
    UndoableAttributesState(Serializable* item, const String& name, const Variant& value) : item_(item)
    {
        attributes_[name] = value;
    }

    /// Construct state consisting of multiple attributes.
    UndoableAttributesState(Serializable* item, const HashMap <String, Variant>& values) : item_(item),
        attributes_(values)
    {
    }

    /// Apply attributes if they are different and return true if operation was carried out.
    bool Apply() override
    {
        if (IsCurrent())
            return false;

        for (auto it = attributes_.Begin(); it != attributes_.End(); it++)
            item_->SetAttribute(it->first_, it->second_);

        item_->ApplyAttributes();

        return true;
    }

    /// Return true if attributes saved in this state match attributes set to a serializable.
    bool IsCurrent() override
    {
        for (auto it = attributes_.Begin(); it != attributes_.End(); it++)
        {
            if (item_->GetAttribute(it->first_) != it->second_)
                return false;
        }
        return true;
    }

    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override
    {
        auto other_ = dynamic_cast<UndoableAttributesState*>(other);
        if (!other_)
            return false;

        if (item_ != other_->item_)
            return false;

        for (auto it = attributes_.Begin(); it != attributes_.End(); it++)
        {
            if (item_->GetAttribute(it->first_) != it->second_)
                return false;
        }

        return true;
    }

    /// Return string representation of current state.
    virtual String ToString() const
    {
        return "UndoableAttributesState";
    }
};

/// Tracks item parent state. Used for tracking adding and removing UIElements.
class UndoableItemParentState : public UndoableState
{
public:
    SharedPtr <UIElement> item_;
    SharedPtr <UIElement> parent_;
    unsigned index_ = M_MAX_UNSIGNED;

    /// Construct item from the state and parent
    UndoableItemParentState(UIElement* item, UIElement* parent) : item_(item), parent_(parent)
    {
        if (parent)
            index_ = parent->FindChild(item);
    }

    /// Set parent of the item if it is different and return true if operation was carried out.
    bool Apply() override
    {
        if (IsCurrent())
            return false;

        if (parent_.NotNull())
            item_->SetParent(parent_, index_);
        else
            item_->Remove();

        return true;
    }

    /// Return true if item parent matches and item is at recorded index.
    bool IsCurrent() override
    {
        if (item_->GetParent() != parent_)
            return false;

        if (parent_.NotNull() && parent_->FindChild(item_) != index_)
            return false;

        return true;
    }

    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override
    {
        auto other_ = dynamic_cast<UndoableItemParentState*>(other);

        if (other_ == nullptr)
            return false;

        return item_ == other_->item_ && parent_ == other_->parent_ && index_ == other_->index_;
    }

    /// Return string representation of current state.
    virtual String ToString() const
    {
        return "UndoableItemParentState";
    }
};

/// Tracks xml parent state. Used for tracking adding and removing xml elements to and from data files.
class UndoableXMLVariantState : public UndoableState
{
public:
    XMLElement item_;
    Variant value_;

    /// Construct item from the state and parent
    UndoableXMLVariantState(const XMLElement& item, const Variant& value) : item_(item), value_(value)
    {
    }

    /// Set parent of the item if it is different and return true if operation was carried out.
    bool Apply() override
    {
        if (IsCurrent())
            return false;

        item_.SetVariant(value_);
        return true;
    }

    /// Return true if item parent matches and item is at recorded index.
    bool IsCurrent() override
    {
        return item_.GetVariant() == value_;
    }

    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override
    {
        auto other_ = dynamic_cast<UndoableXMLVariantState*>(other);

        if (other_ == nullptr)
            return false;

        return item_ == other_->item_ && value_ == other_->value_;
    }

    String ToString() const override
    {
        return value_.ToString();
    }
};

/// Tracks item parent state. Used for tracking adding and removing UIElements.
class UndoableXMLParentState : public UndoableState
{
public:
    XMLElement item_;
    XMLElement parent_;

    /// Construct item from the state and parent
    UndoableXMLParentState(const XMLElement& item, const XMLElement& parent=XMLElement()) : item_(item), parent_(parent)
    {
    }

    /// Set parent of the item if it is different and return true if operation was carried out.
    bool Apply() override
    {
        if (IsCurrent())
            return false;

        if (parent_.NotNull())
            parent_.AppendChild(item_);
        else
            item_.GetParent().RemoveChild(item_);

        return true;
    }

    /// Return true if item parent matches and item is at recorded index.
    bool IsCurrent() override
    {
        return item_.GetParent().GetNode() == parent_.GetNode();
    }

    /// Return true if state of this object matches state of specified object.
    bool Equals(UndoableState* other) override
    {
        auto other_ = dynamic_cast<UndoableXMLParentState*>(other);

        if (other_ == nullptr)
            return false;

        return item_.GetNode() == other_->item_.GetNode() && parent_.GetNode() == other_->parent_.GetNode() /*&& index_ == other_->index_*/;
    }

    /// Return string representation of current state.
    virtual String ToString() const
    {
        return "UndoableXMLParentState";
    }
};

class UndoManager : public Object
{
    URHO3D_OBJECT(UndoManager, Object);
public:
    explicit UndoManager(Context* ctx) : Object(ctx)
    {

    }

    /// Go back in the state history.
    void Undo()
    {
        while (index_ >= 0 && index_ < stack_.Size() && !stack_[index_--]->Apply());
        index_ = Clamp<int32_t>(--index_, 0, stack_.Size() - 1);
    }

    /// Go forward in the state history.
    void Redo()
    {
        while (index_ >= 0 && index_ < stack_.Size() && !stack_[index_++]->Apply());
        index_ = Clamp<int32_t>(++index_, 0, stack_.Size() - 1);
    }

    /// Track item state consisting of single attribute.
    void TrackState(Serializable* item, const String& name, const Variant& value)
    {
        Track(new UndoableAttributesState(item, name, value));
    }

    /// Track item state consisting of multiple attributes.
    void TrackState(Serializable* item, const HashMap <String, Variant>& values)
    {
        Track(new UndoableAttributesState(item, values));
    }

    /// Track UIElement creation.
    void TrackCreation(UIElement* item)
    {
        // When item is created it has no parent
        Track(new UndoableItemParentState(item, nullptr));
        // Then it is added to element tree
        Track(new UndoableItemParentState(item, item->GetParent()));
    }

    /// Track UIElement removal.
    void TrackRemoval(UIElement* item)
    {
        // When item is being removed it still has a parent
        Track(new UndoableItemParentState(item, item->GetParent()));
        // Then it is removed from element tree
        Track(new UndoableItemParentState(item, nullptr));
    }

    void TrackCreation(const XMLElement& element)
    {
        // "Removed" element has empty variant value.
        Track(new UndoableXMLParentState(element));
        // When value is set element exists.
        Track(new UndoableXMLParentState(element, element.GetParent()));
    }

    void TrackRemoval(const XMLElement& element)
    {
        // When value is set element exists.
        Track(new UndoableXMLParentState(element, element.GetParent()));
        // "Removed" element has empty variant value.
        Track(new UndoableXMLParentState(element));
    }

    void TrackState(const XMLElement& element, const Variant& value)
    {
        Track(new UndoableXMLVariantState(element, value));
    }

protected:
    /// Track add undoable state to the state stack.
    void Track(UndoableState* state)
    {
        assert(state);

        // If current state matches state to be tracked - do nothing.
        if (!stack_.Empty() && stack_.Back()->Equals(state))
            return;

        // Discards any state that is further on the stack.
        stack_.Resize(++index_);
        // Tracks new state.
        stack_.Push(SharedPtr<UndoableState>(state));
        context_->GetLog()->Write(LOG_DEBUG, ToString("UNDO: Save %d: %s", index_, state->ToString().CString()));
    }

    /// State stack
    Vector<SharedPtr<UndoableState> > stack_;
    /// Current state index, -1 when stack is empty.
    int32_t index_ = -1;
};

}
