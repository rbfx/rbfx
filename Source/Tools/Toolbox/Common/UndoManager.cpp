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

#include <Urho3D/Core/CoreEvents.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include "UndoManager.h"

namespace Urho3D
{

namespace Undo
{

AttributeState::AttributeState(Serializable* item, const String& name, const Variant& value)
    : item_(item)
    , name_(name)
    , value_(value)
{
}

bool AttributeState::Apply()
{
    if (IsCurrent())
        return false;

    item_->SetAttribute(name_, value_);
    item_->ApplyAttributes();

    return true;
}

bool AttributeState::IsCurrent() const
{
    return item_->GetAttribute(name_) == value_;
}

bool AttributeState::Equals(State* other) const
{
    auto other_ = dynamic_cast<AttributeState*>(other);
    if (!other_)
        return false;

    if (item_ != other_->item_)
        return false;

    return value_ == other_->value_;
}

String AttributeState::ToString() const
{
    return Urho3D::ToString("AttributeState %s = %s", name_.CString(), value_.ToString().CString());
}

ElementParentState::ElementParentState(UIElement* item, UIElement* parent) : item_(item), parent_(parent)
{
    if (parent)
        index_ = parent->FindChild(item);
}

bool ElementParentState::Apply()
{
    if (IsCurrent())
        return false;

    if (parent_.NotNull())
        item_->SetParent(parent_, index_);
    else
        item_->Remove();

    return true;
}

bool ElementParentState::IsCurrent() const
{
    if (item_->GetParent() != parent_)
        return false;

    if (parent_.NotNull() && parent_->FindChild(item_) != index_)
        return false;

    return true;
}

bool ElementParentState::Equals(State* other) const
{
    auto other_ = dynamic_cast<ElementParentState*>(other);

    if (other_ == nullptr)
        return false;

    return item_ == other_->item_ && parent_ == other_->parent_ && index_ == other_->index_;
}

String ElementParentState::ToString() const
{
    return "UndoableElementParentState";
}

NodeParentState::NodeParentState(Node* item, Node* parent) : item_(item), parent_(parent)
{
    if (parent)
        index_ = parent->GetChildren().IndexOf(SharedPtr<Node>(item));
}

bool NodeParentState::Apply()
{
    if (IsCurrent())
        return false;

    if (parent_.NotNull())
        parent_->AddChild(item_, index_);
    else
        item_->Remove();

    return true;
}

bool NodeParentState::IsCurrent() const
{
    if (item_->GetParent() != parent_)
        return false;

    if (parent_.NotNull() && parent_->GetChildren().IndexOf(item_) != index_)
        return false;

    return true;
}

bool NodeParentState::Equals(State* other) const
{
    auto other_ = dynamic_cast<NodeParentState*>(other);

    if (other_ == nullptr)
        return false;

    return item_ == other_->item_ && parent_ == other_->parent_ && index_ == other_->index_;
}

String NodeParentState::ToString() const
{
    return "UndoableNodeParentState";
}

XMLVariantState::XMLVariantState(const XMLElement& item, const Variant& value) : item_(item), value_(value)
{
}

bool XMLVariantState::Apply()
{
    if (IsCurrent())
        return false;

    item_.SetVariant(value_);
    return true;
}

bool XMLVariantState::IsCurrent() const
{
    return item_.GetVariant() == value_;
}

bool XMLVariantState::Equals(State* other) const
{
    auto other_ = dynamic_cast<XMLVariantState*>(other);

    if (other_ == nullptr)
        return false;

    return item_ == other_->item_ && value_ == other_->value_;
}

String XMLVariantState::ToString() const
{
    return value_.ToString();
}

XMLParentState::XMLParentState(const XMLElement& item, const XMLElement& parent) : item_(item), parent_(parent)
{
}

bool XMLParentState::Apply()
{
    if (IsCurrent())
        return false;

    if (parent_.NotNull())
        parent_.AppendChild(item_);
    else
        item_.GetParent().RemoveChild(item_);

    return true;
}

bool XMLParentState::IsCurrent() const
{
    return item_.GetParent().GetNode() == parent_.GetNode();
}

bool XMLParentState::Equals(State* other) const
{
    auto other_ = dynamic_cast<XMLParentState*>(other);

    if (other_ == nullptr)
        return false;

    return item_.GetNode() == other_->item_.GetNode() && parent_.GetNode() == other_->parent_.GetNode() /*&& index_ == other_->index_*/;
}

String XMLParentState::ToString() const
{
    return "UndoableXMLParentState";
}

bool StateCollection::Apply()
{
    bool applied = false;
    for (auto& state : states_)
        applied |= state->Apply();
    return applied;
}

bool StateCollection::Contains(State* other) const
{
    for (const auto& state : states_)
    {
        if (state->Equals(other))
            return true;
    }
    return false;
}

bool StateCollection::PushUnique(const SharedPtr<State>& state)
{
    if (!Contains(state))
    {
        states_.Push(state);
        return true;
    }
    return false;
}

Manager::Manager(Context* ctx) : Object(ctx)
{
    SubscribeToEvent(E_ENDFRAME, [&](StringHash, VariantMap&)
    {
        if (!trackingSuspended_)
        {
            for (auto& container : {previous_, next_})
            {
                if (container.Size())
                {
                    index_++;
                    stack_.Resize(index_ + 1);
                    stack_.Back().states_.Clear();
                    for (auto& state : container)
                    {
                        if (stack_.Back().PushUnique(state))
                            URHO3D_LOGDEBUGF("UNDO: Save %d: %s", index_, state->ToString().CString());
                    }
                }
            }
        }
        previous_.Clear();
        next_.Clear();
    });
}

void Manager::Undo()
{
    ApplyStateFromStack(false);
}

void Manager::Redo()
{
    ApplyStateFromStack(true);
}

void Manager::Clear()
{
    previous_.Clear();
    next_.Clear();
    stack_.Clear();
    index_ = -1;
}

void Manager::ApplyStateFromStack(bool forward)
{
    trackingSuspended_ = true;
    int direction = forward ? 1 : -1;
    while (index_ >= 0 && index_ < stack_.Size())
    {
        auto stateCollection = stack_[index_];
        index_ += direction;
        if (stateCollection.Apply())
        {
            URHO3D_LOGDEBUGF("Undo: apply %d", index_ - direction);
            break;
        }
        else
            URHO3D_LOGDEBUGF("Undo: apply skipped %d", index_ - direction);
    }
    index_ = Clamp<int32_t>(index_, 0, stack_.Size() - 1);
    trackingSuspended_ = false;
}

void Manager::TrackState(Serializable* item, const String& name, const Variant& value, const Variant& oldValue)
{
    // Item has it's state already modified, manually track the change.
    TrackBefore<AttributeState>(item, name, oldValue);
    TrackAfter<AttributeState>(item, name, value);
}

XMLElement Manager::XMLCreate(XMLElement& parent, const String& name)
{
    auto element = parent.CreateChild(name);
    TrackBefore<XMLParentState>(element);                       // "Removed" element has empty variant value.
    TrackAfter<XMLParentState>(element, element.GetParent());   // When value is set element exists.
    return element;
}

void Manager::XMLRemove(XMLElement& element)
{
    TrackBefore<XMLParentState>(element, element.GetParent());  // When value is set element exists.
    TrackAfter<XMLParentState>(element);                        // "Removed" element has empty variant value.
    element.Remove();
}

void Manager::XMLSetVariantValue(XMLElement& element, const Variant& value)
{
    TrackBefore<XMLVariantState>(element, element.GetVariant());
    TrackAfter<XMLVariantState>(element, value);
    element.SetVariantValue(value);
}

template<typename T, typename... Args>
void Manager::TrackBefore(Args... args)
{
    previous_.Push(SharedPtr<State>(new T(args...)));
}

template<typename T, typename... Args>
void Manager::TrackAfter(Args... args)
{
    next_.Push(SharedPtr<State>(new T(args...)));
}
void Manager::Connect(Scene* scene)
{
    UnsubscribeFromEvent(E_NODEADDED);
    UnsubscribeFromEvent(E_NODEREMOVED);

    SubscribeToEvent(scene, E_NODEADDED, [&](StringHash, VariantMap& args) {
        if (trackingSuspended_)
            return;

        using namespace NodeRemoved;
        auto node = dynamic_cast<Node*>(args[P_NODE].GetPtr());
        auto parent = dynamic_cast<Node*>(args[P_PARENT].GetPtr());

        TrackBefore<NodeParentState>(node, nullptr);        // Removed from the scene state
        TrackAfter<NodeParentState>(node, parent);          // Present in the scene state
    });

    SubscribeToEvent(scene, E_NODEREMOVED, [&](StringHash, VariantMap& args) {
        if (trackingSuspended_)
            return;

        using namespace NodeRemoved;
        auto node = dynamic_cast<Node*>(args[P_NODE].GetPtr());
        auto parent = dynamic_cast<Node*>(args[P_PARENT].GetPtr());

        TrackBefore<NodeParentState>(node, parent);        // Present in the scene state
        TrackAfter<NodeParentState>(node, nullptr);        // Removed from the scene state
    });
}

void Manager::Connect(AttributeInspector* inspector)
{
    UnsubscribeFromEvent(E_ATTRIBUTEINSPECTVALUEMODIFIED);

    SubscribeToEvent(inspector, E_ATTRIBUTEINSPECTVALUEMODIFIED, [&](StringHash, VariantMap& args) {
        if (trackingSuspended_)
            return;

        using namespace AttributeInspectorValueModified;
        auto item = dynamic_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());

        auto attributeName = reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr())->name_;
        TrackBefore<AttributeState>(item, attributeName, args[P_OLDVALUE]);
        TrackAfter<AttributeState>(item, attributeName, args[P_NEWVALUE]);
    });
}

