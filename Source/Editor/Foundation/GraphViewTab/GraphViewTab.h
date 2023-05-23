//
// Copyright (c) 2023-2023 the rbfx project.
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
#include "../../Foundation/Shared/CameraController.h"
#include "../../Project/Project.h"
#include "../../Project/ResourceEditorTab.h"

#include <Urho3D/Resource/Graph.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <ImGuiNodeEditor/imgui_node_editor.h>
#include <EASTL/fixed_vector.h>

namespace Urho3D
{
namespace Detail
{
struct GraphLinkView
{
    ax::NodeEditor::PinId from_;
    ax::NodeEditor::PinId to_;
};

enum class GraphPinViewType
{
    Input,
    Output,
    Enter,
    Exit
};

struct GraphPinView
{
    GraphPinView(ax::NodeEditor::PinId id, const ea::string& title, GraphPinViewType pinType);
    GraphPinView(ax::NodeEditor::PinId id, const ea::string& title, GraphPinViewType pinType, VariantType type,
        const Variant& value);
    /// Global unique identifier of the pin.
    ax::NodeEditor::PinId id_{};
    /// Name of the pin.
    ea::string title_;
    /// Field type. VAR_NONE means that it could be of any type.
    VariantType valueType_{VAR_NONE};
    /// Type of the pin.
    GraphPinViewType pinType_;
    /// Field value.
    Variant value_{};
    /// Temporal value for editor.
    Variant tempValue_{};
    /// Value as text string.
    ea::string text_;
    /// Pin type.
    ax::NodeEditor::PinKind kind_{ax::NodeEditor::PinKind::Input};
    /// Connected link ID
    ax::NodeEditor::LinkId link_;
};

/// Reference to node's pin view
struct PinNodeViewRef
{
    ax::NodeEditor::NodeId node_;
    GraphPinViewType type_;
    unsigned index_;
};

struct GraphNodeView
{
    GraphNodeView(ax::NodeEditor::NodeId id, const ea::string& title);
    /// Global unique identifier of the node.
    ax::NodeEditor::NodeId id_{};
    ea::string title_;
    Vector2 position_{0, 0};
    Vector2 size_{0, 0};
    ea::fixed_vector<GraphPinView,1> enterPins_;
    ea::fixed_vector<GraphPinView,3> inputPins_;
    ea::fixed_vector<GraphPinView,1> exitPins_;
    ea::fixed_vector<GraphPinView,1> outputPins_;
    GraphPinView* GetPinView(const ::Urho3D::Detail::PinNodeViewRef& pinRef);
};

struct GraphView
{
    uintptr_t nextUniqueId_{1};
    ea::unordered_map<ax::NodeEditor::NodeId, GraphNodeView> nodes_;
    ea::unordered_map<ax::NodeEditor::PinId, PinNodeViewRef> pinToNode_;
    ea::unordered_map<ax::NodeEditor::LinkId, GraphLinkView> links_;

    GraphPinView* GetInputPinView(ax::NodeEditor::NodeId node, const ea::string& pinName);
    GraphPinView* GetInputPinView(ax::NodeEditor::NodeId node, unsigned pinIndex);

    GraphNodeView* GetNode(ax::NodeEditor::NodeId id);
    /// Reset graph view to empty.
    void Reset();
    /// Add link between two pins.
    void AddLink(ax::NodeEditor::LinkId id, GraphPinView& from, GraphPinView& to);
    /// Add link between two pins.
    void AddLink(ax::NodeEditor::LinkId id, ax::NodeEditor::PinId from, ax::NodeEditor::PinId to);
    /// Populate view from the graph resource.
    void Populate(Graph* graph);
    /// Build graph from view.
    SharedPtr<Graph> BuildGraph(Context* context);
    /// Add graph node.
    ax::NodeEditor::NodeId AddNode(GraphNode* graphNode);
    /// Add graph node.
    bool AddNode(const GraphNodeView& nodeView);
    /// Evaluate node positions.
    void AutoLayout();
    void DeleteLink(const ax::NodeEditor::LinkId& linkId);
    void DeleteNode(const ax::NodeEditor::NodeId& id);
};

} // namespace Detail

/// Tab that renders graph.
class GraphViewTab : public ResourceEditorTab
{
    URHO3D_OBJECT(GraphViewTab, ResourceEditorTab)

public:
    GraphViewTab(Context* context, const ea::string& title, const ea::string& guid, EditorTabFlags flags,
        EditorTabPlacement placement);
    ~GraphViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;
    bool IsUndoSupported() override { return true; }
    void ApplyLayoutFromView();
    /// @}

    Detail::GraphView* GetGraphView() { return &graph_; } 
    void Reset();

    virtual SharedPtr<GraphNode> CreateNewNodePopup() const;

protected:
    virtual void RenderTitle();

    void RenderGraph();
    void RenderPin(ax::NodeEditor::NodeId nodeId, Detail::GraphPinView& pin);
    void DeleteLink(const ax::NodeEditor::LinkId& link_id);
    bool CreateLink(const ax::NodeEditor::PinId& from, const ax::NodeEditor::PinId& to);

    ax::NodeEditor::EditorContext* editorContext_ = nullptr; // Editor context, required to trace a editor state.
    bool showOrdinals_{};
    bool applyLayout_{true};
    Detail::GraphView graph_;
    Widgets::EditVariantOptions editVariantOptions_;
    std::vector<ax::NodeEditor::NodeId> orderedNodeIds_;
    std::vector<ax::NodeEditor::NodeId> nodesToDelete_;
};

} // namespace Urho3D
