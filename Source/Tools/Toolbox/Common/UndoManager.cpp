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
#include <Urho3D/Scene/Component.h>
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

void AttributeState::Apply()
{
    item_->SetAttribute(name_, value_);
    item_->ApplyAttributes();
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

void ElementParentState::Apply()
{
    if (parent_.NotNull())
        item_->SetParent(parent_, index_);
    else
        item_->Remove();
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
    return Urho3D::ToString("ElementParentState parent = %p child = %p index = %d", parent_.Get(), item_.Get(), index_);
}

NodeParentState::NodeParentState(Node* item, Node* parent) : item_(item), parent_(parent)
{
    if (parent)
        index_ = parent->GetChildren().IndexOf(SharedPtr<Node>(item));
}

void NodeParentState::Apply()
{
    if (parent_.NotNull())
        parent_->AddChild(item_, index_);
    else
        item_->Remove();
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
    return Urho3D::ToString("NodeParentState parent = %p child = %p index = %d", parent_.Get(), item_.Get(), index_);
}

ComponentParentState::ComponentParentState(Component* item, Node* parent) : item_(item), parent_(parent)
{
    id_ = item->GetID();
}

void ComponentParentState::Apply()
{
    if (parent_.NotNull())
        parent_->AddComponent(item_, id_, LOCAL);   // Replication mode is irrelevant, because it is used only for ID
                                                    // creation.
    else
        item_->Remove();
}

bool ComponentParentState::Equals(State* other) const
{
    auto other_ = dynamic_cast<ComponentParentState*>(other);

    if (other_ == nullptr)
        return false;

    return item_ == other_->item_ && parent_ == other_->parent_ && id_ == other_->id_;
}

String ComponentParentState::ToString() const
{
    return Urho3D::ToString("ComponentParentState parent = %p child = %p id = %d", parent_.Get(), item_.Get(), id_);
}

XMLVariantState::XMLVariantState(const XMLElement& item, const Variant& value) : item_(item), value_(value)
{
}

void XMLVariantState::Apply()
{
    item_.SetVariant(value_);
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
    return Urho3D::ToString("XMLVariantState value = %s", value_.ToString().CString());
}

XMLParentState::XMLParentState(const XMLElement& item, const XMLElement& parent) : item_(item), parent_(parent)
{
}

void XMLParentState::Apply()
{
    if (parent_.NotNull())
        parent_.AppendChild(item_);
    else
        item_.GetParent().RemoveChild(item_);
}

bool XMLParentState::Equals(State* other) const
{
    auto other_ = dynamic_cast<XMLParentState*>(other);

    if (other_ == nullptr)
        return false;

    return item_.GetNode() == other_->item_.GetNode() && parent_.GetNode() == other_->parent_.GetNode();
}

String XMLParentState::ToString() const
{
    return Urho3D::ToString("XMLParentState parent = %s", parent_.IsNull() ? "null" : "set");
}

void StateCollection::Apply()
{
    for (auto& state : states_)
        state->Apply();
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
            assert(previous_.Size() == next_.Size());

            if (previous_.Size())
            {
                // When stack is empty we insert two items - old state and new state. When stack already has states
                // saved - we save old state to the state collection at the end of the stack and insert new
                // collection for a new state.
                if (stack_.Empty())
                {
                    stack_.Resize(1);
                    index_++;
                }

                for (auto& state : previous_)
                {
                    if (stack_.Back().PushUnique(state))
                        URHO3D_LOGDEBUGF("UNDO: Save %d: %s", index_, state->ToString().CString());
                }

                index_++;
                stack_.Resize(index_ + 1);

                for (auto& state : next_)
                {
                    if (stack_.Back().PushUnique(state))
                        URHO3D_LOGDEBUGF("UNDO: Save %d: %s", index_, state->ToString().CString());
                }
                previous_.Clear();
                next_.Clear();
            }
        }
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
    index_ += direction;
    if (index_ >= 0 && index_ < stack_.Size())
    {
        stack_[index_].Apply();
        URHO3D_LOGDEBUGF("Undo: apply %d", index_);
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

    SubscribeToEvent(scene, E_COMPONENTADDED, [&](StringHash, VariantMap& args) {
        if (trackingSuspended_)
            return;

        using namespace ComponentAdded;
        auto component = dynamic_cast<Component*>(args[P_COMPONENT].GetPtr());
        auto parent = dynamic_cast<Node*>(args[P_NODE].GetPtr());

        TrackBefore<ComponentParentState>(component, nullptr);
        TrackAfter<ComponentParentState>(component, parent);
    });

    SubscribeToEvent(scene, E_COMPONENTREMOVED, [&](StringHash, VariantMap& args) {
        if (trackingSuspended_)
            return;

        using namespace ComponentAdded;
        auto component = dynamic_cast<Component*>(args[P_COMPONENT].GetPtr());
        auto parent = dynamic_cast<Node*>(args[P_NODE].GetPtr());

        TrackBefore<ComponentParentState>(component, parent);
        TrackAfter<ComponentParentState>(component, nullptr);
    });
}

void Manager::Connect(AttributeInspector* inspector)
{
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
    SubscribeToEvent(gizmo, E_GIZMONODEMODIFIED, [&](StringHash, VariantMap& args) {
        using namespace GizmoNodeModified;
        auto node = dynamic_cast<Node*>(args[P_NODE].GetPtr());
        auto oldTransform = args[P_OLDTRANSFORM].GetMatrix3x4();
        auto newTransform = args[P_NEWTRANSFORM].GetMatrix3x4();

        TrackBefore<AttributeState>(node, "Position", oldTransform.Translation());
        TrackBefore<AttributeState>(node, "Rotation", oldTransform.Rotation());
        TrackBefore<AttributeState>(node, "Scale", oldTransform.Scale());

        TrackAfter<AttributeState>(node, "Position", newTransform.Translation());
        TrackAfter<AttributeState>(node, "Rotation", newTransform.Rotation());
        TrackAfter<AttributeState>(node, "Scale", newTransform.Scale());
    });
}

}

}
