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

#include "../Core/CommonEditorActions.h"
#include "../Project/EditorTab.h"
#include "../Project/Project.h"
#include "../Project/ResourceFactory.h"

#include <Urho3D/Core/Signal.h>
#include <Urho3D/SystemUI/DragDropPayload.h>
#include <Urho3D/Utility/FileSystemReflection.h>

#include <EASTL/functional.h>
#include <EASTL/optional.h>

namespace Urho3D
{

void Foundation_ResourceBrowserTab(Context* context, Project* project);

class ResourceBrowserTab : public EditorTab
{
    URHO3D_OBJECT(ResourceBrowserTab, EditorTab);

public:
    struct Selection
    {
        unsigned selectedRoot_{1};
        ea::string selectedLeftPath_;
        ea::unordered_set<ea::string> selectedRightPaths_;

        bool operator==(const Selection& rhs) const;
        bool operator!=(const Selection& rhs) const { return !(*this == rhs); }
    };

    explicit ResourceBrowserTab(Context* context);
    ~ResourceBrowserTab() override;

    void AddFactory(SharedPtr<ResourceFactory> factory);

    Selection GetSelection() const;
    void SetSelection(const Selection& selection);
    void RenameOrMove(const ea::string& oldFileName, const ea::string& newFileName,
        const ea::string& oldResourceName, const ea::string& newResourceName, bool suppressUndo = false);

    /// Commands
    /// @{
    void DeleteSelected();
    void RenameSelected();
    void RevealInExplorerSelected();
    void OpenSelected();
    /// @}

    /// Implement EditorTab
    /// @{
    void RenderContent() override;
    void RenderContextMenuItems() override;

    bool IsUndoSupported() override { return true; }
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}


private:
    /// Root index and resource name used to safely reference an entry.
    struct EntryReference
    {
        unsigned rootIndex_{};
        ea::string resourcePath_;

        bool operator<(const EntryReference& rhs) const;
    };

    struct ResourceRoot
    {
        ea::string name_;
        bool openByDefault_{};
        bool supportCompositeFiles_{};
        StringVector watchedDirectories_;
        ea::string activeDirectory_;

        SharedPtr<FileSystemReflection> reflection_;
    };

    void InitializeRoots();
    void InitializeDefaultFactories();
    void InitializeHotkeys();

    void OnProjectRequest(RefCounted* sender, ProjectRequest* request);
    const FileSystemEntry* FindLeftPanelEntry(const ea::string& resourceName) const;

    /// Render left panel
    /// @{
    void RenderDirectoryTree(const FileSystemEntry& entry, const ea::string& displayedName);
    /// @}

    /// Render right panel
    /// @{
    void RenderDirectoryContent();
    void RenderDirectoryUp(const FileSystemEntry& entry);
    void RenderDirectoryContentEntry(const FileSystemEntry& entry);
    void RenderCompositeFile(ea::span<const FileSystemEntry*> entries);
    void RenderCompositeFileEntry(const FileSystemEntry& entry, const ea::string& localResourceName);
    void RenderCreateButton(const FileSystemEntry& entry);
    /// @}

    /// Common rendering
    /// @{
    void RenderDialogs();

    void RenderEntryContextMenu(const FileSystemEntry& entry);
    void RenderEntryContextMenuItems(const FileSystemEntry& entry);
    ea::optional<unsigned> RenderEntryCreateContextMenu(const FileSystemEntry& entry);

    void RenderRenameDialog();
    void RenderDeleteDialog();
    void RenderCreateDialog();
    /// @}

    /// Drag&drop handling
    /// @{
    void AddEntryToPayload(ResourceDragDropPayload& payload, const FileSystemEntry& entry) const;
    SharedPtr<ResourceDragDropPayload> CreatePayloadFromEntry(const FileSystemEntry& entry) const;
    SharedPtr<ResourceDragDropPayload> CreatePayloadFromRightSelection() const;

    void BeginEntryDrag(const FileSystemEntry& entry);
    void BeginRightSelectionDrag();
    void DropPayloadToFolder(const FileSystemEntry& entry);
    /// @}

    /// Utility functions
    /// @{
    ea::string GetEntryIcon(const FileSystemEntry& entry, bool isCompositeFile) const;
    unsigned GetRootIndex(const FileSystemEntry& entry) const;
    const ResourceRoot& GetRoot(const FileSystemEntry& entry) const;
    bool IsEntryFromCache(const FileSystemEntry& entry) const;
    EntryReference GetReference(const FileSystemEntry& entry) const;
    const FileSystemEntry* GetEntry(const EntryReference& ref) const;
    const FileSystemEntry* GetSelectedEntryForCursor() const;
    const FileSystemEntry* GetCurrentFolderEntry() const;
    ea::pair<bool, ea::string> IsFileNameValid(const ea::string& name) const;
    ea::pair<bool, ea::string> IsFileNameAvailable(const FileSystemEntry& parentEntry,
        const ea::string& oldName, const ea::string& newName) const;
    ea::vector<const FileSystemEntry*> GetEntries(const ea::vector<EntryReference>& refs) const;
    ea::optional<unsigned> GetRootIndex(const ea::string& fileName) const;
    bool IsNormalDirectory(const FileSystemEntry& entry) const;
    bool IsLeafDirectory(const FileSystemEntry& entry) const;
    ea::pair<bool, const FileSystemEntry*> IsCompositeFile(const FileSystemEntry& entry) const;
    /// @}

