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

#include "UndoManager.h"

namespace Urho3D
{
UndoableAttributesState::UndoableAttributesState(Serializable* item, const String& name, const Variant& value) : item_(item)
{
    attributes_[name] = value;
}

UndoableAttributesState::UndoableAttributesState(Serializable* item, const HashMap<String, Variant>& values) : item_(item),
    attributes_(values)
{
}

bool UndoableAttributesState::Apply()
{
    if (IsCurrent())
        return false;

    for (auto it = attributes_.Begin(); it != attributes_.End(); it++)
        item_->SetAttribute(it->first_, it->second_);

    item_->ApplyAttributes();

    return true;
}

bool UndoableAttributesState::IsCurrent()
{
    for (auto it = attributes_.Begin(); it != attributes_.End(); it++)
    {
        if (item_->GetAttribute(it->first_) != it->second_)
            return false;
    }
    return true;
}

bool UndoableAttributesState::Equals(UndoableState* other)
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

String UndoableAttributesState::ToString() const
{
    return "UndoableAttributesState";
}

UndoableItemParentState::UndoableItemParentState(UIElement* item, UIElement* parent) : item_(item), parent_(parent)
{
    if (parent)
        index_ = parent->FindChild(item);
}

bool UndoableItemParentState::Apply()
{
    if (IsCurrent())
        return false;

    if (parent_.NotNull())
        item_->SetParent(parent_, index_);
    else
        item_->Remove();

    return true;
}

bool UndoableItemParentState::IsCurrent()
{
    if (item_->GetParent() != parent_)
        return false;

    if (parent_.NotNull() && parent_->FindChild(item_) != index_)
        return false;

    return true;
}

bool UndoableItemParentState::Equals(UndoableState* other)
{
    auto other_ = dynamic_cast<UndoableItemParentState*>(other);

    if (other_ == nullptr)
        return false;

    return item_ == other_->item_ && parent_ == other_->parent_ && index_ == other_->index_;
}

String UndoableItemParentState::ToString() const
{
    return "UndoableItemParentState";
}

UndoableXMLVariantState::UndoableXMLVariantState(const XMLElement& item, const Variant& value) : item_(item), value_(value)
{
}

bool UndoableXMLVariantState::Apply()
{
    if (IsCurrent())
        return false;

    item_.SetVariant(value_);
    return true;
}

bool UndoableXMLVariantState::IsCurrent()
{
    return item_.GetVariant() == value_;
}

bool UndoableXMLVariantState::Equals(UndoableState* other)
{
    auto other_ = dynamic_cast<UndoableXMLVariantState*>(other);

    if (other_ == nullptr)
        return false;

    return item_ == other_->item_ && value_ == other_->value_;
}

String UndoableXMLVariantState::ToString() const
{
    return value_.ToString();
}

UndoableXMLParentState::UndoableXMLParentState(const XMLElement& item, const XMLElement& parent) : item_(item), parent_(parent)
{
}

bool UndoableXMLParentState::Apply()
{
    if (IsCurrent())
        return false;

    if (parent_.NotNull())
        parent_.AppendChild(item_);
    else
        item_.GetParent().RemoveChild(item_);

    return true;
}

bool UndoableXMLParentState::IsCurrent()
{
    return item_.GetParent().GetNode() == parent_.GetNode();
}

bool UndoableXMLParentState::Equals(UndoableState* other)
{
    auto other_ = dynamic_cast<UndoableXMLParentState*>(other);

    if (other_ == nullptr)
        return false;

    return item_.GetNode() == other_->item_.GetNode() && parent_.GetNode() == other_->parent_.GetNode() /*&& index_ == other_->index_*/;
}

String UndoableXMLParentState::ToString() const
{
    return "UndoableXMLParentState";
}

UndoManager::UndoManager(Context* ctx) : Object(ctx)
{

}

void UndoManager::Undo()
{
    while (index_ >= 0 && index_ < stack_.Size() && !stack_[index_--]->Apply());
    index_ = Clamp<int32_t>(--index_, 0, stack_.Size() - 1);
}

void UndoManager::Redo()
{
    while (index_ >= 0 && index_ < stack_.Size() && !stack_[index_++]->Apply());
    index_ = Clamp<int32_t>(++index_, 0, stack_.Size() - 1);
}

void UndoManager::TrackState(Serializable* item, const String& name, const Variant& value)
{
    Track(new UndoableAttributesState(item, name, value));
}

void UndoManager::TrackState(Serializable* item, const HashMap<String, Variant>& values)
{
    Track(new UndoableAttributesState(item, values));
}

void UndoManager::TrackCreation(UIElement* item)
{
    // When item is created it has no parent
    Track(new UndoableItemParentState(item, nullptr));
    // Then it is added to element tree
    Track(new UndoableItemParentState(item, item->GetParent()));
}

void UndoManager::TrackRemoval(UIElement* item)
{
    // When item is being removed it still has a parent
    Track(new UndoableItemParentState(item, item->GetParent()));
    // Then it is removed from element tree
    Track(new UndoableItemParentState(item, nullptr));
}

void UndoManager::TrackCreation(const XMLElement& element)
{
    // "Removed" element has empty variant value.
    Track(new UndoableXMLParentState(element));
    // When value is set element exists.
    Track(new UndoableXMLParentState(element, element.GetParent()));
}

void UndoManager::TrackRemoval(const XMLElement& element)
{
    // When value is set element exists.
    Track(new UndoableXMLParentState(element, element.GetParent()));
    // "Removed" element has empty variant value.
    Track(new UndoableXMLParentState(element));
}

void UndoManager::TrackState(const XMLElement& element, const Variant& value)
{
    Track(new UndoableXMLVariantState(element, value));
}

void UndoManager::Track(UndoableState* state)
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

}
