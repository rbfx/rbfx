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

#include "../Core/Signal.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../SystemUI/SerializableInspectorWidget.h"

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
