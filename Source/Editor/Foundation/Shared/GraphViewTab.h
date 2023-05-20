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

#include <ImGuiNodeEditor/imgui_node_editor.h>
#include <EASTL/fixed_vector.h>

namespace Urho3D
{

struct GraphLinkView
{
    ax::NodeEditor::PinId from_;
    ax::NodeEditor::PinId to_;
};

struct GraphPinView
{
    GraphPinView(ax::NodeEditor::PinId id);
    ax::NodeEditor::PinId id_{0};
    ea::string title_;
    VariantType type_{VAR_NONE};
    ea::string value_;
    ax::NodeEditor::PinKind kind_{ax::NodeEditor::PinKind::Input};
};

struct GraphNodeView
{
    ax::NodeEditor::NodeId id_;
    ea::string title_;
    Vector2 position_{0, 0};
    Vector2 size_{0, 0};
    ea::vector<GraphPinView> enterPins_;
    ea::vector<GraphPinView> inputPins_;
    ea::vector<GraphPinView> exitPins_;
    ea::vector<GraphPinView> outputPins_;
};

struct GraphView
{
    uintptr_t nextUniqueId_{1};
    ea::vector<GraphNodeView> nodes_;
    ea::unordered_map<ax::NodeEditor::LinkId, GraphLinkView> links_;

    void Reset();
    void Populate(Graph* graph);
    void AutoLayout();
};

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
    /// @}

    GraphView* GetGraphView() { return &graph_; } 
    void Reset();

    virtual void RenderGraph();

protected:
    virtual void RenderTitle();

    ax::NodeEditor::EditorContext* editorContext_ = nullptr; // Editor context, required to trace a editor state.
    bool showOrdinals_{};
    bool applyLayout_{true};
    GraphView graph_;
    std::vector<ax::NodeEditor::NodeId> orderedNodeIds_;
};

} // namespace Urho3D
