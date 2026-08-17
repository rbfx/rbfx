// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BlueprintTab.h"

#include "../Core/IniHelpers.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Blueprint/BlueprintResource.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace Urho3D
{

namespace
{

constexpr float NodeWidth = 240.0f;
constexpr float HeaderHeight = 32.0f;
constexpr float PinHeight = 21.0f;
constexpr float PinRadius = 5.0f;

bool IsOutputPin(BlueprintPinKind kind)
{
    return kind == BlueprintPinKind::Output || kind == BlueprintPinKind::ExecutionOutput;
}

bool IsExecutionPin(BlueprintPinKind kind)
{
    return kind == BlueprintPinKind::ExecutionInput || kind == BlueprintPinKind::ExecutionOutput;
}

ImU32 PinColor(BlueprintDataType type, bool execution)
{
    if (execution)
        return IM_COL32(245, 205, 80, 255);
    switch (type)
    {
    case BlueprintDataType::Bool: return IM_COL32(90, 205, 120, 255);
    case BlueprintDataType::Int:
    case BlueprintDataType::Int64:
    case BlueprintDataType::Float:
    case BlueprintDataType::Double: return IM_COL32(90, 165, 240, 255);
    case BlueprintDataType::String: return IM_COL32(230, 150, 90, 255);
    case BlueprintDataType::Vector2:
    case BlueprintDataType::Vector3:
    case BlueprintDataType::Vector4: return IM_COL32(180, 110, 230, 255);
    default: return IM_COL32(185, 185, 195, 255);
    }
}

const auto Hotkey_Cut = EditorHotkey{"Blueprint.Cut"}.Ctrl().Press(KEY_X);
const auto Hotkey_Copy = EditorHotkey{"Blueprint.Copy"}.Ctrl().Press(KEY_C);
const auto Hotkey_Paste = EditorHotkey{"Blueprint.Paste"}.Ctrl().Press(KEY_V);
const auto Hotkey_Delete = EditorHotkey{"Blueprint.Delete"}.Press(KEY_DELETE);
const auto Hotkey_Duplicate = EditorHotkey{"Blueprint.Duplicate"}.Ctrl().Press(KEY_D);
const auto Hotkey_Breakpoint = EditorHotkey{"Blueprint.Breakpoint"}.Press(KEY_F9);

class BlueprintGraphSnapshotAction final : public EditorAction
{
public:
    BlueprintGraphSnapshotAction(BlueprintTab* tab, const ea::string& before, const ea::string& after)
        : tab_(tab), before_(before), after_(after)
    {
    }

    void Redo() const override
    {
        if (tab_)
            tab_->ApplyGraphSnapshot(after_);
    }

    void Undo() const override
    {
        if (tab_)
            tab_->ApplyGraphSnapshot(before_);
    }

private:
    WeakPtr<BlueprintTab> tab_;
    ea::string before_;
    ea::string after_;
};

bool IsInputPin(BlueprintPinKind kind)
{
    return kind == BlueprintPinKind::Input || kind == BlueprintPinKind::ExecutionInput;
}

}

void Foundation_BlueprintTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<BlueprintTab>(context));
}

