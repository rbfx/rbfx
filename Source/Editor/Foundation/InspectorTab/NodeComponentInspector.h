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

#include "../../Core/CommonEditorActions.h"
#include "../../Foundation/InspectorTab.h"
#include "../../Project/ProjectRequest.h"

#include <Urho3D/SystemUI/NodeInspectorWidget.h>
#include <Urho3D/SystemUI/SerializableInspectorWidget.h>
#include <Urho3D/Utility/PackedSceneData.h>
#include <Urho3D/Utility/SceneSelection.h>

#include <EASTL/map.h>

namespace Urho3D
{

void Foundation_NodeComponentInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class NodeComponentInspector : public Object, public InspectorSource
{
    URHO3D_OBJECT(NodeComponentInspector, Object)

public:
    explicit NodeComponentInspector(Project* project);

    /// Implement InspectorSource
    /// @{
    EditorTab* GetOwnerTab() override { return inspectedTab_; }
    bool IsUndoSupported() override { return true; }

    void RenderContent() override;
    void RenderContextMenuItems() override;
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    using NodeVector = NodeInspectorWidget::NodeVector;

    struct AttributeSnapshot
    {
        VariantVector values_;
        ea::vector<PackedNodeData> nodes_;
        PackedSceneData scene_;
    };

    void OnProjectRequest(RefCounted* senderTab, ProjectRequest* request);

    NodeVector CollectNodes() const;
    WeakSerializableVector CollectComponents() const;
    void InspectObjects();

    void RenderComponentSummary();
    void RenderAddComponent();

    void BeginEditNodeAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void EndEditNodeAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void BeginEditComponentAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void EndEditComponentAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void BeginAction(const WeakSerializableVector& objects);
    void EndAction(const WeakSerializableVector& objects);
    void AddComponentToNodes(StringHash componentType);
    void RemoveComponent(Component* component);

    WeakPtr<Project> project_;

    WeakPtr<EditorTab> inspectedTab_;
    WeakPtr<Scene> scene_;
    InspectNodeComponentRequest::WeakNodeVector nodes_;
    InspectNodeComponentRequest::WeakComponentVector components_;

    SharedPtr<NodeInspectorWidget> nodeWidget_;
    SharedPtr<SerializableInspectorWidget> componentWidget_;
    ea::map<ea::string, unsigned> componentSummary_;

    ChangeAttributeBuffer actionBuffer_;
    ea::unique_ptr<ChangeNodeAttributesActionBuilder> nodeActionBuilder_;
    ea::unique_ptr<ChangeComponentAttributesActionBuilder> componentActionBuilder_;

    NodeVector changedNodes_;
    ea::vector<PackedNodeData> oldData_;
};

}
