// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "../Project/EditorTab.h"
#include "../Project/Project.h"

#include <Urho3D/Blueprint/BlueprintRuntime.h>

namespace Urho3D
{

/// Register the Blueprint editor tab in a project.
void Foundation_BlueprintTab(Context* context, Project* project);

/// Blueprint graph editor with a native ImGui canvas and executable graph preview.
class BlueprintTab : public EditorTab
{
    URHO3D_OBJECT(BlueprintTab, EditorTab);

public:
    explicit BlueprintTab(Context* context);

    /// Implement EditorTab.
    /// @{
    void RenderContent() override;
    void RenderToolbar() override;
    void RenderContextMenuItems() override;
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

private:
    void CreateDemoGraph();
    void AddNodeFromToolbar(const ea::string& typeName);
    void RenderGraphCanvas();
    void RenderNode(const BlueprintNode& node, const ImVec2& canvasOrigin, ImDrawList* drawList);
    void RenderLinks(const ImVec2& canvasOrigin, ImDrawList* drawList);
    void RenderDiagnostics();
    void SaveGraph();
    void LoadGraph();
    ea::string GetGraphFileName() const;
    BlueprintNode* FindNodeAt(const Vector2& graphPosition);
    const BlueprintNode* FindNode(BlueprintId id) const;
    ImVec2 GraphToScreen(const Vector2& graphPosition, const ImVec2& canvasOrigin) const;
    Vector2 ScreenToGraph(const ImVec2& screenPosition, const ImVec2& canvasOrigin) const;
    float GetNodeHeight(const BlueprintNode& node) const;
    float GetPinY(const BlueprintNode& node, unsigned pinIndex) const;
    void AddPin(BlueprintNode& node, const ea::string& name, BlueprintPinKind kind,
        BlueprintDataType dataType, const Variant& defaultValue = Variant());

    BlueprintGraph graph_;
    BlueprintRuntime runtime_;
    BlueprintId selectedNode_{BLUEPRINT_INVALID_ID};
    BlueprintId draggingNode_{BLUEPRINT_INVALID_ID};
    Vector2 pan_{Vector2::ZERO};
    float zoom_{1.0f};
    ImVec2 previousMousePosition_{};
    bool hasPreviousMousePosition_{};
    bool graphDirty_{};
    ea::string status_;
    ea::string graphFileName_;
};

}
