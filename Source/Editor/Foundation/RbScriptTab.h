// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include "../Project/ResourceEditorTab.h"
#include "../Project/Project.h"

#include <Urho3D/RbScript/RbScriptDefs.h>
#include <Urho3D/RbScript/RbScriptResource.h>
#include <Urho3D/RbScript/RbScriptType.h>
#include <Urho3D/RbScript/RbScriptVM.h>

namespace Urho3D
{

/// Register the rbscript source editor in a project.
void Foundation_RbScriptTab(Context* context, Project* project);

/// Native rbscript source editor with lexical highlighting, diagnostics and resource lifecycle integration.
class RbScriptTab : public ResourceEditorTab
{
    URHO3D_OBJECT(RbScriptTab, ResourceEditorTab);

public:
    explicit RbScriptTab(Context* context);

    /// ResourceEditorTab implementation.
    /// @{
    void RenderContent() override;
    void RenderToolbar() override;
    void RenderContextMenuItems() override;
    bool CanOpenResource(const ResourceFileDescriptor& desc) override;
    bool SupportMultipleResources() override { return true; }
    ea::string GetResourceTitle() override { return "rbscript"; }
    bool IsUndoSupported() override { return true; }
    /// Apply an undo/redo source snapshot.
    void ApplySourceSnapshot(const ea::string& source);
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

protected:
    /// ResourceEditorTab callbacks.
    /// @{
    void OnResourceLoaded(const ea::string& resourceName) override;
    void OnResourceUnloaded(const ea::string& resourceName) override;
    void OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName) override;
    void OnResourceSaved(const ea::string& resourceName) override;
    void OnResourceShallowSaved(const ea::string& resourceName) override;
    /// @}

private:
    struct Document
    {
        ea::string source;
        ea::vector<RbScriptToken> tokens;
        ea::vector<RbScriptDiagnostic> diagnostics;
        RbScriptChunk chunk;
        RbScriptVM debugVm;
        bool compiled{false};
        bool showPreview{true};
    };

    void LoadDocument(const ea::string& resourceName);
    void RefreshDocument(const ea::string& resourceName, bool compile);
    void ApplySource(const ea::string& resourceName, const ea::string& source, bool compile);
    void SaveDocument(const ea::string& resourceName);
    void CompileActiveDocument();
    void RenderDiagnostics(const Document& document);
    void RenderDebugPanel(Document& document);
    void RenderTokenPreview(const Document& document);
    void RenderAutocomplete(const Document& document);
    void TokenizeDocument(Document& document);
    Document* GetActiveDocument();
    const Document* GetActiveDocument() const;
    ea::string GetActiveSource() const;
    void SetActiveSource(const ea::string& source);
    void PushSourceEdit(const ea::string& before, const ea::string& after);
    ea::map<ea::string, Document> documents_;
    ea::string activeSource_;
    ea::string status_;
    ea::string searchText_;
    int selectedDiagnostic_{-1};
    bool showDiagnostics_{true};
    bool showPreview_{true};
    bool autoCompile_{true};
    bool sourceFocused_{false};
    unsigned breakpointLine_{1};
    RbScriptTypeRegistry typeRegistry_;
};

} // namespace Urho3D
