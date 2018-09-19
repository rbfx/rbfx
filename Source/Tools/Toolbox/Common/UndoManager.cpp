#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/SystemUI/SystemUIEvents.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include <Toolbox/SystemUI/Gizmo.h>
#include "UndoManager.h"

namespace Urho3D
{

namespace Undo
{

Manager::Manager(Context* ctx)
    : Object(ctx)
{
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&)
    {
        if (trackingEnabled_ && !currentFrameStates_.Empty())
        {
            stack_.Resize(index_);              // Discard unneeded states
            stack_.Push(currentFrameStates_);
            index_++;
            currentFrameStates_.Clear();
        }
    });

    SubscribeToEvent(E_UNDO, [this](StringHash, VariantMap& args)
    {
        if (index_ > 0)
        {
            // Pick a state with latest time
            auto time = stack_[index_ - 1][0]->time_;
            if (args[P_TIME].GetUInt() < time)
            {
                // Find and return undo manager with latest state recording
                args[P_TIME] = time;
                args[P_MANAGER] = this;
            }
        }
    });

    SubscribeToEvent(E_REDO, [this](StringHash, VariantMap& args)
    {
        if (index_ < stack_.Size())
        {
            auto time = stack_[index_][0]->time_;
            if (args[P_TIME].GetUInt() > time)
            {
                // Find and return undo manager with latest state recording
                args[P_TIME] = time;
                args[P_MANAGER] = this;
            }
        }
    });
}

void Manager::Undo()
{
    bool isTracking = IsTrackingEnabled();
    SetTrackingEnabled(false);
    if (index_ > 0)
    {
        index_--;
        const auto& actions = stack_[index_];
        for (int i = actions.Size() - 1; i >= 0; --i)
            actions[i]->Undo();
    }
    SetTrackingEnabled(isTracking);
}

void Manager::Redo()
{
    bool isTracking = IsTrackingEnabled();
    SetTrackingEnabled(false);
    if (index_ < stack_.Size())
    {
        for (auto& action : stack_[index_])
            action->Redo();
        index_++;
    }
    SetTrackingEnabled(isTracking);
}

void Manager::Clear()
{
    stack_.Clear();
    currentFrameStates_.Clear();
    index_ = 0;
}

void Manager::Connect(Scene* scene)
{
    SubscribeToEvent(scene, E_NODEADDED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        Track<CreateNodeAction>(dynamic_cast<Node*>(args[NodeAdded::P_NODE].GetPtr()));
    });

    SubscribeToEvent(scene, E_NODEREMOVED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        Track<DeleteNodeAction>(dynamic_cast<Node*>(args[NodeRemoved::P_NODE].GetPtr()));
    });

    SubscribeToEvent(scene, E_COMPONENTADDED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        Track<CreateComponentAction>(dynamic_cast<Component*>(args[ComponentAdded::P_COMPONENT].GetPtr()));
    });

    SubscribeToEvent(scene, E_COMPONENTREMOVED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        Track<DeleteComponentAction>(dynamic_cast<Component*>(args[ComponentAdded::P_COMPONENT].GetPtr()));
    });
}

void Manager::Connect(AttributeInspector* inspector)
{
    SubscribeToEvent(inspector, E_ATTRIBUTEINSPECTVALUEMODIFIED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace AttributeInspectorValueModified;
        auto item = dynamic_cast<Serializable*>(args[P_SERIALIZABLE].GetPtr());
        const auto& name = reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr())->name_;
        const auto& oldValue = args[P_OLDVALUE];
        const auto& newValue = item->GetAttribute(name);
        if (oldValue != newValue)
        {
            // Dummy attributes are used for rendering custom inspector widgets that do not map to Variant values.
            // These dummy values are not modified, however inspector event is still useful for tapping into their
            // modifications. State tracking for these dummy values is not needed and would introduce extra ctrl+z
            // presses that do nothing.
            Track<EditAttributeAction>(item, name, oldValue, newValue);
        }
    });
}

void Manager::Connect(UIElement* root)
{
    SubscribeToEvent(E_ELEMENTADDED, [&, root](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace ElementAdded;
        auto eventRoot = dynamic_cast<UIElement*>(args[P_ROOT].GetPtr());
        if (root != eventRoot)
            return;
        Track<CreateUIElementAction>(dynamic_cast<UIElement*>(args[P_ELEMENT].GetPtr()));
    });

    SubscribeToEvent(E_ELEMENTREMOVED, [&, root](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace ElementRemoved;
        auto eventRoot = dynamic_cast<UIElement*>(args[P_ROOT].GetPtr());
        if (root != eventRoot)
            return;
        Track<DeleteUIElementAction>(dynamic_cast<UIElement*>(args[P_ELEMENT].GetPtr()));
    });
}

void Manager::Connect(Gizmo* gizmo)
{
    SubscribeToEvent(gizmo, E_GIZMONODEMODIFIED, [&](StringHash, VariantMap& args)
    {
        if (!trackingEnabled_)
            return;
        using namespace GizmoNodeModified;
        auto node = dynamic_cast<Node*>(args[P_NODE].GetPtr());
        const auto& oldTransform = args[P_OLDTRANSFORM].GetMatrix3x4();
        const auto& newTransform = args[P_NEWTRANSFORM].GetMatrix3x4();

        Track<EditAttributeAction>(node, "Position", oldTransform.Translation(), newTransform.Translation());
        Track<EditAttributeAction>(node, "Rotation", oldTransform.Rotation(), newTransform.Rotation());
        Track<EditAttributeAction>(node, "Scale", oldTransform.Scale(), newTransform.Scale());
    });
}

}

}