void Manager::Connect(UIElement* root)
{
    UnsubscribeFromEvent(E_ELEMENTADDED);
    UnsubscribeFromEvent(E_ELEMENTREMOVED);

    SubscribeToEvent(E_ELEMENTADDED, [&, root](StringHash, VariantMap& args) {
        if (trackingSuspended_)
            return;

        using namespace ElementAdded;
        auto element = dynamic_cast<UIElement*>(args[P_ELEMENT].GetPtr());
        auto parent = dynamic_cast<UIElement*>(args[P_PARENT].GetPtr());
        auto eventRoot = dynamic_cast<UIElement*>(args[P_ROOT].GetPtr());

        if (root != eventRoot)
            return;

        TrackBefore<ElementParentState>(element, nullptr);        // Removed from the scene state
        TrackAfter<ElementParentState>(element, parent);          // Present in the scene state
    });

    SubscribeToEvent(E_ELEMENTREMOVED, [&, root](StringHash, VariantMap& args) {
        if (trackingSuspended_)
            return;

        using namespace ElementRemoved;
        auto element = dynamic_cast<UIElement*>(args[P_ELEMENT].GetPtr());
        auto parent = dynamic_cast<UIElement*>(args[P_PARENT].GetPtr());
        auto eventRoot = dynamic_cast<UIElement*>(args[P_ROOT].GetPtr());

        if (root != eventRoot)
            return;

        TrackBefore<ElementParentState>(element, parent);        // Removed from the scene state
        TrackAfter<ElementParentState>(element, nullptr);        // Present in the scene state
    });
}

void Manager::Connect(Gizmo* gizmo)
{
    // TODO
}

}

}
