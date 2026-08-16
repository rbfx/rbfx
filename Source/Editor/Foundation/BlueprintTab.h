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
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    bool IsUndoSupported() override { return true; }

    /// Apply a serialized graph snapshot during undo/redo.
    void ApplyGraphSnapshot(const ea::string& snapshot);

    /// @}

private:
    void CreateDemoGraph();
    void AddNodeFromToolbar(const ea::string& typeName);
    void RenderGraphCanvas();
    void RenderNode(const BlueprintNode& node, const ImVec2& canvasOrigin, ImDrawList* drawList);
    void RenderLinks(const ImVec2& canvasOrigin, ImDrawList* drawList);
    void RenderDiagnostics();
    void RenderTypePanels();
    void RenderNodePalette();
    void RenderCanvasContextMenu();
    void RenderNodeContextMenu();
    void RenderLinkPreview(const ImVec2& canvasOrigin, ImDrawList* drawList);
    void RenderSelectionOverlay(const ImVec2& canvasOrigin, ImDrawList* drawList);
    void RenderMinimap(const ImVec2& canvasOrigin, const ImVec2& canvasSize);
    void RenderComments(const ImVec2& canvasOrigin, ImDrawList* drawList);
    void RenderDebugToolbar();
    void RenderWatchWindow();
    void PerformAutoLayout();
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
    BlueprintPin* FindPinAt(const Vector2& graphPosition, BlueprintNode*& node);
    void BeginGraphEdit();
    void CommitGraphEdit(const ea::string& status = ea::string());
    void DeleteSelected();
    void CopySelection();
    void CutSelection();
    void PasteSelection();
    void DuplicateSelection();
    void ToggleBreakpoint();
    bool IsSelected(BlueprintId nodeId) const;
    void SelectNode(BlueprintId nodeId, bool toggle);
    ea::string SerializeSelection() const;
    bool DeserializeSelection(const ea::string& serialized, const Vector2& offset);

    BlueprintGraph graph_;
    BlueprintRuntime runtime_;
    BlueprintId selectedNode_{BLUEPRINT_INVALID_ID};
    BlueprintId draggingNode_{BLUEPRINT_INVALID_ID};
    BlueprintId contextNode_{BLUEPRINT_INVALID_ID};
    Vector2 contextGraphPosition_{Vector2::ZERO};
    BlueprintId linkingNode_{BLUEPRINT_INVALID_ID};
    ea::string linkingPin_;
    bool linkingFromOutput_{};
    ea::vector<BlueprintId> selectedNodes_;
    ea::string clipboard_;
    ea::string graphEditSnapshot_;
    Vector2 pan_{Vector2::ZERO};
    float zoom_{1.0f};
    ImVec2 previousMousePosition_{};
    bool hasPreviousMousePosition_{};
    bool graphDirty_{};
    ea::string status_;
    ea::string graphFileName_;
    ea::string nodeSearch_;
    ea::vector<BlueprintId> searchResults_;
    ea::vector<BlueprintId> breakpoints_;
    BlueprintId debugCurrentNode_{BLUEPRINT_INVALID_ID};
    bool showMinimap_{true};
    bool showComments_{true};
    bool debugPaused_{};
    bool showWatchWindow_{true};
    bool contextMenuRequested_{};
    bool nodeContextMenuRequested_{};
    bool showTypePanels_{true};
    ea::string newStructName_;
    ea::string newEnumName_;
    ea::string newDelegateName_;
    ea::string newTimelineName_;
};

}
