// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Signal.h"
#include "Urho3D/Scene/Component.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/SystemUI/SerializableInspectorWidget.h"

namespace Urho3D
{

/// SystemUI widget used to edit materials.
class URHO3D_API NodeInspectorWidget : public Object
{
    URHO3D_OBJECT(NodeInspectorWidget, Object);

public:
    using NodeVector = ea::vector<WeakPtr<Node>>;

    Signal<void(const WeakSerializableVector& objects, const AttributeInfo* attribute)> OnEditNodeAttributeBegin;
    Signal<void(const WeakSerializableVector& objects, const AttributeInfo* attribute)> OnEditNodeAttributeEnd;
    Signal<void(const WeakSerializableVector& objects, const AttributeInfo* attribute)> OnEditComponentAttributeBegin;
    Signal<void(const WeakSerializableVector& objects, const AttributeInfo* attribute)> OnEditComponentAttributeEnd;
    Signal<void(const WeakSerializableVector& objects)> OnActionBegin;
    Signal<void(const WeakSerializableVector& objects)> OnActionEnd;
    Signal<void(Component* component)> OnComponentRemoved;

    NodeInspectorWidget(Context* context, const NodeVector& nodes);
    ~NodeInspectorWidget() override;

    void RenderTitle();
    void RenderContent();

    const NodeVector& GetNodes() const { return nodes_; }

private:
    using NodeComponentVector = ea::vector<ea::pair<Node*, Component*>>;
    using ComponentVectorsByType = ea::vector<ea::vector<WeakPtr<Component>>>;

    NodeComponentVector GetAllComponents() const;
    ComponentVectorsByType GetSharedComponents() const;

    NodeVector nodes_;
    SharedPtr<SerializableInspectorWidget> nodeInspector_;

    NodeComponentVector components_;
    ea::vector<SharedPtr<SerializableInspectorWidget>> componentInspectors_;
    unsigned numSkippedComponents_{};

    ea::vector<WeakPtr<Component>> pendingRemoveComponents_;
};

}
