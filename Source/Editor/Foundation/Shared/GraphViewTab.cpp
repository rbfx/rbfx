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

#include "../../Core/IniHelpers.h"
#include "../../Foundation/Shared/GraphViewTab.h"

#include "Urho3D/Resource/GraphNode.h"

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <ImGuiNodeEditor/utilities/widgets.h>

namespace ed = ax::NodeEditor;

namespace Urho3D
{
namespace
{
    struct StablePinKey
    {
        unsigned nodeId_;
        unsigned pinIndex_;

        template <typename T> StablePinKey(const GraphPinRef<T>& pinRef)
            : nodeId_(pinRef.GetNode() ? pinRef.GetNode()->GetID() : 0)
            , pinIndex_(pinRef.GetNode() ? pinRef.GetNode()->GetPinIndex(pinRef.GetPin()) : 0)
        {
        }
        bool operator<(const StablePinKey& rhs) const
        {
            return (nodeId_ < rhs.nodeId_) || ((nodeId_ == rhs.nodeId_) && (pinIndex_ < rhs.pinIndex_));
        }
        bool operator ==(const StablePinKey& rhs) const
        {
            return (nodeId_ == rhs.nodeId_) && (pinIndex_ == rhs.pinIndex_);
        }

        /// Return hash value for HashSet & HashMap.
        unsigned ToHash() const
        {
            unsigned result = 0;
            CombineHash(result, nodeId_);
            CombineHash(result, pinIndex_);
            return result;
        }

    };
}

GraphPinView::GraphPinView(ax::NodeEditor::PinId id): id_(id)
{}

void GraphView::Reset()
{
    nextUniqueId_ = 1;
    nodes_.clear();
    links_.clear();
}

void GraphView::Populate(Graph* graph)
{
    Reset();
    bool needLayouting = true;

    ea::vector<unsigned> nodeIds;
    graph->GetNodeIds(nodeIds);
    ea::unordered_map<StablePinKey, ed::PinId> enterPins;
    ea::unordered_map<StablePinKey, ed::PinId> exitPins;
    ea::unordered_map<StablePinKey, ed::PinId> inputPins;
    ea::unordered_map<StablePinKey, ed::PinId> outputPins;
    nextUniqueId_ = graph->GetNextNodeId();
    for (unsigned i : nodeIds)
    {
        auto* node = graph->GetNode(i);
        auto& nodeView = nodes_.emplace_back();
        nodeView.id_ = node->GetID();
        nodeView.title_ = node->GetName();
        nodeView.position_ = node->GetPositionHint();
        needLayouting &= nodeView.position_ == Vector2::ZERO;

        for (unsigned pinIndex = 0; pinIndex < node->GetNumEnters(); ++pinIndex)
        {
            auto pin = node->GetEnter(pinIndex);
            enterPins[StablePinKey(pin)] = nextUniqueId_;
            auto& pinView = nodeView.enterPins_.emplace_back(nextUniqueId_++);
            pinView.title_ = pin.GetPin()->GetName();
        }

        for (unsigned pinIndex = 0; pinIndex < node->GetNumInputs(); ++pinIndex)
        {
            auto pinRef = node->GetInput(pinIndex);
            inputPins[StablePinKey(pinRef)] = nextUniqueId_;
            auto& pinView = nodeView.inputPins_.emplace_back(nextUniqueId_++);
            auto* pin = pinRef.GetPin();
            pinView.title_ = pin->GetName();
            pinView.value_ = pin->GetValue().ToString();
            pinView.type_ = pin->GetValue().GetType();
        }

        for (unsigned pinIndex = 0; pinIndex < node->GetNumExits(); ++pinIndex)
        {
            auto pin = node->GetExit(pinIndex);
            exitPins[StablePinKey(pin)] = nextUniqueId_;
            auto& pinView = nodeView.exitPins_.emplace_back(nextUniqueId_++);
            pinView.title_ = pin.GetPin()->GetName();
        }

        for (unsigned pinIndex = 0; pinIndex < node->GetNumOutputs(); ++pinIndex)
        {
            auto pin = node->GetOutput(pinIndex);
            outputPins[StablePinKey(pin)] = nextUniqueId_;
            auto& pinView = nodeView.inputPins_.emplace_back(nextUniqueId_++);
            pinView.title_ = pin.GetPin()->GetName();
        }
    }

    for (unsigned i : nodeIds)
    {
        auto* node = graph->GetNode(i);
        for (unsigned pinIndex = 0; pinIndex < node->GetNumInputs(); ++pinIndex)
        {
            auto pinRef = node->GetInput(pinIndex);
            auto connectedPinRef = pinRef.GetConnectedPin<GraphOutPin>();
            if (connectedPinRef.GetNode())
            {
                links_[nextUniqueId_++] = GraphLinkView{outputPins[pinRef], inputPins[connectedPinRef]};
            }
        }
        for (unsigned pinIndex = 0; pinIndex < node->GetNumExits(); ++pinIndex)
        {
            auto pinRef = node->GetExit(pinIndex);
            auto connectedPinRef = pinRef.GetConnectedPin<GraphEnterPin>();
            if (connectedPinRef.GetNode())
            {
                links_[nextUniqueId_++] = GraphLinkView{exitPins[pinRef], enterPins[connectedPinRef]};
            }
        }

    }

    if (needLayouting)
    {
        int x = 0;
        for (auto& node: nodes_)
        {
            node.position_ = Vector2(x, 0);
            node.size_ = Vector2(100, 100);
            x += 100;
        }
        AutoLayout();
    }
}

void GraphView::AutoLayout()
{
    // TODO: perform layouting
}

GraphViewTab::GraphViewTab(Context* context, const ea::string& title, const ea::string& guid,
    EditorTabFlags flags, EditorTabPlacement placement)
    : ResourceEditorTab(context, title, guid, flags, placement)
{
    ed::Config config;
    config.SettingsFile = nullptr;
    editorContext_ = ed::CreateEditor(&config);
}

GraphViewTab::~GraphViewTab()
{
    ed::DestroyEditor(editorContext_);
}


void GraphViewTab::Reset()
{
    applyLayout_ = true;
    graph_.Reset();
}

void GraphViewTab::RenderGraph()
{
    for (auto& node : graph_.nodes_)
    {
        if (applyLayout_)
            ed::SetNodePosition(node.id_, ImVec2(node.position_.x_, node.position_.y_));
        ed::BeginNode(node.id_);
        ImGui::Text("%s", node.title_.c_str());

        ImGui::BeginGroup();
        for (auto& pin : node.enterPins_)
        {
            ed::BeginPin(pin.id_, ax::NodeEditor::PinKind::Input);
            ax::Widgets::Icon(ImVec2(16, 16), ax::Drawing::IconType::Flow, true);
            ImGui::SameLine();
            ImGui::Text("%s", pin.title_.c_str());
            ed::EndPin();
        }
        for (auto& pin : node.inputPins_)
        {
            ed::BeginPin(pin.id_, ax::NodeEditor::PinKind::Input);
            if (pin.type_ != VAR_NONE)
            {
                ImGui::PushItemWidth(100.0f);
                ImGui::PushID(pin.id_.Get());
                ImGui::InputText("##edit", &pin.value_);
                ImGui::PopID();
                ImGui::PopItemWidth();
                ImGui::SameLine();
            }
            ImGui::Text("%s", pin.title_.c_str());
            ed::EndPin();
        }
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        for (auto& pin : node.exitPins_)
        {
            ed::BeginPin(pin.id_, ax::NodeEditor::PinKind::Output);
            ImGui::Text("%s", pin.title_.c_str());
            ImGui::SameLine();
            ax::Widgets::Icon(ImVec2(16, 16), ax::Drawing::IconType::Flow, true);
            ed::EndPin();
        }
        for (auto& pin : node.outputPins_)
        {
            ed::BeginPin(pin.id_, ax::NodeEditor::PinKind::Output);
            ImGui::Text("%s", pin.title_.c_str());
            ed::EndPin();
        }
        ImGui::EndGroup();

        ed::EndNode();

        auto newPos = ToVector2(ed::GetNodePosition(node.id_));
        if (node.position_ != newPos)
        {
            node.position_ = newPos;
            //TODO: add to Undo stack
        }
        node.size_ = ToVector2(ed::GetNodeSize(node.id_));
    }

    for (auto& link : graph_.links_)
    {
        ed::Link(link.first, link.second.from_, link.second.to_);
    }

    //if (ed::BeginCreate())
    //{
    //    ed::PinId inputPinId, outputPinId;
    //    if (ed::QueryNewLink(&inputPinId, &outputPinId))
    //    {
    //        ed::RejectNewItem();
    //    }

    //    ed::PinId newNodeId;
    //    if (ed::QueryNewNode(&newNodeId))
    //    {
    //        ed::RejectNewItem();
    //    }
    //}

    //if (ed::BeginDelete())
    //{
    //    ed::LinkId deletedLinkId;
    //    while (ed::QueryDeletedLink(&deletedLinkId))
    //    {
    //        ed::RejectDeletedItem();
    //    }

    //    ed::NodeId deletedNodeId;
    //    while (ed::QueryDeletedNode(&deletedNodeId))
    //    {
    //        ed::RejectDeletedItem();
    //    }
    //}
}

void GraphViewTab::RenderTitle()
{
    ui::Text("%s", GetActiveResourceName().c_str());
}

void GraphViewTab::RenderContent()
{
    const ImVec2 basePosition = ui::GetCursorPos();

    RenderTitle();

   
    ed::SetCurrentEditor(editorContext_);

    if (ImGui::Button("Zoom to Content"))
        ed::NavigateToContent();
    ImGui::SameLine();
    if (ImGui::Button("Autolayout"))
        graph_.AutoLayout();
    //if (ImGui::Button("Save"))
    //    Save();
    ImGui::SameLine();
    ImGui::Checkbox("Show Ordinals", &showOrdinals_);

    const ImVec2 contentPosition = ui::GetCursorPos();
    const auto contentSize = GetContentSize() - IntVector2(0, contentPosition.y - basePosition.y);
    const auto imContentSize = ToImGui(VectorMax(contentSize, IntVector2::ONE));

    // Start interaction with editor.
    ed::Begin("graph_view", imContentSize);

    RenderGraph();

    // End of interaction with editor.
    ed::End();

    auto editorMin = ImGui::GetItemRectMin();
    auto editorMax = ImGui::GetItemRectMax();

    if (showOrdinals_)
    {
        int nodeCount = ed::GetNodeCount();
        orderedNodeIds_.resize(static_cast<size_t>(nodeCount));
        ed::GetOrderedNodeIds(orderedNodeIds_.data(), nodeCount);

        auto drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(editorMin, editorMax);

        int ordinal = 0;
        for (auto& nodeId : orderedNodeIds_)
        {
            auto p0 = ed::GetNodePosition(nodeId);
            auto p1 = p0 + ed::GetNodeSize(nodeId);
            p0 = ed::CanvasToScreen(p0);
            p1 = ed::CanvasToScreen(p1);

            ImGuiTextBuffer builder;
            builder.appendf("#%d", ordinal++);

            auto textSize = ImGui::CalcTextSize(builder.c_str());
            auto padding = ImVec2(2.0f, 2.0f);
            auto widgetSize = textSize + padding * 2;

            auto widgetPosition = ImVec2(p1.x, p0.y) + ImVec2(0.0f, -widgetSize.y);

            drawList->AddRectFilled(widgetPosition, widgetPosition + widgetSize, IM_COL32(100, 80, 80, 190), 3.0f,
                ImDrawFlags_RoundCornersAll);
            drawList->AddRect(widgetPosition, widgetPosition + widgetSize, IM_COL32(200, 160, 160, 190), 3.0f,
                ImDrawFlags_RoundCornersAll);
            drawList->AddText(widgetPosition + padding, IM_COL32(255, 255, 255, 255), builder.c_str());
        }

        drawList->PopClipRect();
    }


    ed::SetCurrentEditor(nullptr);

    applyLayout_ = false;
}

} // namespace Urho3D