    /// Manage selection
    /// @{
    bool IsRightSelected(const ea::string& path) const;
    void SelectLeftPanel(const ea::string& path, ea::optional<unsigned> rootIndex = ea::nullopt);
    void SelectRightPanel(const ea::string& path, bool clearSelection = true);
    void DeselectRightPanel(const ea::string& path);
    void ChangeRightPanelSelection(const ea::string& path, bool toggleSelection);
    void AdjustSelectionOnRename(unsigned oldRootIndex, const ea::string& oldResourceName,
        unsigned newRootIndex, const ea::string& newResourceName);
    void ScrollToSelection();
    void OnSelectionChanged(bool sendEmptyEvent = false);
    /// @}

    /// Manipulation utilities and helpers
    /// @{
    void BeginEntryDelete(const FileSystemEntry& entry);
    void BeginRightSelectionDelete();
    void BeginEntryRename(const FileSystemEntry& entry);
    void BeginEntryCreate(const FileSystemEntry& entry, ResourceFactory* factory);

    void RefreshContents();
    void RevealInExplorer(const ea::string& path);
    void RenameEntry(const FileSystemEntry& entry, const ea::string& newName);
    void DeleteEntry(const FileSystemEntry& entry);
    void CleanupResourceCache(const ea::string& resourceName);
    void OpenEntryInEditor(const FileSystemEntry& entry);
    /// @}

    ea::vector<ResourceRoot> roots_;
    unsigned defaultRoot_{};
    bool waitingForUpdate_{};

    ea::vector<SharedPtr<ResourceFactory>> factories_;
    bool sortFactories_{true};

    bool suppressInspector_{};
    bool ignoreNextMouseRelease_{};

    /// UI state
    /// @{
    struct LeftPanel
    {
        unsigned selectedRoot_{1};
        ea::string selectedPath_;

        bool scrollToSelection_{};
    } left_;

    struct RightPanel
    {
        ea::string lastSelectedPath_;
        ea::unordered_set<ea::string> selectedPaths_;

        bool scrollToSelection_{};
    } right_;

    struct CursorForHotkeys
    {
        ea::string selectedPath_{};
        bool isLeftPanel_{};
    } cursor_;

    struct RenameDialog
    {
        EntryReference entryRef_;
        ea::string popupTitle_;
        ea::string inputBuffer_;
        bool openPending_{};
    } rename_;

    struct DeleteDialog
    {
        ea::vector<EntryReference> entryRefs_;
        ea::string popupTitle_;
        bool openPending_{};
    } delete_;

    struct CreateDialog
    {
        EntryReference parentEntryRef_;
        ea::string popupTitle_;
        SharedPtr<ResourceFactory> factory_;
        bool openPending_{};
    } create_;
    /// @}

    struct TempEntry
    {
        const FileSystemEntry* entry_{};
        ea::string localName_;
    };
    ea::vector<TempEntry> tempEntryList_;
    bool selectionDirty_{};
};

class ChangeResourceSelectionAction : public EditorAction
{
public:
    ChangeResourceSelectionAction(ResourceBrowserTab* tab,
        const ResourceBrowserTab::Selection& oldSelection, const ResourceBrowserTab::Selection& newSelection);

    /// Implement EditorAction
    /// @{
    bool IsTransparent() const override { return true; }
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    WeakPtr<ResourceBrowserTab> tab_;
    ResourceBrowserTab::Selection oldSelection_;
    ResourceBrowserTab::Selection newSelection_;
};

class RenameResourceAction : public EditorAction
{
public:
    RenameResourceAction(ResourceBrowserTab* tab,
        const ea::string& oldFileName, const ea::string& newFileName,
        const ea::string& oldResourceName, const ea::string& newResourceName);

    /// Implement EditorAction
    /// @{
    bool CanRedo() const override;
    void Redo() const override;
    bool CanUndo() const override;
    void Undo() const override;
    /// @}

private:
    bool CanRenameTo(const ea::string& fileName) const;

    WeakPtr<ResourceBrowserTab> tab_;
    ea::string oldFileName_;
    ea::string newFileName_;
    ea::string oldResourceName_;
    ea::string newResourceName_;
};

}
