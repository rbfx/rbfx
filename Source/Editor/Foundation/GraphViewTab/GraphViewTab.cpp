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
#include "../../Foundation/GraphViewTab/GraphViewTab.h"

#include "CreateLink.h"
#include "CreateNode.h"
#include "DeleteLink.h"
#include "DeleteNode.h"
#include "MoveNodes.h"
#include "UpdatePinValue.h"

#include <Urho3D/Resource/GraphNode.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <ImGuiNodeEditor/utilities/widgets.h>

namespace ed = ax::NodeEditor;

namespace Urho3D
{
namespace
{
    enum class PopupMenuType
    {
        None,
        Background
    };
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

namespace Detail
{
GraphPinView::GraphPinView(ax::NodeEditor::PinId id, const ea::string& title)
    : id_(id)
    , title_(title)
    , link_(ed::LinkId::Invalid)
{
}

GraphPinView::GraphPinView(ax::NodeEditor::PinId id, const ea::string& title, VariantType type, const Variant& value)
    : id_(id)
    , title_(title)
    , type_(type)
    , value_(value)
    , text_(value.ToString())
    , link_(ed::LinkId::Invalid)
{
}

GraphNodeView::GraphNodeView(ax::NodeEditor::NodeId id, const ea::string& title)
    : id_(id)
    , title_(title)
{
}

GraphPinView* GraphNodeView::GetPinView(const Urho3D::Detail::PinNodeViewRef& pinRef)
{
    switch (pinRef.type_)
    {
    case GraphPinViewType::Output:
            return (pinRef.index_ < outputPins_.size()) ? &outputPins_[pinRef.index_]
                                                        : static_cast<GraphPinView*>(nullptr);
    case GraphPinViewType::Input:
            return (pinRef.index_ < inputPins_.size()) ? &inputPins_[pinRef.index_]
                                                        : static_cast<GraphPinView*>(nullptr);
    case GraphPinViewType::Enter:
            return (pinRef.index_ < enterPins_.size()) ? &enterPins_[pinRef.index_]
                                                        : static_cast<GraphPinView*>(nullptr);
    case GraphPinViewType::Exit:
            return (pinRef.index_ < exitPins_.size()) ? &exitPins_[pinRef.index_]
                                                        : static_cast<GraphPinView*>(nullptr);
    }
    return nullptr;
}

GraphPinView* GraphView::GetInputPinView(ax::NodeEditor::NodeId node, const ea::string& pinName)
{
    auto it = nodes_.find(node);
    if (it != nodes_.end())
    {
        for (auto& pin : it->second.inputPins_)
        {
            if (pin.title_ == pinName)
                return &pin;
        }
    }
    return nullptr;
}

GraphPinView* GraphView::GetInputPinView(ax::NodeEditor::NodeId node, unsigned pinIndex)
{
    auto it = nodes_.find(node);
    if (it != nodes_.end())
    {
        if (pinIndex < it->second.inputPins_.size())
            return &it->second.inputPins_[pinIndex];
    }
    return nullptr;
}

GraphNodeView* GraphView::GetNode(ax::NodeEditor::NodeId id)
{
    auto nodeIt = nodes_.find(id);
    if (nodeIt != nodes_.end())
        return &nodeIt->second;
    return nullptr;
}

void GraphView::Reset()
{
    nextUniqueId_ = 1;
    nodes_.clear();
    links_.clear();
}

void GraphView::AddLink(ax::NodeEditor::LinkId id, GraphPinView& from, GraphPinView& to)
{
    links_[id] = {from.id_, to.id_};
    from.link_ = id;
    to.link_ = id;
}

void GraphView::AddLink(ax::NodeEditor::LinkId linkId, ax::NodeEditor::PinId from, ax::NodeEditor::PinId to)
{
    auto fromIt = pinToNode_.find(from);
    auto toIt = pinToNode_.find(to);
    if (fromIt != pinToNode_.end() && toIt != pinToNode_.end())
    {
        switch (fromIt->second.type_)
        {
        case GraphPinViewType::Enter: return AddLink(linkId, to, from);
        case GraphPinViewType::Input: return AddLink(linkId, to, from);
        case GraphPinViewType::Output:
        {
            auto* fromNode = GetNode(fromIt->second.node_);
            auto* toNode =  GetNode(toIt->second.node_);
            return AddLink(linkId, fromNode->outputPins_[fromIt->second.index_], toNode->inputPins_[toIt->second.index_]);
        }
        case GraphPinViewType::Exit:
        {
            auto* fromNode = GetNode(fromIt->second.node_);
            auto* toNode = GetNode(toIt->second.node_);
            return AddLink(linkId, fromNode->exitPins_[fromIt->second.index_], toNode->enterPins_[toIt->second.index_]);
        }
        }
    }
}

void GraphView::Populate(Graph* graph)
{
    Reset();
    bool needLayouting = true;

    ea::unordered_map<GraphNode*, ed::NodeId> nodeMap;

    ea::vector<unsigned> nodeIds;
    graph->GetNodeIds(nodeIds);

    for (unsigned i : nodeIds)
    {
        auto* node = graph->GetNode(i);
        auto nodeId = AddNode(node);
        nodeMap[node] = nodeId;
        auto* nodeView = GetNode(nodeId);
        needLayouting &= nodeView->position_ == Vector2::ZERO;
    }

    for (unsigned i : nodeIds)
    {
        auto* node = graph->GetNode(i);
        auto nodeId = nodeMap[node];
        auto* nodeView = GetNode(nodeId);
        for (unsigned pinIndex = 0; pinIndex < node->GetNumInputs(); ++pinIndex)
        {
            auto pinRef = node->GetInput(pinIndex);
            auto connectedPinRef = pinRef.GetConnectedPin<GraphOutPin>();
            if (auto connectedNode = connectedPinRef.GetNode())
            {
                auto connectedNodeId = nodeMap[connectedNode];
                auto* connectedNodeView = GetNode(connectedNodeId);
                const auto connectedPinIndex = connectedNode->GetPinIndex(connectedPinRef.GetPin());
                AddLink(nextUniqueId_++, connectedNodeView->outputPins_[connectedPinIndex], nodeView->inputPins_[pinIndex]);
            }
        }
        for (unsigned pinIndex = 0; pinIndex < node->GetNumExits(); ++pinIndex)
        {
            auto pinRef = node->GetExit(pinIndex);
            auto connectedPinRef = pinRef.GetConnectedPin<GraphEnterPin>();
            if (auto connectedNode = connectedPinRef.GetNode())
            {
                auto connectedNodeId = nodeMap[connectedNode];
                auto* connectedNodeView = GetNode(connectedNodeId);
                const auto connectedPinIndex = connectedNode->GetPinIndex(connectedPinRef.GetPin());
                AddLink(nextUniqueId_++, nodeView->exitPins_[pinIndex], connectedNodeView->enterPins_[connectedPinIndex]);
            }
        }
    }

    if (needLayouting)
    {
        int x = 0;
        for (auto& node: nodes_)
        {
            node.second.position_ = Vector2(x, 0);
            node.second.size_ = Vector2(100, 100);
            x += 100;
        }
        AutoLayout();
    }
}

SharedPtr<Graph> GraphView::BuildGraph(Context* context)
{
    auto graph = MakeShared<Graph>(context);
    ea::unordered_map<ax::NodeEditor::NodeId, GraphNode*> nodeMap;
    ea::unordered_map<ax::NodeEditor::PinId, ea::tuple<GraphNode*, unsigned>> pinMap;
    for (auto& nodeKeyValue: nodes_)
    {
        auto node = MakeShared<GraphNode>(context);
        graph->Add(node);
        nodeMap[nodeKeyValue.first] = node.Get();
        node->SetName(nodeKeyValue.second.title_);
        node->SetPositionHint(nodeKeyValue.second.position_);

        unsigned pinIndex = 0;
        for (auto enterPin : nodeKeyValue.second.enterPins_)
        {
            node->WithEnter(enterPin.title_);
            pinMap[enterPin.id_] = ea::make_tuple(node.Get(), pinIndex);
            ++pinIndex;
        }
        for (auto inputPin : nodeKeyValue.second.inputPins_)
        {
            node->WithInput(inputPin.title_, inputPin.value_);
        }
        pinIndex = 0;
        for (auto exitPin : nodeKeyValue.second.exitPins_)
        {
            node->WithExit(exitPin.title_);
            pinMap[exitPin.id_] = ea::make_tuple(node.Get(), pinIndex);
            ++pinIndex;
        }
        for (auto outputPin : nodeKeyValue.second.outputPins_)
        {
            node->WithOutput(outputPin.title_, outputPin.type_);
        }
    }

    for (auto& linkKeyValue : links_)
    {
        const auto from = pinMap.find(linkKeyValue.second.from_);
        const auto to = pinMap.find(linkKeyValue.second.to_);
        if (from != pinMap.end() && to != pinMap.end())
        {
            auto* fromNode = ea::get<0>(from->second);
            unsigned fromPinIndex = ea::get<1>(from->second);
            auto* toNode = ea::get<0>(to->second);
            unsigned toIndex = ea::get<1>(to->second);

            fromNode->GetExit(fromPinIndex).GetPin()->ConnectTo(toNode->GetEnter(toIndex));
        }
    }

    return graph;
}

bool GraphView::AddNode(const GraphNodeView& nodeView)
{
    auto id = nodeView.id_;
    if (!id)
    {
        id = nextUniqueId_++;
    }
    auto nodeIt = nodes_.find(id);
    if (nodeIt == nodes_.end())
    {
        nodes_.insert({id, nodeView});
        return true;
    }
    return false;
}

ax::NodeEditor::NodeId GraphView::AddNode(GraphNode* node)
{
    auto nodeId = ax::NodeEditor::NodeId(nextUniqueId_++);
    GraphNodeView nodeView{nodeId, node->GetName()};
    nodeView.position_ = node->GetPositionHint();

    for (unsigned pinIndex = 0; pinIndex < node->GetNumEnters(); ++pinIndex)
    {
        auto pin = node->GetEnter(pinIndex);
        auto id = ed::PinId(nextUniqueId_++);
        pinToNode_[id] = {nodeId, GraphPinViewType::Enter, pinIndex};
        auto& pinView = nodeView.enterPins_.emplace_back(id, pin.GetPin()->GetName());
    }

    for (unsigned pinIndex = 0; pinIndex < node->GetNumInputs(); ++pinIndex)
    {
        auto pinRef = node->GetInput(pinIndex);
        auto* pin = pinRef.GetPin();
        auto id = ed::PinId(nextUniqueId_++);
        pinToNode_[id] = {nodeId, GraphPinViewType::Input, pinIndex};
        auto& pinView = nodeView.inputPins_.emplace_back(
            id, pin->GetName(), pin->GetValue().GetType(), pin->GetValue());
    }

    for (unsigned pinIndex = 0; pinIndex < node->GetNumExits(); ++pinIndex)
    {
        auto pin = node->GetExit(pinIndex);
        auto id = ed::PinId(nextUniqueId_++);
        pinToNode_[id] = {nodeId, GraphPinViewType::Exit, pinIndex};
        auto& pinView = nodeView.exitPins_.emplace_back(id, pin.GetPin()->GetName());
    }

    for (unsigned pinIndex = 0; pinIndex < node->GetNumOutputs(); ++pinIndex)
    {
        auto pin = node->GetOutput(pinIndex);
        auto id = ed::PinId(nextUniqueId_++);
        pinToNode_[id] = {nodeId, GraphPinViewType::Output, pinIndex};
        auto& pinView = nodeView.outputPins_.emplace_back(
            id, pin.GetPin()->GetName(), pin.GetPin()->GetType(), Variant(pin.GetPin()->GetType()));
    }
    AddNode(nodeView);
    return nodeId;
}

void GraphView::AutoLayout()
{
    // TODO: perform layouting
}

void GraphView::DeleteLink(const ax::NodeEditor::LinkId& linkId)
{
    auto linkIt = links_.find(linkId);
    if (linkIt != links_.end())
    {
        auto fromIt = pinToNode_.find(linkIt->second.from_);
        auto toIt = pinToNode_.find(linkIt->second.to_);
        if (fromIt != pinToNode_.end() && toIt != pinToNode_.end())
        {
            auto* fromNode = GetNode(fromIt->second.node_);
            auto* toNode = GetNode(toIt->second.node_);

            fromNode->GetPinView(fromIt->second)->link_ = ed::LinkId::Invalid;
            toNode->GetPinView(toIt->second)->link_ = ed::LinkId::Invalid;
            links_.erase(linkId);
        }
    }
}

void GraphView::DeleteNode(const ax::NodeEditor::NodeId& id)
{
    nodes_.erase(id);
}

} // namespace Detail

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

SharedPtr<GraphNode> GraphViewTab::CreateNewNodePopup() const
{
    return {};
}

void GraphViewTab::RenderGraph()
{
    SharedPtr<MoveNodesAction> moveNodesAction;
    for (auto& nodeKeyValue : graph_.nodes_)
    {
        auto nodeId = nodeKeyValue.first;
        auto& node = nodeKeyValue.second;
        if (applyLayout_)
            ed::SetNodePosition(nodeId, ImVec2(node.position_.x_, node.position_.y_));
        ed::BeginNode(nodeId);
        ImGui::Text("%s", node.title_.c_str());

        ImGui::BeginGroup();
        for (auto& pin : node.enterPins_)
        {
            ed::BeginPin(pin.id_, ax::NodeEditor::PinKind::Input);
            ax::Widgets::Icon(ImVec2(16, 16), ax::Drawing::IconType::Flow, !!pin.link_);
            ImGui::SameLine();
            ImGui::Text("%s", pin.title_.c_str());
            ed::EndPin();
        }
        for (auto& pin : node.inputPins_)
        {
            ed::BeginPin(pin.id_, ax::NodeEditor::PinKind::Input);
            ax::Widgets::Icon(ImVec2(16, 16), ax::Drawing::IconType::Circle, !!pin.link_);
            ImGui::SameLine();
            if (pin.type_ == VAR_NONE)
            {
            }
            if (pin.value_.GetType() != VAR_NONE)
            {
                ImGui::PushItemWidth(100.0f);
                ImGui::PushID(pin.id_.Get());
                if (ImGui::InputText("##edit", &pin.text_))
                {
                    Variant value;
                    value.FromString(pin.value_.GetType(), pin.text_);

                    PushAction(MakeShared<UpdatePinValueAction>(this, nodeId, pin.id_, pin.value_,value));
                    pin.value_ = value;
                }
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
            ax::Widgets::Icon(ImVec2(16, 16), ax::Drawing::IconType::Flow, !!pin.link_);
            ed::EndPin();
        }
        for (auto& pin : node.outputPins_)
        {
            ed::BeginPin(pin.id_, ax::NodeEditor::PinKind::Output);
            ImGui::Text("%s", pin.title_.c_str());
            ImGui::SameLine();
            ax::Widgets::Icon(ImVec2(16, 16), ax::Drawing::IconType::Circle, !!pin.link_);
            ed::EndPin();
        }
        ImGui::EndGroup();

        ed::EndNode();

        auto newPos = ToVector2(ed::GetNodePosition(nodeId));
        if (node.position_ != newPos)
        {
            if (!moveNodesAction)
                moveNodesAction = MakeShared<MoveNodesAction>(this);
            moveNodesAction->Add(nodeKeyValue.first, node.position_, newPos);
            node.position_ = newPos;
        }
        node.size_ = ToVector2(ed::GetNodeSize(nodeId));
    }

    for (auto& link : graph_.links_)
    {
        ed::Link(link.first, link.second.from_, link.second.to_);
    }

    if (moveNodesAction)
    {
        PushAction(moveNodesAction);
    }

    if (ed::BeginCreate())
    {
        ed::PinId inputPinId, outputPinId;
        if (ed::QueryNewLink(&inputPinId, &outputPinId))
        {
            if (inputPinId && outputPinId)
            {
                if (ed::AcceptNewItem())
                {
                    CreateLink(inputPinId, outputPinId);
                }
            }
            //ed::RejectNewItem();
        }

        ed::PinId newNodeId;
        if (ed::QueryNewNode(&newNodeId))
        {
            ed::RejectNewItem();
        }
    }
    ed::EndCreate();

    if (ed::BeginDelete())
    {
        ed::LinkId deletedLinkId;
        while (ed::QueryDeletedLink(&deletedLinkId))
        {
            DeleteLink(deletedLinkId);
        }

        nodesToDelete_.clear();
        ed::NodeId deletedNodeId;
        while (ed::QueryDeletedNode(&deletedNodeId))
        {
            nodesToDelete_.push_back(deletedNodeId);
        }

        for (auto& nodeToDelete: nodesToDelete_)
        {
            if (auto* node = graph_.GetNode(nodeToDelete))
            {
                for (auto& pin : node->inputPins_)
                    DeleteLink(pin.link_);
                for (auto& pin : node->outputPins_)
                    DeleteLink(pin.link_);
                for (auto& pin : node->enterPins_)
                    DeleteLink(pin.link_);
                for (auto& pin : node->exitPins_)
                    DeleteLink(pin.link_);
            }
        }

        for (auto& nodeToDelete : nodesToDelete_)
        {
            auto action = MakeShared<DeleteNodeAction>(this, graph_.GetNode(nodeToDelete));
            action->Redo();
            PushAction(action);
        }
    }
    ed::EndDelete();
}

void GraphViewTab::ApplyLayoutFromView()
{
    applyLayout_ = true;
}

void GraphViewTab::RenderTitle()
{
    ui::Text("%s", GetActiveResourceName().c_str());
}

void GraphViewTab::DeleteLink(const ax::NodeEditor::LinkId& deletedLinkId)
{
    if (!deletedLinkId)
        return;
    if (graph_.links_.find(deletedLinkId) != graph_.links_.end())
    {
        auto action = MakeShared<DeleteLinkAction>(this, deletedLinkId);
        action->Redo();
        PushAction(action);
    }
}

void GraphViewTab::CreateLink(const ax::NodeEditor::PinId& from, const ax::NodeEditor::PinId& to)
{
    auto fromIt = graph_.pinToNode_.find(from);
    auto toIt = graph_.pinToNode_.find(to);
    if (fromIt == graph_.pinToNode_.end() || toIt == graph_.pinToNode_.end())
        return;
    if (fromIt->second.type_ == Detail::GraphPinViewType::Input
        || fromIt->second.type_ == Detail::GraphPinViewType::Enter)
    {
        if (toIt->second.type_ == Detail::GraphPinViewType::Input
            || toIt->second.type_ == Detail::GraphPinViewType::Enter)
            return;
        CreateLink(to, from);
        return;
    }

    auto fromRef = fromIt->second;
    auto toRef = toIt->second;

    if (fromRef.node_ == toRef.node_)
        return;

    auto* fromView = graph_.GetNode(fromRef.node_)->GetPinView(fromRef);
    auto* toView = graph_.GetNode(toRef.node_)->GetPinView(fromRef);

    if (fromRef.type_ == Detail::GraphPinViewType::Output)
    {
        if (toRef.type_ != Detail::GraphPinViewType::Input)
            return;
        if (fromView->type_ != toView->type_)
            return;
        if (toView->link_)
            DeleteLink(toView->link_);
    }
    else if (fromRef.type_ == Detail::GraphPinViewType::Exit)
    {
        if (toRef.type_ != Detail::GraphPinViewType::Enter)
            return;
        if (fromView->link_)
            DeleteLink(fromView->link_);
    }
    else
    {
        return;
    }

    auto action = MakeShared<CreateLinkAction>(this, from, to);
    action->Redo();
    PushAction(action);
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
    ImGui::SameLine();
    ImGui::Checkbox("Show Ordinals", &showOrdinals_);

    const ImVec2 contentPosition = ui::GetCursorPos();
    const auto contentSize = GetContentSize() - IntVector2(0, contentPosition.y - basePosition.y);
    const auto imContentSize = ToImGui(VectorMax(contentSize, IntVector2::ONE));

    ImGui::BeginChild("graph_panel", imContentSize);

    // Start interaction with editor.
    ed::Begin("graph_view", imContentSize);

    RenderGraph();

    auto popupMenuType = PopupMenuType::None;
    auto openPopupPosition = ImGui::GetMousePos();

    if (ed::ShowBackgroundContextMenu())
        popupMenuType = PopupMenuType::Background;

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

    switch (popupMenuType)
    {
    case PopupMenuType::Background: ImGui::OpenPopup("Create New Node"); break;
    }

    if (ImGui::BeginPopup("Create New Node"))
    {
        auto newNodePosition = openPopupPosition;

        auto node = CreateNewNodePopup();
        if (node)
        {
            auto nodeId = graph_.AddNode(node);
            auto* nodeView = graph_.GetNode(nodeId);
            nodeView->position_ = Vector2(newNodePosition.x, newNodePosition.y);
            PushAction(MakeShared<CreateNodeAction>(this, nodeView));
            applyLayout_ = true;
        }

        ImGui::EndPopup();
    }


    ImGui::EndChild();
}

} // namespace Urho3D
