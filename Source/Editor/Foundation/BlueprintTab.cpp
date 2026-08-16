// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "BlueprintTab.h"

#include "../Core/IniHelpers.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONFile.h>

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

    if (typeName == "Flow.Print")
    {
        AddPin(*node, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
        AddPin(*node, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
        AddPin(*node, "message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("Message")));
    }
    else if (typeName == "Flow.Branch")
    {
        AddPin(*node, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
        AddPin(*node, "true", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
        AddPin(*node, "false", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
        AddPin(*node, "condition", BlueprintPinKind::Input, BlueprintDataType::Bool, Variant(false));
    }
    else if (typeName == "Math.AddFloat" || typeName == "Math.MultiplyFloat" || typeName == "Math.LessFloat")
    {
        AddPin(*node, "a", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.0f));
        AddPin(*node, "b", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.0f));
        AddPin(*node, "result", BlueprintPinKind::Output,
            typeName == "Math.LessFloat" ? BlueprintDataType::Bool : BlueprintDataType::Float);
    }
    else if (typeName == "Variable.Get")
    {
        node->properties["variableName"] = Variant(ea::string("Score"));
        AddPin(*node, "value", BlueprintPinKind::Output, BlueprintDataType::Variant);
    }
    else if (typeName == "Variable.Set")
    {
        node->properties["variableName"] = Variant(ea::string("Score"));
        AddPin(*node, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
        AddPin(*node, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
        AddPin(*node, "value", BlueprintPinKind::Input, BlueprintDataType::Variant);
    }

    selectedNode_ = nodeId;
    graphDirty_ = true;
    status_ = Format("Added {}", typeName);
}

void BlueprintTab::RenderContent()
{
    if (ui::BeginChild("##BlueprintMain", ui::GetContentRegionAvail(), false))
    {
        RenderNodePalette();
        RenderGraphCanvas();
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
    if (ui::Button("Stop Debug"))
    {
        runtime_.StopDebug();
        debugCurrentNode_ = BLUEPRINT_INVALID_ID;
        debugPaused_ = false;
        status_ = "Debugger stopped";
    }
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

    if (hovered && ui::IsMouseClicked(MOUSEB_LEFT))
    {
        const Vector2 graphPosition = ScreenToGraph(io.MousePos, canvasOrigin);
        if (BlueprintNode* node = FindNodeAt(graphPosition))
        {
            selectedNode_ = node->id;
            draggingNode_ = node->id;
        }
        else
        {
            selectedNode_ = BLUEPRINT_INVALID_ID;
            draggingNode_ = BLUEPRINT_INVALID_ID;
        }
    }
    if (!ui::IsMouseDown(MOUSEB_LEFT))
        draggingNode_ = BLUEPRINT_INVALID_ID;
    if (hovered && draggingNode_ != BLUEPRINT_INVALID_ID && ui::IsMouseDown(MOUSEB_LEFT))
    {
        if (BlueprintNode* node = graph_.GetNode(draggingNode_))
        {
            node->position += Vector2(io.MouseDelta.x / zoom_, io.MouseDelta.y / zoom_);
            graphDirty_ = true;
        }
    }

    if (showComments_)
        RenderComments(canvasOrigin, drawList);
    RenderLinks(canvasOrigin, drawList);
    for (const BlueprintNode& node : graph_.GetNodes())
        RenderNode(node, canvasOrigin, drawList);
    if (showMinimap_)
        RenderMinimap(canvasOrigin, canvasSize);
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

void BlueprintTab::RenderNode(const BlueprintNode& node, const ImVec2& canvasOrigin, ImDrawList* drawList)
{
    const ImVec2 topLeft = GraphToScreen(node.position, canvasOrigin);
    const ImVec2 bottomRight = topLeft + ImVec2{NodeWidth * zoom_, GetNodeHeight(node) * zoom_};
    const bool selected = node.id == selectedNode_;
    const bool debugCurrent = node.id == debugCurrentNode_ && runtime_.IsDebugActive();
    const ImU32 bodyColor = debugCurrent ? IM_COL32(115, 78, 35, 255)
        : selected ? IM_COL32(55, 75, 105, 255) : IM_COL32(43, 48, 57, 255);
    const ImU32 headerColor = selected ? IM_COL32(70, 110, 165, 255) : IM_COL32(55, 61, 72, 255);
    drawList->AddRectFilled(topLeft, bottomRight, bodyColor, 6.0f);
    drawList->AddRectFilled(topLeft, topLeft + ImVec2{NodeWidth * zoom_, HeaderHeight * zoom_}, headerColor, 6.0f,
        ImDrawFlags_RoundCornersTop);
    drawList->AddRect(topLeft, bottomRight, IM_COL32(110, 120, 135, 255), 6.0f, 0, 1.0f);

    drawList->AddText(topLeft + ImVec2{10.0f, 8.0f} * zoom_, IM_COL32(245, 245, 250, 255), node.title.c_str());
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
    ui::Text(runtime_.IsDebugActive() ? "Debug session active" : "Debug session inactive");
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

    JSONFile file(context_);
    file.GetRoot() = graph_.ToJSON();
    if (file.SaveFile(GetGraphFileName()))
    {
        graphDirty_ = false;
        status_ = "Blueprint saved";
    }
    else
        status_ = "Unable to save Blueprint";
}

void BlueprintTab::LoadGraph()
{
    JSONFile file(context_);
    if (!file.LoadFile(GetGraphFileName()))
    {
        status_ = "No saved Blueprint found";
        return;
    }

    ea::string error;
    if (graph_.FromJSON(file.GetRoot(), &error))
    {
        graphDirty_ = false;
        status_ = "Blueprint loaded";
    }
    else
        status_ = Format("Unable to load Blueprint: {}", error);
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