BlueprintTab::BlueprintTab(Context* context)
    : EditorTab(context, "Blueprint", "7fef95f7-42da-4ad1-9a32-3a7e9c1cc101",
          EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
    , graph_("Main")
{
    BindHotkey(Hotkey_Cut, &BlueprintTab::CutSelection);
    BindHotkey(Hotkey_Copy, &BlueprintTab::CopySelection);
    BindHotkey(Hotkey_Paste, &BlueprintTab::PasteSelection);
    BindHotkey(Hotkey_Delete, &BlueprintTab::DeleteSelected);
    BindHotkey(Hotkey_Duplicate, &BlueprintTab::DuplicateSelection);
    BindHotkey(Hotkey_Breakpoint, &BlueprintTab::ToggleBreakpoint);

    runtime_.RegisterReflectedNodes(context);
    CreateDemoGraph();
    graphFileName_ = GetGraphFileName();
}

void BlueprintTab::CreateDemoGraph()
{
    graph_.Clear();

    const BlueprintId eventNode = graph_.AddNode("Event.OnStart", "On Start", {-40.0f, 80.0f}, BlueprintExecutionMode::Immediate);
    const BlueprintId printNode = graph_.AddNode("Flow.Print", "Print", {300.0f, 80.0f}, BlueprintExecutionMode::Immediate);

    if (BlueprintNode* event = graph_.GetNode(eventNode))
        AddPin(*event, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);

    if (BlueprintNode* print = graph_.GetNode(printNode))
    {
        AddPin(*print, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
        AddPin(*print, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
        AddPin(*print, "message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("Blueprint ready")));
    }

    graph_.AddLink(eventNode, "then", printNode, "execute");
    selectedNode_ = eventNode;
    graphDirty_ = false;
    status_ = "Demo graph ready";
}

void BlueprintTab::AddPin(BlueprintNode& node, const ea::string& name, BlueprintPinKind kind,
    BlueprintDataType dataType, const Variant& defaultValue)
{
    BlueprintPin pin;
    pin.name = name;
    pin.displayName = name;
    pin.kind = kind;
    pin.dataType = dataType;
    pin.defaultValue = defaultValue;
    node.pins.push_back(pin);
}

BlueprintPin* BlueprintTab::FindPinAt(const Vector2& graphPosition, BlueprintNode*& node)
{
    for (auto nodeIter = graph_.GetNodes().rbegin(); nodeIter != graph_.GetNodes().rend(); ++nodeIter)
    {
        BlueprintNode* candidate = graph_.GetNode(nodeIter->id);
        if (!candidate)
            continue;
        for (unsigned i = 0; i < candidate->pins.size(); ++i)
        {
            const BlueprintPin& pin = candidate->pins[i];
            const float pinX = IsOutputPin(pin.kind) ? NodeWidth : 0.0f;
            const float dx = graphPosition.x_ - (candidate->position.x_ + pinX);
            const float dy = graphPosition.y_ - (candidate->position.y_ + GetPinY(*candidate, i));
            if (dx * dx + dy * dy <= 12.0f * 12.0f)
            {
                node = candidate;
                return &candidate->pins[i];
            }
        }
    }
    node = nullptr;
    return nullptr;
}

void BlueprintTab::BeginGraphEdit()
{
    if (graphEditSnapshot_.empty())
        graphEditSnapshot_ = graph_.ToString();
}

void BlueprintTab::CommitGraphEdit(const ea::string& status)
{
    if (graphEditSnapshot_.empty())
        return;

    const ea::string after = graph_.ToString();
    if (after != graphEditSnapshot_)
        PushAction<BlueprintGraphSnapshotAction>(this, graphEditSnapshot_, after);
    graphEditSnapshot_.clear();
    graphDirty_ = true;
    if (!status.empty())
        status_ = status;
}

void BlueprintTab::ApplyGraphSnapshot(const ea::string& snapshot)
{
    ea::string error;
    if (!graph_.FromString(snapshot, &error))
    {
        status_ = Format("Unable to restore Blueprint snapshot: {}", error);
        return;
    }

    selectedNodes_.clear();
    selectedNode_ = BLUEPRINT_INVALID_ID;
    draggingNode_ = BLUEPRINT_INVALID_ID;
    linkingNode_ = BLUEPRINT_INVALID_ID;
    graphDirty_ = true;
}

bool BlueprintTab::IsSelected(BlueprintId nodeId) const
{
    for (const BlueprintId selected : selectedNodes_)
    {
        if (selected == nodeId)
            return true;
    }
    return false;
}

void BlueprintTab::SelectNode(BlueprintId nodeId, bool toggle)
{
    if (!toggle)
        selectedNodes_.clear();

    if (nodeId == BLUEPRINT_INVALID_ID)
    {
        selectedNode_ = BLUEPRINT_INVALID_ID;
        return;
    }

    const bool alreadySelected = IsSelected(nodeId);
    if (toggle && alreadySelected)
    {
        for (auto iter = selectedNodes_.begin(); iter != selectedNodes_.end(); ++iter)
        {
            if (*iter == nodeId)
            {
                selectedNodes_.erase(iter);
                break;
            }
        }
    }
    else if (!alreadySelected)
        selectedNodes_.push_back(nodeId);
    selectedNode_ = selectedNodes_.empty() ? BLUEPRINT_INVALID_ID : nodeId;
}

ea::string BlueprintTab::SerializeSelection() const
{
    BlueprintGraph selection("BlueprintClipboard");
    for (const BlueprintId nodeId : selectedNodes_)
    {
        if (const BlueprintNode* node = graph_.GetNode(nodeId))
            selection.AddNode(*node);
    }
    for (const BlueprintLink& link : graph_.GetLinks())
    {
        if (IsSelected(link.fromNode) && IsSelected(link.toNode))
            selection.AddLink(link);
    }
    return selection.ToString();
}

bool BlueprintTab::DeserializeSelection(const ea::string& serialized, const Vector2& offset)
{
    BlueprintGraph selection;
    ea::string error;
    if (!selection.FromString(serialized, &error))
    {
        status_ = Format("Unable to paste Blueprint selection: {}", error);
        return false;
    }

    struct IdPair { BlueprintId oldId; BlueprintId newId; };
    ea::vector<IdPair> idMap;
    selectedNodes_.clear();
    for (const BlueprintNode& sourceNode : selection.GetNodes())
    {
        BlueprintNode node = sourceNode;
        const BlueprintId oldId = node.id;
        node.id = BLUEPRINT_INVALID_ID;
        node.position += offset;
        const BlueprintId newId = graph_.AddNode(node);
        idMap.push_back({oldId, newId});
        selectedNodes_.push_back(newId);
    }

    auto remap = [&idMap](BlueprintId oldId)
    {
        for (const IdPair& pair : idMap)
        {
            if (pair.oldId == oldId)
                return pair.newId;
        }
        return BLUEPRINT_INVALID_ID;
    };
    for (const BlueprintLink& sourceLink : selection.GetLinks())
    {
        BlueprintLink link = sourceLink;
        link.id = BLUEPRINT_INVALID_ID;
        link.fromNode = remap(sourceLink.fromNode);
        link.toNode = remap(sourceLink.toNode);
        if (link.fromNode != BLUEPRINT_INVALID_ID && link.toNode != BLUEPRINT_INVALID_ID)
            graph_.AddLink(link);
    }
    selectedNode_ = selectedNodes_.empty() ? BLUEPRINT_INVALID_ID : selectedNodes_.back();
    return !selectedNodes_.empty();
}

void BlueprintTab::DeleteSelected()
{
    if (selectedNodes_.empty())
        return;
    BeginGraphEdit();
    for (const BlueprintId nodeId : selectedNodes_)
        graph_.RemoveNode(nodeId);
    selectedNodes_.clear();
    selectedNode_ = BLUEPRINT_INVALID_ID;
    CommitGraphEdit("Selected Blueprint nodes deleted");
}

void BlueprintTab::CopySelection()
{
    clipboard_ = SerializeSelection();
    status_ = clipboard_.empty() ? "No Blueprint nodes selected" : "Blueprint selection copied";
}

void BlueprintTab::CutSelection()
{
    if (selectedNodes_.empty())
        return;
    CopySelection();
    DeleteSelected();
}

void BlueprintTab::PasteSelection()
{
    if (clipboard_.empty())
        return;
    BeginGraphEdit();
    if (DeserializeSelection(clipboard_, Vector2{48.0f, 48.0f}))
        CommitGraphEdit("Blueprint selection pasted");
    else
    {
        graphEditSnapshot_.clear();
        status_ = "Unable to paste Blueprint selection";
    }
}

void BlueprintTab::DuplicateSelection()
{
    CopySelection();
    PasteSelection();
}

void BlueprintTab::ToggleBreakpoint()
{
    if (selectedNode_ == BLUEPRINT_INVALID_ID)
        return;
    for (auto iter = breakpoints_.begin(); iter != breakpoints_.end(); ++iter)
    {
        if (*iter == selectedNode_)
        {
            breakpoints_.erase(iter);
            runtime_.SetBreakpoint(selectedNode_, false);
            status_ = "Breakpoint removed";
            return;
        }
    }
    breakpoints_.push_back(selectedNode_);
    runtime_.SetBreakpoint(selectedNode_, true);
    status_ = "Breakpoint added";
}

void BlueprintTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    (void)hotkeyManager;
}

void BlueprintTab::AddNodeFromToolbar(const ea::string& typeName)
{
    const Vector2 position{240.0f + static_cast<float>(graph_.GetNodes().size()) * 24.0f, 300.0f};
    const BlueprintId nodeId = graph_.AddNode(typeName, typeName, position,
        typeName.starts_with("Math.") || typeName == "Variable.Get"
            ? BlueprintExecutionMode::Pure : BlueprintExecutionMode::Immediate);
    BlueprintNode* node = graph_.GetNode(nodeId);
    if (!node)
        return;

    if (const BlueprintNodeDefinition* definition = runtime_.GetRegistry().Find(typeName))
    {
        node->executionMode = definition->executionMode;
        node->pins = definition->pins;
    }

    if (typeName == "Variable.Get" || typeName == "Variable.Set")
        node->properties["variableName"] = Variant(ea::string("Score"));
    else if (typeName == "Function.Call")
        node->properties["functionName"] = Variant(ea::string());

    selectedNode_ = nodeId;
    graphDirty_ = true;
    status_ = Format("Added {}", typeName);
}

void BlueprintTab::RenderContent()
{
    if (ui::BeginChild("##BlueprintMain", ui::GetContentRegionAvail(), false))
    {
        RenderNodePalette();
        RenderTypePanels();
        RenderGraphCanvas();
        RenderWatchWindow();
        ui::Separator();
        RenderDiagnostics();
    }
    ui::EndChild();
}

void BlueprintTab::RenderToolbar()
{
    if (ui::Button("New"))
        CreateDemoGraph();
    ui::SameLine();
    if (ui::Button("Load"))
        LoadGraph();
    ui::SameLine();
    if (ui::Button("Save"))
        SaveGraph();
    ui::SameLine();
    if (ui::Button("Validate"))
    {
        const BlueprintValidationResult result = graph_.Validate();
        status_ = result.IsValid() ? "Graph is valid" : Format("{} graph error(s)", result.diagnostics.size());
    }
    ui::SameLine();
    if (ui::Button("Run"))
    {
        const bool success = runtime_.ExecuteEvent(graph_, "Event.OnStart");
        status_ = success ? "Graph executed" : "Runtime error";
    }
    ui::SameLine();
    if (ui::Button("Add Event"))
        AddNodeFromToolbar("Event.OnStart");
    ui::SameLine();
    if (ui::Button("Add Print"))
        AddNodeFromToolbar("Flow.Print");
    ui::SameLine();
    if (ui::Button("Add Branch"))
        AddNodeFromToolbar("Flow.Branch");
    ui::SameLine();
    if (ui::Button("Add Math"))
        AddNodeFromToolbar("Math.AddFloat");
    ui::SameLine();
    if (ui::Button("Auto Layout"))
        PerformAutoLayout();
    ui::SameLine();
    if (ui::Button(showMinimap_ ? "Hide Minimap" : "Show Minimap"))
        showMinimap_ = !showMinimap_;
    ui::SameLine();
    if (ui::Button(showComments_ ? "Hide Comments" : "Show Comments"))
        showComments_ = !showComments_;
    ui::SameLine();
    RenderDebugToolbar();
    ui::SameLine();
    ui::Text("Zoom %.2f", zoom_);
}

void BlueprintTab::RenderGraphCanvas()
{
    const ImVec2 canvasSize = ui::GetContentRegionAvail();
    if (canvasSize.x < 10.0f || canvasSize.y < 10.0f)
        return;

    const ImVec2 canvasOrigin = ui::GetCursorScreenPos();
    ui::InvisibleButton("##BlueprintCanvas", canvasSize,
        ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle | ImGuiButtonFlags_MouseButtonRight);
    const bool hovered = ui::IsItemHovered();
    ImDrawList* drawList = ui::GetWindowDrawList();
    const ImVec2 canvasEnd = canvasOrigin + canvasSize;

    drawList->AddRectFilled(canvasOrigin, canvasEnd, IM_COL32(23, 26, 31, 255));
    const float grid = 32.0f * zoom_;
    const float startX = fmodf(pan_.x_, grid);
    const float startY = fmodf(pan_.y_, grid);
    for (float x = startX; x < canvasSize.x; x += grid)
        drawList->AddLine(canvasOrigin + ImVec2{x, 0}, canvasOrigin + ImVec2{x, canvasSize.y}, IM_COL32(38, 43, 51, 255));
    for (float y = startY; y < canvasSize.y; y += grid)
        drawList->AddLine(canvasOrigin + ImVec2{0, y}, canvasOrigin + ImVec2{canvasSize.x, y}, IM_COL32(38, 43, 51, 255));

    // Open popups from the stable canvas window scope on the following frame.
    // Opening directly while the InvisibleButton is processing the right click can
    // make ImGui close the popup immediately when the item loses ownership.
    if (contextMenuRequested_)
    {
        ui::OpenPopup("BlueprintCanvasContext");
        contextMenuRequested_ = false;
    }
    if (nodeContextMenuRequested_)
    {
        ui::OpenPopup("BlueprintNodeContext");
        nodeContextMenuRequested_ = false;
    }

    const ImGuiIO& io = ui::GetIO();
    if (hovered && (ui::IsMouseDown(MOUSEB_MIDDLE) || ui::IsMouseDown(MOUSEB_RIGHT)))
    {
        pan_ += Vector2(io.MouseDelta.x, io.MouseDelta.y);
    }
    if (hovered && io.MouseWheel != 0.0f)
    {
        const float oldZoom = zoom_;
        zoom_ = Clamp(zoom_ + io.MouseWheel * 0.1f, 0.35f, 2.5f);
        const ImVec2 mouseBefore = io.MousePos;
        const Vector2 graphBefore = ScreenToGraph(mouseBefore, canvasOrigin);
        pan_ += Vector2((mouseBefore.x - canvasOrigin.x) - (graphBefore.x_ * zoom_ + pan_.x_),
            (mouseBefore.y - canvasOrigin.y) - (graphBefore.y_ * zoom_ + pan_.y_));
        if (oldZoom == zoom_)
            pan_ += Vector2::ZERO;
    }

    if (hovered && ui::IsMouseClicked(MOUSEB_RIGHT))
    {
        const Vector2 graphPosition = ScreenToGraph(io.MousePos, canvasOrigin);
        contextGraphPosition_ = graphPosition;
        if (BlueprintNode* node = FindNodeAt(graphPosition))
        {
            contextNode_ = node->id;
            SelectNode(node->id, io.KeyCtrl);
            nodeContextMenuRequested_ = true;
        }
        else
        {
            contextNode_ = BLUEPRINT_INVALID_ID;
            contextMenuRequested_ = true;
        }
    }

    if (hovered && ui::IsMouseClicked(MOUSEB_LEFT))
    {
        const Vector2 graphPosition = ScreenToGraph(io.MousePos, canvasOrigin);
        BlueprintNode* pinNode = nullptr;
        if (BlueprintPin* pin = FindPinAt(graphPosition, pinNode))
        {
            linkingNode_ = pinNode->id;
            linkingPin_ = pin->name;
            linkingFromOutput_ = IsOutputPin(pin->kind);
            BeginGraphEdit();
        }
        else if (BlueprintNode* node = FindNodeAt(graphPosition))
        {
            SelectNode(node->id, io.KeyCtrl);
            draggingNode_ = node->id;
            BeginGraphEdit();
        }
        else if (!io.KeyCtrl)
        {
            selectedNodes_.clear();
            selectedNode_ = BLUEPRINT_INVALID_ID;
            draggingNode_ = BLUEPRINT_INVALID_ID;
        }
    }

    if (hovered && draggingNode_ != BLUEPRINT_INVALID_ID && ui::IsMouseDown(MOUSEB_LEFT))
    {
        for (const BlueprintId nodeId : selectedNodes_)
        {
            if (BlueprintNode* node = graph_.GetNode(nodeId))
                node->position += Vector2(io.MouseDelta.x / zoom_, io.MouseDelta.y / zoom_);
        }
        graphDirty_ = true;
    }

    if (linkingNode_ != BLUEPRINT_INVALID_ID && ui::IsMouseReleased(MOUSEB_LEFT))
    {
        const Vector2 graphPosition = ScreenToGraph(io.MousePos, canvasOrigin);
        BlueprintNode* targetNode = nullptr;
        BlueprintPin* targetPin = FindPinAt(graphPosition, targetNode);
        bool connected = false;
        if (targetPin && targetNode->id != linkingNode_ && IsInputPin(targetPin->kind) != !linkingFromOutput_)
        {
            if (linkingFromOutput_)
                connected = graph_.AddLink(linkingNode_, linkingPin_, targetNode->id, targetPin->name) != BLUEPRINT_INVALID_ID;
            else
                connected = graph_.AddLink(targetNode->id, targetPin->name, linkingNode_, linkingPin_) != BLUEPRINT_INVALID_ID;
        }
        linkingNode_ = BLUEPRINT_INVALID_ID;
        linkingPin_.clear();
        if (connected)
            CommitGraphEdit("Blueprint link created");
        else
            graphEditSnapshot_.clear();
    }

    if (draggingNode_ != BLUEPRINT_INVALID_ID && ui::IsMouseReleased(MOUSEB_LEFT))
    {
        draggingNode_ = BLUEPRINT_INVALID_ID;
        CommitGraphEdit("Blueprint nodes moved");
    }

    if (showComments_)
        RenderComments(canvasOrigin, drawList);
    RenderLinks(canvasOrigin, drawList);
    RenderLinkPreview(canvasOrigin, drawList);
    for (const BlueprintNode& node : graph_.GetNodes())
        RenderNode(node, canvasOrigin, drawList);
    RenderSelectionOverlay(canvasOrigin, drawList);
    if (showMinimap_)
        RenderMinimap(canvasOrigin, canvasSize);

    RenderCanvasContextMenu();
    RenderNodeContextMenu();
}

void BlueprintTab::RenderLinks(const ImVec2& canvasOrigin, ImDrawList* drawList)
{
    for (const BlueprintLink& link : graph_.GetLinks())
    {
        const BlueprintNode* from = FindNode(link.fromNode);
        const BlueprintNode* to = FindNode(link.toNode);
        if (!from || !to)
            continue;

        unsigned fromPinIndex = 0;
        unsigned toPinIndex = 0;
        bool foundFrom = false;
        bool foundTo = false;
        for (unsigned i = 0; i < from->pins.size(); ++i)
        {
            if (from->pins[i].name == link.fromPin)
            {
                fromPinIndex = i;
                foundFrom = true;
                break;
            }
        }
        for (unsigned i = 0; i < to->pins.size(); ++i)
        {
            if (to->pins[i].name == link.toPin)
            {
                toPinIndex = i;
                foundTo = true;
                break;
            }
        }
        if (!foundFrom || !foundTo)
            continue;

        const ImVec2 fromPosition = GraphToScreen(from->position, canvasOrigin) + ImVec2{NodeWidth, GetPinY(*from, fromPinIndex)};
        const ImVec2 toPosition = GraphToScreen(to->position, canvasOrigin) + ImVec2{0, GetPinY(*to, toPinIndex)};
        const float distance = Max(40.0f, Abs(toPosition.x - fromPosition.x) * 0.5f);
        const ImU32 color = PinColor(from->pins[fromPinIndex].dataType, IsExecutionPin(from->pins[fromPinIndex].kind));
        drawList->AddBezierCubic(fromPosition, fromPosition + ImVec2{distance, 0},
            toPosition - ImVec2{distance, 0}, toPosition, color, 2.5f);
    }
}

void BlueprintTab::RenderLinkPreview(const ImVec2& canvasOrigin, ImDrawList* drawList)
{
    if (linkingNode_ == BLUEPRINT_INVALID_ID)
        return;
    const BlueprintNode* node = FindNode(linkingNode_);
    if (!node)
        return;

    unsigned pinIndex = 0;
    for (; pinIndex < node->pins.size(); ++pinIndex)
    {
        if (node->pins[pinIndex].name == linkingPin_)
            break;
    }
    if (pinIndex >= node->pins.size())
        return;

    const ImGuiIO& io = ui::GetIO();
    const ImVec2 start = GraphToScreen(node->position, canvasOrigin)
        + ImVec2{linkingFromOutput_ ? NodeWidth * zoom_ : 0.0f, GetPinY(*node, pinIndex) * zoom_};
    const ImVec2 end = io.MousePos;
    const float distance = Max(40.0f, Abs(end.x - start.x) * 0.5f);
    const ImVec2 controlA = start + ImVec2{linkingFromOutput_ ? distance : -distance, 0.0f};
    const ImVec2 controlB = end + ImVec2{linkingFromOutput_ ? -distance : distance, 0.0f};
    drawList->AddBezierCubic(start, controlA, controlB, end, IM_COL32(245, 205, 80, 220), 2.0f);
}

void BlueprintTab::RenderSelectionOverlay(const ImVec2& canvasOrigin, ImDrawList* drawList)
{
    const ImGuiIO& io = ui::GetIO();
    if (linkingNode_ == BLUEPRINT_INVALID_ID || !ui::IsMouseDown(MOUSEB_LEFT))
        return;
    drawList->AddCircle(io.MousePos, 7.0f, IM_COL32(245, 205, 80, 255), 16, 1.5f);
}

void BlueprintTab::RenderCanvasContextMenu()
{
    if (!ui::BeginPopup("BlueprintCanvasContext"))
        return;

    ui::Text("Create Blueprint node");
    ui::Separator();
    ui::InputText("Search", &nodeSearch_);
    for (const BlueprintNodeDefinition& definition : runtime_.GetRegistry().GetDefinitions())
    {
        if (!nodeSearch_.empty() && definition.typeName.find(nodeSearch_) == ea::string::npos
            && definition.category.find(nodeSearch_) == ea::string::npos
            && definition.description.find(nodeSearch_) == ea::string::npos)
            continue;
        if (ui::MenuItem(definition.typeName.c_str()))
        {
            BeginGraphEdit();
            const BlueprintId oldNode = selectedNode_;
            AddNodeFromToolbar(definition.typeName);
            if (BlueprintNode* node = graph_.GetNode(selectedNode_))
                node->position = contextGraphPosition_;
            CommitGraphEdit("Blueprint node created from context menu");
            (void)oldNode;
            nodeSearch_.clear();
            ui::CloseCurrentPopup();
            break;
        }
    }
    ui::EndPopup();
}

void BlueprintTab::RenderNodeContextMenu()
{
    if (!ui::BeginPopup("BlueprintNodeContext"))
        return;

    if (ui::MenuItem("Copy", GetHotkeyLabel(Hotkey_Copy).c_str(), false, !selectedNodes_.empty()))
        CopySelection();
    if (ui::MenuItem("Cut", GetHotkeyLabel(Hotkey_Cut).c_str(), false, !selectedNodes_.empty()))
        CutSelection();
    if (ui::MenuItem("Duplicate", GetHotkeyLabel(Hotkey_Duplicate).c_str(), false, !selectedNodes_.empty()))
        DuplicateSelection();
    if (ui::MenuItem("Delete", GetHotkeyLabel(Hotkey_Delete).c_str(), false, !selectedNodes_.empty()))
    {
        DeleteSelected();
        ui::CloseCurrentPopup();
    }
    if (ui::MenuItem("Toggle breakpoint", GetHotkeyLabel(Hotkey_Breakpoint).c_str(), false, selectedNode_ != BLUEPRINT_INVALID_ID))
        ToggleBreakpoint();
    ui::Separator();
    if (ui::MenuItem("Create node from here"))
    {
        contextGraphPosition_ = FindNode(contextNode_) ? FindNode(contextNode_)->position + Vector2{NodeWidth + 40.0f, 0.0f} : contextGraphPosition_;
        contextMenuRequested_ = true;
        ui::CloseCurrentPopup();
    }
    ui::EndPopup();
}

void BlueprintTab::RenderNode(const BlueprintNode& node, const ImVec2& canvasOrigin, ImDrawList* drawList)
{
    const ImVec2 topLeft = GraphToScreen(node.position, canvasOrigin);
    const ImVec2 bottomRight = topLeft + ImVec2{NodeWidth * zoom_, GetNodeHeight(node) * zoom_};
    const bool selected = IsSelected(node.id) || node.id == selectedNode_;
    const bool debugCurrent = node.id == debugCurrentNode_ && runtime_.IsDebugActive();
    const ImU32 bodyColor = debugCurrent ? IM_COL32(115, 78, 35, 255)
        : selected ? IM_COL32(55, 75, 105, 255) : IM_COL32(43, 48, 57, 255);
    const ImU32 headerColor = selected ? IM_COL32(70, 110, 165, 255) : IM_COL32(55, 61, 72, 255);
    drawList->AddRectFilled(topLeft, bottomRight, bodyColor, 6.0f);
    drawList->AddRectFilled(topLeft, topLeft + ImVec2{NodeWidth * zoom_, HeaderHeight * zoom_}, headerColor, 6.0f,
        ImDrawFlags_RoundCornersTop);
    drawList->AddRect(topLeft, bottomRight, IM_COL32(110, 120, 135, 255), 6.0f, 0, 1.0f);

    drawList->AddText(topLeft + ImVec2{10.0f, 8.0f} * zoom_, IM_COL32(245, 245, 250, 255), node.title.c_str());
    if (IsSelected(node.id))
        drawList->AddCircleFilled(topLeft + ImVec2{NodeWidth * zoom_ - 12.0f, 16.0f * zoom_}, 4.0f * zoom_, IM_COL32(100, 210, 255, 255));
    for (const BlueprintId breakpoint : breakpoints_)
    {
        if (breakpoint == node.id)
            drawList->AddCircleFilled(topLeft + ImVec2{NodeWidth * zoom_ - 26.0f, 16.0f * zoom_}, 4.0f * zoom_, IM_COL32(235, 70, 65, 255));
    }
    for (unsigned i = 0; i < node.pins.size(); ++i)
    {
        const BlueprintPin& pin = node.pins[i];
        const bool output = IsOutputPin(pin.kind);
        const float x = output ? NodeWidth * zoom_ : 0.0f;
        const ImVec2 position = topLeft + ImVec2{x, GetPinY(node, i) * zoom_};
        const ImU32 color = PinColor(pin.dataType, IsExecutionPin(pin.kind));
        drawList->AddCircleFilled(position, PinRadius * zoom_, color);
        const ImVec2 textPosition = position + ImVec2{output ? -8.0f : 8.0f, -7.0f} * zoom_;
        const ImVec2 textSize = ui::CalcTextSize(pin.displayName.c_str()) * zoom_;
        drawList->AddText(output ? textPosition - ImVec2{textSize.x, 0} : textPosition,
            IM_COL32(225, 228, 235, 255), pin.displayName.c_str());
    }
}

void BlueprintTab::RenderDiagnostics()
{
    if (!status_.empty())
        ui::Text("%s", status_.c_str());

    const BlueprintValidationResult validation = graph_.Validate();
    if (validation.IsValid())
    {
        ui::TextColored({0.45f, 0.9f, 0.55f, 1.0f}, "Graph valid: %u node(s), %u link(s)",
            graph_.GetNodes().size(), graph_.GetLinks().size());
    }
    else
    {
        ui::TextColored({1.0f, 0.45f, 0.35f, 1.0f}, "Graph diagnostics: %u", validation.diagnostics.size());
        for (const BlueprintDiagnostic& diagnostic : validation.diagnostics)
            ui::BulletText("[%s] %s", diagnostic.code.c_str(), diagnostic.message.c_str());
    }
    for (const BlueprintDiagnostic& diagnostic : runtime_.GetDiagnostics())
    {
        ui::TextColored({1.0f, 0.65f, 0.25f, 1.0f}, "Runtime [%s] %s", diagnostic.code.c_str(), diagnostic.message.c_str());
    }
}

void BlueprintTab::RenderTypePanels()
{
    if (!showTypePanels_)
        return;

    if (!ui::CollapsingHeader("Blueprint Types", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ui::Text("Structs");
    ui::InputText("New struct", &newStructName_);
    ui::SameLine();
    if (ui::Button("Add struct"))
    {
        BlueprintStructDef structure;
        structure.name = newStructName_.empty() ? Format("Struct{}", graph_.GetStructs().size() + 1) : newStructName_;
        structure.description = "User-defined Blueprint struct";
        BlueprintStructField field;
        field.name = "Value";
        field.dataType = BlueprintDataType::Variant;
        structure.fields.push_back(field);
        BeginGraphEdit();
        if (graph_.AddStruct(structure))
            CommitGraphEdit(Format("Added struct {}", structure.name));
        else
            graphEditSnapshot_.clear();
        newStructName_.clear();
    }

    for (const BlueprintStructDef& structure : graph_.GetStructs())
    {
        ui::PushID(structure.name.c_str());
        if (ui::TreeNode(structure.name.c_str()))
        {
            ui::TextWrapped("%s", structure.description.c_str());
            for (const BlueprintStructField& field : structure.fields)
                ui::BulletText("%s : %s", field.name.c_str(), ToString(field.dataType).c_str());
            if (ui::SmallButton("Add field"))
            {
                BeginGraphEdit();
                if (BlueprintStructDef* editable = graph_.GetStruct(structure.name))
                {
                    BlueprintStructField field;
                    field.name = Format("Field{}", editable->fields.size() + 1);
                    field.dataType = BlueprintDataType::Variant;
                    editable->fields.push_back(field);
                    CommitGraphEdit(Format("Updated struct {}", structure.name));
                }
                else
                    graphEditSnapshot_.clear();
            }
            ui::SameLine();
            if (ui::SmallButton("Remove struct"))
            {
                BeginGraphEdit();
                if (graph_.RemoveStruct(structure.name))
                    CommitGraphEdit(Format("Removed struct {}", structure.name));
                else
                    graphEditSnapshot_.clear();
                ui::TreePop();
                ui::PopID();
                break;
            }
            ui::TreePop();
        }
        ui::PopID();
    }

    ui::Separator();
    ui::Text("Enums");
    ui::InputText("New enum", &newEnumName_);
    ui::SameLine();
    if (ui::Button("Add enum"))
    {
        BlueprintEnumDef enumeration;
        enumeration.name = newEnumName_.empty() ? Format("Enum{}", graph_.GetEnums().size() + 1) : newEnumName_;
        enumeration.description = "User-defined Blueprint enum";
        enumeration.values.push_back({"None", 0});
        BeginGraphEdit();
        if (graph_.AddEnum(enumeration))
            CommitGraphEdit(Format("Added enum {}", enumeration.name));
        else
            graphEditSnapshot_.clear();
        newEnumName_.clear();
    }

    for (const BlueprintEnumDef& enumeration : graph_.GetEnums())
    {
        ui::PushID(enumeration.name.c_str());
        if (ui::TreeNode(enumeration.name.c_str()))
        {
            ui::TextWrapped("%s", enumeration.description.c_str());
            for (const BlueprintEnumValue& value : enumeration.values)
                ui::BulletText("%s = %d", value.name.c_str(), value.value);
            if (ui::SmallButton("Add enum value"))
            {
                BeginGraphEdit();
                if (BlueprintEnumDef* editable = graph_.GetEnum(enumeration.name))
                {
                    BlueprintEnumValue value;
                    value.name = Format("Value{}", editable->values.size());
                    value.value = static_cast<int>(editable->values.size());
                    editable->values.push_back(value);
                    CommitGraphEdit(Format("Updated enum {}", enumeration.name));
                }
                else
                    graphEditSnapshot_.clear();
            }
            ui::SameLine();
            if (ui::SmallButton("Remove enum"))
            {
                BeginGraphEdit();
                if (graph_.RemoveEnum(enumeration.name))
                    CommitGraphEdit(Format("Removed enum {}", enumeration.name));
                else
                    graphEditSnapshot_.clear();
                ui::TreePop();
                ui::PopID();
                break;
            }
            ui::TreePop();
        }
        ui::PopID();
    }

    ui::Separator();
    ui::Text("Delegates and signals");
    ui::InputText("New delegate", &newDelegateName_);
    ui::SameLine();
    if (ui::Button("Add delegate"))
    {
        BlueprintDelegate delegate;
        delegate.name = newDelegateName_.empty() ? Format("Signal{}", graph_.GetDelegates().size() + 1) : newDelegateName_;
        delegate.description = "Blueprint delegate or signal";
        BlueprintPin parameter;
        parameter.name = "Value";
        parameter.displayName = parameter.name;
        parameter.kind = BlueprintPinKind::Input;
        parameter.dataType = BlueprintDataType::Variant;
        delegate.parameters.push_back(parameter);
        BeginGraphEdit();
        if (graph_.AddDelegate(delegate))
            CommitGraphEdit(Format("Added delegate {}", delegate.name));
        else
            graphEditSnapshot_.clear();
        newDelegateName_.clear();
    }

    for (const BlueprintDelegate& delegate : graph_.GetDelegates())
    {
        ui::PushID(delegate.name.c_str());
        ui::BulletText("%s (%u parameter(s))", delegate.name.c_str(), delegate.parameters.size());
        ui::SameLine();
        if (ui::SmallButton("Add parameter"))
        {
            BeginGraphEdit();
            if (BlueprintDelegate* editable = graph_.GetDelegate(delegate.name))
            {
                BlueprintPin parameter;
                parameter.name = Format("Param{}", editable->parameters.size() + 1);
                parameter.displayName = parameter.name;
                parameter.kind = BlueprintPinKind::Input;
                parameter.dataType = BlueprintDataType::Variant;
                editable->parameters.push_back(parameter);
                CommitGraphEdit(Format("Updated delegate {}", delegate.name));
            }
            else
                graphEditSnapshot_.clear();
        }
        ui::SameLine();
        if (ui::SmallButton("Remove delegate"))
        {
            BeginGraphEdit();
            if (graph_.RemoveDelegate(delegate.name))
                CommitGraphEdit(Format("Removed delegate {}", delegate.name));
            else
                graphEditSnapshot_.clear();
            ui::PopID();
            break;
        }
        ui::PopID();
    }

    ui::Separator();
    ui::Text("Timelines");
    ui::InputText("New timeline", &newTimelineName_);
    ui::SameLine();
    if (ui::Button("Add timeline"))
    {
        BlueprintTimeline timeline;
        timeline.name = newTimelineName_.empty() ? Format("Timeline{}", graph_.GetTimelines().size() + 1) : newTimelineName_;
        timeline.description = "Blueprint timeline";
        timeline.keyframes.push_back({0.0f, Variant(0.0f)});
        timeline.keyframes.push_back({1.0f, Variant(1.0f)});
        BeginGraphEdit();
        if (graph_.AddTimeline(timeline))
            CommitGraphEdit(Format("Added timeline {}", timeline.name));
        else
            graphEditSnapshot_.clear();
        newTimelineName_.clear();
    }

    for (const BlueprintTimeline& timeline : graph_.GetTimelines())
    {
        ui::PushID(timeline.name.c_str());
        if (ui::TreeNode(timeline.name.c_str()))
        {
            BlueprintTimeline* editable = graph_.GetTimeline(timeline.name);
            if (editable)
            {
                BeginGraphEdit();
                bool changed = ui::DragFloat("Length", &editable->length, 0.01f, 0.01f, 3600.0f);
                changed = ui::Checkbox("Looping", &editable->looping) || changed;
                for (unsigned i = 0; i < editable->keyframes.size(); ++i)
                {
                    ui::PushID(static_cast<int>(i));
                    changed = ui::DragFloat("Time", &editable->keyframes[i].time, 0.01f, 0.0f, editable->length) || changed;
                    ui::SameLine();
                    ui::Text("Value: %s", editable->keyframes[i].value.ToString().c_str());
                    ui::PopID();
                }
                if (ui::SmallButton("Add keyframe"))
                {
                    editable->keyframes.push_back({editable->length, Variant(0.0f)});
                    changed = true;
                }
                ui::SameLine();
                if (ui::SmallButton("Remove timeline"))
                {
                    graph_.RemoveTimeline(timeline.name);
                    changed = true;
                    CommitGraphEdit(Format("Removed timeline {}", timeline.name));
                    ui::TreePop();
                    ui::PopID();
                    break;
                }
                if (changed)
                    CommitGraphEdit(Format("Updated timeline {}", timeline.name));
                else
                    graphEditSnapshot_.clear();
            }
            ui::TreePop();
        }
        ui::PopID();
    }
}

void BlueprintTab::RenderNodePalette()
{
    ui::Text("Blueprint Node Palette");
    ui::SameLine();
    ui::InputText("Search", &nodeSearch_);
    if (nodeSearch_.empty())
        return;

    const ea::string query = nodeSearch_;
    for (const BlueprintNodeDefinition& definition : runtime_.GetRegistry().GetDefinitions())
    {
        if (definition.typeName.find(query) == ea::string::npos
            && definition.category.find(query) == ea::string::npos
            && definition.description.find(query) == ea::string::npos)
            continue;
        if (ui::Button(definition.typeName.c_str()))
        {
            AddNodeFromToolbar(definition.typeName);
            nodeSearch_.clear();
            break;
        }
        ui::SameLine();
    }
    ui::NewLine();
}

void BlueprintTab::RenderComments(const ImVec2& canvasOrigin, ImDrawList* drawList)
{
    for (const BlueprintComment& comment : graph_.GetComments())
    {
        const ImVec2 topLeft = GraphToScreen(comment.position, canvasOrigin);
        const ImVec2 size{comment.size.x_ * zoom_, comment.size.y_ * zoom_};
        const ImVec2 bottomRight = topLeft + size;
        drawList->AddRectFilled(topLeft, bottomRight, comment.color, 6.0f);
        drawList->AddRect(topLeft, bottomRight, IM_COL32(190, 190, 205, 180), 6.0f, 0, 1.0f);
        drawList->AddText(topLeft + ImVec2{8.0f, 6.0f} * zoom_, IM_COL32(245, 245, 250, 255), comment.text.c_str());
    }
}

void BlueprintTab::RenderMinimap(const ImVec2& canvasOrigin, const ImVec2& canvasSize)
{
    const ImVec2 minimapSize{210.0f, 140.0f};
    const ImVec2 topLeft = canvasOrigin + canvasSize - minimapSize - ImVec2{14.0f, 14.0f};
    const ImVec2 bottomRight = topLeft + minimapSize;
    ImDrawList* drawList = ui::GetWindowDrawList();
    drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(18, 20, 24, 230), 4.0f);
    drawList->AddRect(topLeft, bottomRight, IM_COL32(105, 115, 130, 220), 4.0f);

    if (graph_.GetNodes().empty())
        return;

    Vector2 minimum = graph_.GetNodes().front().position;
    Vector2 maximum = minimum;
    for (const BlueprintNode& node : graph_.GetNodes())
    {
        minimum.x_ = Min(minimum.x_, node.position.x_);
        minimum.y_ = Min(minimum.y_, node.position.y_);
        maximum.x_ = Max(maximum.x_, node.position.x_ + NodeWidth);
        maximum.y_ = Max(maximum.y_, node.position.y_ + GetNodeHeight(node));
    }
    const Vector2 extent = maximum - minimum;
    const float scale = Min((minimapSize.x - 16.0f) / Max(extent.x_, 1.0f), (minimapSize.y - 16.0f) / Max(extent.y_, 1.0f));
    for (const BlueprintNode& node : graph_.GetNodes())
    {
        const ImVec2 nodeTopLeft = topLeft + ImVec2{8.0f + (node.position.x_ - minimum.x_) * scale,
            8.0f + (node.position.y_ - minimum.y_) * scale};
        const ImVec2 nodeSize{Max(5.0f, NodeWidth * scale), Max(4.0f, GetNodeHeight(node) * scale)};
        const ImU32 color = node.id == debugCurrentNode_ ? IM_COL32(245, 175, 60, 255)
            : node.id == selectedNode_ ? IM_COL32(90, 155, 235, 255) : IM_COL32(100, 105, 120, 255);
        drawList->AddRectFilled(nodeTopLeft, nodeTopLeft + nodeSize, color, 2.0f);
    }
}

void BlueprintTab::RenderDebugToolbar()
{
    if (ui::Button("Debug Start"))
    {
        debugPaused_ = runtime_.BeginDebug(graph_, "Event.OnStart");
        debugCurrentNode_ = runtime_.GetDebugCurrentNode();
        status_ = debugPaused_ ? "Debugger paused at entry" : "Unable to start debugger";
    }
    ui::SameLine();
    if (ui::Button("Step") && runtime_.IsDebugActive())
    {
        debugPaused_ = runtime_.StepDebug();
        debugCurrentNode_ = runtime_.GetDebugCurrentNode();
        status_ = debugPaused_ ? "Debugger advanced one node" : "Debugger finished or stopped";
    }
    ui::SameLine();
    if (ui::Button("Continue") && runtime_.IsDebugActive())
    {
        debugPaused_ = runtime_.ContinueDebug();
        debugCurrentNode_ = runtime_.GetDebugCurrentNode();
        status_ = debugPaused_ ? "Debugger paused at breakpoint" : "Debugger finished or stopped";
    }
    ui::SameLine();
    if (ui::Button("Stop Debug"))
    {
        runtime_.StopDebug();
        debugCurrentNode_ = BLUEPRINT_INVALID_ID;
        debugPaused_ = false;
        status_ = "Debugger stopped";
    }
    ui::SameLine();
    ui::Checkbox("Watch", &showWatchWindow_);
}

void BlueprintTab::RenderWatchWindow()
{
    if (!showWatchWindow_)
        return;

    if (!ui::BeginChild("##BlueprintWatch", ImVec2{0.0f, 132.0f}, true))
    {
        ui::EndChild();
        return;
    }

    ui::Text("Blueprint Watch");
    ui::SameLine();
    ui::TextColored(runtime_.IsDebugActive() ? ImVec4{0.45f, 0.9f, 0.55f, 1.0f} : ImVec4{0.65f, 0.65f, 0.7f, 1.0f},
        runtime_.IsDebugActive() ? "paused" : "idle");

    const auto& variables = runtime_.GetWatchVariables();
    for (const auto& item : variables)
        ui::BulletText("%s = %s", item.first.c_str(), item.second.ToString().c_str());

    const auto& values = runtime_.GetWatchValues();
    for (const auto& item : values)
        ui::BulletText("%s = %s", item.first.c_str(), item.second.ToString().c_str());

    const auto& callStack = runtime_.GetCallStack();
    if (!callStack.empty())
    {
        ui::Separator();
        ui::Text("Call stack");
        for (auto iter = callStack.rbegin(); iter != callStack.rend(); ++iter)
            ui::BulletText("%s", iter->c_str());
    }
    ui::EndChild();
}

void BlueprintTab::PerformAutoLayout()
{
    graph_.AutoLayout();
    graphDirty_ = true;
    status_ = "Automatic node layout applied";
}

void BlueprintTab::RenderContextMenuItems()
{
    contextMenuSeparator_.Reset();
    if (ui::MenuItem("Validate Blueprint"))
    {
        const BlueprintValidationResult result = graph_.Validate();
        status_ = result.IsValid() ? "Graph is valid" : "Graph has validation errors";
    }
    if (ui::MenuItem("Reset demo graph"))
        CreateDemoGraph();
}

void BlueprintTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    BaseClassName::WriteIniSettings(output);
    WriteStringToIni(output, "Zoom", Format("{}", zoom_));
    WriteStringToIni(output, "Pan", Format("{},{}", pan_.x_, pan_.y_));
}

void BlueprintTab::ReadIniSettings(const char* line)
{
    BaseClassName::ReadIniSettings(line);
    if (const auto value = ReadStringFromIni(line, "Zoom"))
        zoom_ = Clamp(ToFloat(*value), 0.35f, 2.5f);
    if (const auto value = ReadStringFromIni(line, "Pan"))
        pan_ = ToVector2(*value);
}

void BlueprintTab::SaveGraph()
{
    auto fs = GetSubsystem<FileSystem>();
    const ea::string directory = AddTrailingSlash(GetProject()->GetProjectPath()) + "Blueprints";
    fs->CreateDir(directory);

    BlueprintResource resource(context_);
    resource.SetGraph(graph_);
    if (resource.SaveFile(GetGraphFileName()))
    {
        graphDirty_ = false;
        status_ = "Blueprint resource saved";
    }
    else
        status_ = "Unable to save Blueprint resource";
}

void BlueprintTab::LoadGraph()
{
    auto cache = GetSubsystem<ResourceCache>();
    BlueprintResource* resource = cache ? cache->GetResource<BlueprintResource>(GetGraphFileName()) : nullptr;
    if (!resource)
    {
        status_ = "No saved Blueprint resource found";
        return;
    }

    graph_ = resource->GetGraph();
    selectedNodes_.clear();
    selectedNode_ = BLUEPRINT_INVALID_ID;
    graphDirty_ = false;
    status_ = "Blueprint resource loaded";
}

ea::string BlueprintTab::GetGraphFileName() const
{
    return GetProject() ? AddTrailingSlash(GetProject()->GetProjectPath()) + "Blueprints/Main.blueprint" : "Main.blueprint";
}

BlueprintNode* BlueprintTab::FindNodeAt(const Vector2& graphPosition)
{
    for (auto iter = graph_.GetNodes().rbegin(); iter != graph_.GetNodes().rend(); ++iter)
    {
        const BlueprintNode& node = *iter;
        const float height = GetNodeHeight(node);
        if (graphPosition.x_ >= node.position.x_ && graphPosition.x_ <= node.position.x_ + NodeWidth
            && graphPosition.y_ >= node.position.y_ && graphPosition.y_ <= node.position.y_ + height)
            return graph_.GetNode(node.id);
    }
    return nullptr;
}

const BlueprintNode* BlueprintTab::FindNode(BlueprintId id) const
{
    return graph_.GetNode(id);
}

ImVec2 BlueprintTab::GraphToScreen(const Vector2& graphPosition, const ImVec2& canvasOrigin) const
{
    return canvasOrigin + ImVec2{pan_.x_ + graphPosition.x_ * zoom_, pan_.y_ + graphPosition.y_ * zoom_};
}

Vector2 BlueprintTab::ScreenToGraph(const ImVec2& screenPosition, const ImVec2& canvasOrigin) const
{
    return {(screenPosition.x - canvasOrigin.x - pan_.x_) / zoom_,
        (screenPosition.y - canvasOrigin.y - pan_.y_) / zoom_};
}

float BlueprintTab::GetNodeHeight(const BlueprintNode& node) const
{
    return HeaderHeight + Max(1.0f, static_cast<float>(node.pins.size())) * PinHeight + 10.0f;
}

float BlueprintTab::GetPinY(const BlueprintNode& node, unsigned pinIndex) const
{
    return HeaderHeight + PinHeight * (static_cast<float>(pinIndex) + 0.5f);
}

}
