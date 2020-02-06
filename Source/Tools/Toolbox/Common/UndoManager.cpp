#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/SystemUI/SystemUIEvents.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include <Toolbox/SystemUI/Gizmo.h>
#include "UndoManager.h"

namespace Urho3D
{

UndoStack::UndoStack(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&)
    {
        if (!trackingEnabled_ || currentFrameActions_.empty())
            return;

        auto time = GetSubsystem<Time>();
        unsigned frame = time->GetFrameNumber();
        for (auto& action : currentFrameActions_)
            action->frame_ = frame;

        stack_.resize(index_);              // Discard unneeded states
        stack_.push_back(currentFrameActions_);
        index_++;
        currentFrameActions_.clear();
    });

    SubscribeToEvent(E_UNDO, [this](StringHash, VariantMap& args)
    {
        if (index_ <= 0)
            return;

        // Pick a state with latest time
        using namespace UndoEvent;
        unsigned frame = stack_[index_ - 1][0]->frame_;
        if (args[P_FRAME].GetUInt() < frame)
        {
            // Find and return undo manager with latest state recording
            args[P_FRAME] = frame;
            args[P_MANAGER] = this;
        }
    });

    SubscribeToEvent(E_REDO, [this](StringHash, VariantMap& args)
    {
        if (index_ >= stack_.size())
            return;

        using namespace RedoEvent;
        unsigned frame = stack_[index_][0]->frame_;
        if (args[P_FRAME].GetUInt() > frame)
        {
            // Find and return undo manager with latest state recording
            args[P_FRAME] = frame;
            args[P_MANAGER] = this;
        }
    });
}

void UndoStack::Undo()
{
    if (index_ > 0)
    {
        UndoTrackGuard noTrack(this, false);
        workingValueCache_.Clear();
        index_--;
        const auto& actions = stack_[index_];
        for (int i = (int)actions.size() - 1; i >= 0; --i)
            actions[i]->Undo(context_);
    }
}

void UndoStack::Redo()
{
    if (index_ < stack_.size())
    {
        UndoTrackGuard noTrack(this, false);
        workingValueCache_.Clear();
        for (auto& action : stack_[index_])
            action->Redo(context_);
        index_++;
    }
}

void UndoStack::Clear()
{
    stack_.clear();
    currentFrameActions_.clear();
    index_ = 0;
}

void UndoStack::Connect(Scene* scene)
{
    Connect(static_cast<Object*>(scene));

    SubscribeToEvent(scene, E_NODEADDED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        auto* node = dynamic_cast<Node*>(args[NodeAdded::P_NODE].GetPtr());
        if (node->HasTag("__EDITOR_OBJECT__"))
            return;
        Add<UndoCreateNode>(node);
    });

    SubscribeToEvent(scene, E_NODEREMOVED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        auto* node = dynamic_cast<Node*>(args[NodeRemoved::P_NODE].GetPtr());
        if (node->HasTag("__EDITOR_OBJECT__"))
            return;
        Add<UndoDeleteNode>(node);
    });

    SubscribeToEvent(scene, E_COMPONENTADDED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        auto* node = dynamic_cast<Node*>(args[ComponentAdded::P_NODE].GetPtr());
        auto* component = dynamic_cast<Component*>(args[ComponentAdded::P_COMPONENT].GetPtr());
        if (node->HasTag("__EDITOR_OBJECT__"))
            return;
        Add<UndoCreateComponent>(component);
    });

    SubscribeToEvent(scene, E_COMPONENTREMOVED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        auto* node = dynamic_cast<Node*>(args[ComponentRemoved::P_NODE].GetPtr());
        auto* component = dynamic_cast<Component*>(args[ComponentRemoved::P_COMPONENT].GetPtr());
        if (node->HasTag("__EDITOR_OBJECT__"))
            return;
        Add<UndoDeleteComponent>(component);
    });
}

void UndoStack::Connect(Object* inspector)
{
    SubscribeToEvent(inspector, E_ATTRIBUTEINSPECTVALUEMODIFIED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace AttributeInspectorValueModified;
        auto item = dynamic_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());
        if (Node* node = dynamic_cast<Node*>(item))
        {
            if (node->HasTag("__EDITOR_OBJECT__"))
                return;
        }

        const auto& name = reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr())->name_;
        const auto& oldValue = args[P_OLDVALUE];
        const auto& newValue = item->GetAttribute(name);
        if (oldValue != newValue)
        {
            // Dummy attributes are used for rendering custom inspector widgets that do not map to Variant values.
            // These dummy values are not modified, however inspector event is still useful for tapping into their
            // modifications. State tracking for these dummy values is not needed and would introduce extra ctrl+z
            // presses that do nothing.
            Add<UndoEditAttribute>(item, name, oldValue, newValue);
        }
    });
}

void UndoStack::Connect(UIElement* root)
{
    assert(root->IsElementEventSender());

    Connect(static_cast<Object*>(root));

    SubscribeToEvent(root, E_ELEMENTADDED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace ElementAdded;
        Add<UndoCreateUIElement>(dynamic_cast<UIElement*>(args[P_ELEMENT].GetPtr()));
    });
    SubscribeToEvent(root, E_ELEMENTREMOVED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace ElementRemoved;
        Add<UndoDeleteUIElement>(dynamic_cast<UIElement*>(args[P_ELEMENT].GetPtr()));
    });
}

void UndoStack::Connect(Gizmo* gizmo)
{
    SubscribeToEvent(gizmo, E_GIZMONODEMODIFIED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace GizmoNodeModified;
        auto node = dynamic_cast<Node*>(args[P_NODE].GetPtr());
        if (node->HasTag("__EDITOR_OBJECT__"))
            return;
        const auto& oldTransform = args[P_OLDTRANSFORM].GetMatrix3x4();
        const auto& newTransform = args[P_NEWTRANSFORM].GetMatrix3x4();

        Add<UndoEditAttribute>(node, "Position", oldTransform.Translation(), newTransform.Translation());
        Add<UndoEditAttribute>(node, "Rotation", oldTransform.Rotation(), newTransform.Rotation());
        Add<UndoEditAttribute>(node, "Scale", oldTransform.Scale(), newTransform.Scale());
    });
}

}
