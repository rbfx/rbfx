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

#include "../Core/IniHelpers.h"
#include "../Foundation/ResourceBrowserTab.h"

#include <Urho3D/IO/FileSystem.h>

#include <EASTL/sort.h>
#include <EASTL/tuple.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_Delete = EditorHotkey{"ResourceBrowserTab.Delete"}.Press(KEY_DELETE);
const auto Hotkey_Rename = EditorHotkey{"ResourceBrowserTab.Rename"}.Press(KEY_F2);
const auto Hotkey_RevealInExplorer = EditorHotkey{"ResourceBrowserTab.RevealInExplorer"}.Alt().Shift().Press(KEY_R);

const ea::string contextMenuId = "ResourceBrowserTab_PopupDirectory";
const ea::string satelliteDirectoryExtension = ".d";

ea::optional<ea::string> TryAdjustPathOnRename(const ea::string& path, const ea::string& oldResourceName, const ea::string& newResourceName)
{
    if (path.starts_with(oldResourceName))
    {
        const ea::string pathSuffix = path.substr(oldResourceName.size());
        if (pathSuffix.empty() || pathSuffix.front() == '/')
            return newResourceName + pathSuffix;
    }
    return ea::nullopt;
}

bool IsPayloadMovable(const ResourceDragDropPayload& payload)
{
    return ea::all_of(payload.resources_.begin(), payload.resources_.end(),
        [](const ResourceFileDescriptor& desc) { return !desc.isAutomatic_; });
}

}

void Foundation_ResourceBrowserTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<ResourceBrowserTab>(context));
}

bool ResourceBrowserTab::Selection::operator==(const Selection& rhs) const
{
    return selectedRoot_ == rhs.selectedRoot_
        && selectedLeftPath_ == rhs.selectedLeftPath_
        && selectedRightPaths_ == rhs.selectedRightPaths_;
}

bool ResourceBrowserTab::EntryReference::operator<(const EntryReference& rhs) const
{
    return ea::tie(rootIndex_, resourcePath_) < ea::tie(rhs.rootIndex_, rhs.resourcePath_);
}

ResourceBrowserTab::ResourceBrowserTab(Context* context)
    : EditorTab(context, "Resources", "96c69b8e-ee83-43de-885c-8a51cef65d59",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockBottom)
{
    InitializeRoots();
    InitializeDefaultFactories();
    InitializeHotkeys();

    auto project = GetProject();
    project->OnInitialized.Subscribe(this, &ResourceBrowserTab::RefreshContents);
    project->OnRequest.SubscribeWithSender(this, &ResourceBrowserTab::OnProjectRequest);
}

void ResourceBrowserTab::InitializeRoots()
{
    auto project = GetProject();

    {
        ResourceRoot& root = roots_.emplace_back();
        root.name_ = "CoreData";
        root.watchedDirectories_ = {project->GetCoreDataPath()};
        root.activeDirectory_ = project->GetCoreDataPath();
    }

    {
        ResourceRoot& root = roots_.emplace_back();
        root.name_ = "Data";
        root.watchedDirectories_ = {project->GetDataPath(), project->GetCachePath()};
        root.activeDirectory_ = project->GetDataPath();
        root.openByDefault_ = true;
        root.supportCompositeFiles_ = true;
    }

    defaultRoot_ = 1;

    for (ResourceRoot& root : roots_)
    {
        root.reflection_ = MakeShared<FileSystemReflection>(context_, root.watchedDirectories_);
        root.reflection_->OnListUpdated.Subscribe(this, &ResourceBrowserTab::RefreshContents);
    }
}

void ResourceBrowserTab::InitializeDefaultFactories()
{
    AddFactory(MakeShared<SimpleResourceFactory>(
        context_, M_MIN_INT, ICON_FA_FOLDER " Folder", "New Folder",
        [this](const ea::string& fileName, const ea::string& resourceName)
    {
        auto fs = GetSubsystem<FileSystem>();
        fs->CreateDirsRecursive(fileName);
    }));
}

void ResourceBrowserTab::InitializeHotkeys()
{
    BindHotkey(Hotkey_Delete, &ResourceBrowserTab::DeleteSelected);
    BindHotkey(Hotkey_Rename, &ResourceBrowserTab::RenameSelected);
    BindHotkey(Hotkey_RevealInExplorer, &ResourceBrowserTab::RevealInExplorerSelected);
}

void ResourceBrowserTab::OnProjectRequest(RefCounted* sender, ProjectRequest* request)
{
    if (sender == this)
        return;

    if (const auto openResourceRequest = dynamic_cast<OpenResourceRequest*>(request))
    {
        const ResourceFileDescriptor& desc = openResourceRequest->GetResource();
        if (const FileSystemEntry* entry = FindLeftPanelEntry(desc.resourceName_))
        {
            SelectLeftPanel(entry->resourceName_);
            if (!desc.isDirectory_)
                SelectRightPanel(desc.resourceName_);
            ScrollToSelection();
        }
    }
    else if (const auto createResourceRequest = dynamic_cast<CreateResourceRequest*>(request))
    {
        if (create_.factory_ == nullptr)
        {
            if (const FileSystemEntry* entry = GetCurrentFolderEntry())
                BeginEntryCreate(*entry, createResourceRequest->GetFactory());
        }
    }
}

const FileSystemEntry* ResourceBrowserTab::FindLeftPanelEntry(const ea::string& resourceName) const
{
    const ResourceRoot& root = roots_[left_.selectedRoot_];
    const FileSystemEntry* entry = root.reflection_->FindEntry(resourceName);
    while (entry && (entry->isFile_ || IsEntryFromCache(*entry)))
        entry = entry->parent_;
    return entry;
}

ResourceBrowserTab::CachedEntryData& ResourceBrowserTab::GetCachedEntryData(const FileSystemEntry& entry) const
{
    const auto iter = cachedEntryData_.find(&entry);
    if (iter != cachedEntryData_.end())
        return iter->second;

    CachedEntryData& result = cachedEntryData_[&entry];
    result.simpleDisplayName_ = Format("{} {}", GetEntryIcon(entry, false), entry.localName_);
    result.compositeDisplayName_ = Format("{} {}", GetEntryIcon(entry, true), entry.localName_);
    result.isFileNameIgnored_ = GetProject()->IsFileNameIgnored(entry.localName_);
    return result;
}

ResourceBrowserTab::~ResourceBrowserTab()
{
}

void ResourceBrowserTab::AddFactory(SharedPtr<ResourceFactory> factory)
{
    factories_.push_back(factory);
    sortFactories_ = true;
}

ResourceBrowserTab::Selection ResourceBrowserTab::GetSelection() const
{
    Selection selection;
    selection.selectedRoot_ = left_.selectedRoot_;
    selection.selectedLeftPath_ = left_.selectedPath_;
    selection.selectedRightPaths_ = right_.selectedPaths_;
    return selection;
}

void ResourceBrowserTab::SetSelection(const Selection& selection)
{
    SelectLeftPanel(selection.selectedLeftPath_, selection.selectedRoot_);
    right_.selectedPaths_ = selection.selectedRightPaths_;
    right_.lastSelectedPath_ = right_.selectedPaths_.empty() ? "" : *right_.selectedPaths_.begin();
    OnSelectionChanged();
    ScrollToSelection();
}

void ResourceBrowserTab::DeleteSelected()
{
    // For right panel delete all the items in the selection
    if (!cursor_.isLeftPanel_)
        BeginRightSelectionDelete();
    else if (const FileSystemEntry* entry = GetSelectedEntryForCursor())
        BeginEntryDelete(*entry);
}

void ResourceBrowserTab::RenameSelected()
{
    if (const FileSystemEntry* entry = GetSelectedEntryForCursor())
        BeginEntryRename(*entry);
}

void ResourceBrowserTab::RevealInExplorerSelected()
{
    if (const FileSystemEntry* entry = GetSelectedEntryForCursor())
        RevealInExplorer(entry->absolutePath_);
}

void ResourceBrowserTab::OpenSelected()
{
    if (const FileSystemEntry* entry = GetSelectedEntryForCursor())
        OpenEntryInEditor(*entry);
}

void ResourceBrowserTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    BaseClassName::WriteIniSettings(output);

    const StringVector selectedRightPaths{right_.selectedPaths_.begin(), right_.selectedPaths_.end()};

    WriteIntToIni(output, "SelectedRoot", left_.selectedRoot_);
    WriteStringToIni(output, "SelectedLeftPath", left_.selectedPath_);
    WriteStringToIni(output, "LastSelectedRightPath", right_.lastSelectedPath_);
    WriteStringToIni(output, "SelectedRightPaths", ea::string::joined(selectedRightPaths, ";"));
}

void ResourceBrowserTab::ReadIniSettings(const char* line)
{
    BaseClassName::ReadIniSettings(line);

    if (const auto value = ReadIntFromIni(line, "SelectedRoot"))
        left_.selectedRoot_ = *value;

    if (const auto value = ReadStringFromIni(line, "SelectedLeftPath"))
        SelectLeftPanel(*value);

    if (const auto value = ReadStringFromIni(line, "LastSelectedRightPath"))
        SelectRightPanel(*value);

    if (const auto value = ReadStringFromIni(line, "SelectedRightPaths"))
    {
        const StringVector selectedPaths = value->split(';');
        right_.selectedPaths_ = {selectedPaths.begin(), selectedPaths.end()};
        OnSelectionChanged();
    }
}

void ResourceBrowserTab::ScrollToSelection()
{
    left_.scrollToSelection_ = true;
    right_.scrollToSelection_ = true;
}

void ResourceBrowserTab::RenderContent()
{
    const Selection oldSelection = GetSelection();

    for (ResourceRoot& root : roots_)
        root.reflection_->Update();

    if (waitingForUpdate_ && ui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
        ui::SetTooltip("Waiting for update...");

    static const ImVec2 workaroundPadding{0, -5};
    if (ui::BeginTable("##ResourceBrowserTab", 2, ImGuiTableFlags_Resizable))
    {
        ui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch, 0.35f);
        ui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 0.65f);

        ui::TableNextRow();

        ui::TableSetColumnIndex(0);
        if (ui::BeginChild("##DirectoryTree", ui::GetContentRegionAvail() + workaroundPadding))
        {
            for (ResourceRoot& root : roots_)
                RenderDirectoryTree(root.reflection_->GetRoot(), root.name_);
            left_.scrollToSelection_ = false;
        }
        ui::EndChild();

        ui::TableSetColumnIndex(1);
        if (ui::BeginChild("##DirectoryContent", ui::GetContentRegionAvail() + workaroundPadding))
        {
            RenderDirectoryContent();
            right_.scrollToSelection_ = false;
        }
        ui::EndChild();

        ui::EndTable();
    }

    RenderDialogs();

    if (selectionDirty_)
    {
        selectionDirty_ = false;
        Selection newSelection = GetSelection();
        if (newSelection != oldSelection)
        {
            auto undoManager = GetUndoManager();
            auto action = MakeShared<ChangeResourceSelectionAction>(this, oldSelection, newSelection);
            undoManager->PushAction(action);
        }
    }
}

void ResourceBrowserTab::RenderDialogs()
{
    if (delete_.openPending_)
        ui::OpenPopup(delete_.popupTitle_.c_str());

    if (rename_.openPending_)
        ui::OpenPopup(rename_.popupTitle_.c_str());

    if (create_.openPending_)
        ui::OpenPopup(create_.popupTitle_.c_str());

    RenderRenameDialog();
    RenderDeleteDialog();
    RenderCreateDialog();
}

void ResourceBrowserTab::RenderDirectoryTree(const FileSystemEntry& entry, const ea::string& displayedName)
{
    const auto project = GetProject();
    if (IsFileNameIgnored(entry, project, displayedName))
        return;

    const IdScopeGuard guard(displayedName.c_str());

    const unsigned rootIndex = GetRootIndex(entry);
    const ResourceRoot& root = roots_[rootIndex];
    ImGuiContext& g = *GImGui;

    // Open tree node if child is selected
    if (left_.scrollToSelection_ && rootIndex == left_.selectedRoot_
        && left_.selectedPath_.starts_with(entry.resourceName_))
    {
        if (left_.selectedPath_ != entry.resourceName_)
            ui::SetNextItemOpen(true);
        ui::SetScrollHereY();
    }

    // Render the element itself
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;

    if (IsLeafDirectory(entry))
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (entry.resourceName_ == left_.selectedPath_ && rootIndex == left_.selectedRoot_)
        flags |= ImGuiTreeNodeFlags_Selected;
    if (entry.resourceName_.empty() && root.openByDefault_)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    const bool isOpen = ui::TreeNodeEx(displayedName.c_str(), flags);

    // Process clicking
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);
    if (ui::IsItemClicked(MOUSEB_LEFT))
        SelectLeftPanel(entry.resourceName_, rootIndex);

    // Process drag&drop from this element
    if (ui::BeginDragDropSource())
    {
        BeginEntryDrag(entry);
        ui::EndDragDropSource();
    }

    // Process drag&drop to this element
    if (ui::BeginDragDropTarget())
    {
        DropPayloadToFolder(entry);
        ui::EndDragDropTarget();
    }

    // Render children
    if (isOpen)
    {
        for (const FileSystemEntry& childEntry : entry.children_)
        {
            if (IsNormalDirectory(childEntry))
                RenderDirectoryTree(childEntry, childEntry.localName_);
        }
        ui::TreePop();
    }

    if (isContextMenuOpen)
        ui::OpenPopup(contextMenuId.c_str());

    // Render context menu and popups
    RenderEntryContextMenu(entry);
}

void ResourceBrowserTab::RenderEntryContextMenuItems(const FileSystemEntry& entry)
{
    bool needSeparator = false;

    const ResourceRoot& root = GetRoot(entry);

    if (!entry.isFile_ && !IsEntryFromCache(entry))
    {
        needSeparator = true;
        if (ui::BeginMenu("Create"))
        {
            if (const auto index = RenderEntryCreateContextMenu(entry))
                BeginEntryCreate(entry, factories_[*index]);
            ui::EndMenu();
        }
    }

    if (needSeparator)
        ui::Separator();
    needSeparator = false;

    if (ui::MenuItem("Open"))
    {
        if (!entry.resourceName_.empty())
            OpenEntryInEditor(entry);
    }

    if (ui::MenuItem("Reveal in Explorer", GetHotkeyLabel(Hotkey_RevealInExplorer).c_str()))
    {
        if (entry.resourceName_.empty())
            RevealInExplorer(root.activeDirectory_);
        else
            RevealInExplorer(entry.absolutePath_);
    }

    if (ui::MenuItem("Copy Absolute Path"))
        ui::SetClipboardText(entry.absolutePath_.c_str());

    if (ui::MenuItem("Copy Relative Path (aka Resource Name)"))
        ui::SetClipboardText(entry.resourceName_.c_str());

    ui::Separator();

    const bool isEditable = !entry.resourceName_.empty() && !IsEntryFromCache(entry);
    ui::BeginDisabled(!isEditable);
    if (ui::MenuItem("Rename", GetHotkeyLabel(Hotkey_Rename).c_str()))
        BeginEntryRename(entry);

    if (ui::MenuItem("Delete", GetHotkeyLabel(Hotkey_Delete).c_str()))
        BeginEntryDelete(entry);
    ui::EndDisabled();
}

void ResourceBrowserTab::RenderEntryContextMenu(const FileSystemEntry& entry)
{
    if (ui::BeginPopup(contextMenuId.c_str()))
    {
        RenderEntryContextMenuItems(entry);
        ui::EndPopup();
    }
}

ea::optional<unsigned> ResourceBrowserTab::RenderEntryCreateContextMenu(const FileSystemEntry& entry)
{
    ea::optional<unsigned> result;

    if (sortFactories_)
    {
        ea::sort(factories_.begin(), factories_.end(), ResourceFactory::Compare);
        sortFactories_ = false;
    }

    ea::optional<int> previousGroup;
    unsigned index = 0;
    for (const ResourceFactory* factory : factories_)
    {
        const IdScopeGuard guard(index);

        if (previousGroup && *previousGroup != factory->GetGroup())
            ui::Separator();
        previousGroup = factory->GetGroup();

        const bool isEnabled = factory->IsEnabled(entry);
        ui::BeginDisabled(!isEnabled);
        if (ui::MenuItem(factory->GetTitle().c_str()))
            result = index;
        ui::EndDisabled();

        ++index;
    }

    return result;
}

void ResourceBrowserTab::RenderDirectoryContent()
{
    const ResourceRoot& root = roots_[left_.selectedRoot_];
    const FileSystemEntry* entry = root.reflection_->FindEntry(left_.selectedPath_);
    if (!entry)
        return;

    RenderCreateButton(*entry);

    if (!entry->resourceName_.empty())
        RenderDirectoryUp(*entry);

    for (const FileSystemEntry& childEntry : entry->children_)
    {
        if (!childEntry.isFile_)
            RenderDirectoryContentEntry(childEntry);
    }

    for (const FileSystemEntry& childEntry : entry->children_)
    {
        if (childEntry.isFile_)
            RenderDirectoryContentEntry(childEntry);
    }
}

void ResourceBrowserTab::RenderDirectoryUp(const FileSystemEntry& entry)
{
    const IdScopeGuard guard("..");

    // Render the element itself
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf;

    const ea::string name = Format("{} {}", ICON_FA_FOLDER_OPEN, "[..]");
    const bool isOpen = ui::TreeNodeEx(name.c_str(), flags);

    if (ui::IsItemClicked(MOUSEB_LEFT) && ui::IsMouseDoubleClicked(MOUSEB_LEFT))
    {
        auto parts = left_.selectedPath_.split('/');
        if (!parts.empty())
            parts.pop_back();

        const ea::string newSelection = ea::string::joined(parts, "/");
        SelectLeftPanel(newSelection);
        ScrollToSelection();
    }

    if (isOpen)
        ui::TreePop();

    // Process drag&drop to this element
    if (ui::BeginDragDropTarget())
    {
        URHO3D_ASSERT(entry.parent_);
        DropPayloadToFolder(*entry.parent_);
        ui::EndDragDropTarget();
    }
}

void ResourceBrowserTab::RenderDirectoryContentEntry(const FileSystemEntry& entry)
{
    const auto project = GetProject();
    if (IsFileNameIgnored(entry, project, entry.localName_))
        return;

    const IdScopeGuard guard(entry.localName_.c_str());

    ImGuiContext& g = *GImGui;
    const auto [isCompositeFile, satelliteDirectory] = IsCompositeFile(entry);
    const bool isNormalDirectory = IsNormalDirectory(entry);
    const bool isNormalFile = !entry.isDirectory_ && !isCompositeFile;
    const bool isSelected = IsRightSelected(entry.resourceName_);

    if (!isNormalDirectory && !isNormalFile && !isCompositeFile)
        return;

    // Scroll to selection if requested
    if (right_.scrollToSelection_ && isSelected)
        ui::SetScrollHereY();

    // Render the element itself
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;
    if (!isCompositeFile)
        flags |= ImGuiTreeNodeFlags_Leaf;

    const bool isOpen = ui::TreeNodeEx(GetDisplayName(entry, isCompositeFile), flags);
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);
    const bool toggleSelection = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);

    if (ui::IsItemClicked(MOUSEB_LEFT) && ui::IsItemToggledOpen())
        ignoreNextMouseRelease_ = true;

    if (ui::IsItemClicked(MOUSEB_LEFT) && ui::IsMouseDoubleClicked(MOUSEB_LEFT))
    {
        if (isNormalDirectory)
        {
            SelectLeftPanel(entry.resourceName_);
            ScrollToSelection();
        }
        else if (isNormalFile || isCompositeFile)
        {
            ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
            OpenEntryInEditor(entry);
        }
    }
    else if (ui::IsItemHovered() && ui::IsMouseReleased(MOUSEB_LEFT) && !ui::IsMouseDragPastThreshold(MOUSEB_LEFT))
    {
        if (ignoreNextMouseRelease_)
            ignoreNextMouseRelease_ = false;
        else
            ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
    }
    else if (isContextMenuOpen)
    {
        if (!IsRightSelected(entry.resourceName_))
        {
            suppressInspector_ = true;
            ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
            suppressInspector_ = false;
        }
    }

    // Process drag&drop from this element
    if (ui::BeginDragDropSource())
    {
        if (!IsRightSelected(entry.resourceName_))
        {
            suppressInspector_ = true;
            ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
            suppressInspector_ = false;
        }

        BeginRightSelectionDrag();
        ui::EndDragDropSource();
    }

    // Process drag&drop to this element only if directory
    if (isNormalDirectory && ui::BeginDragDropTarget())
    {
        DropPayloadToFolder(entry);
        ui::EndDragDropTarget();
    }

    // Render children if any
    if (isOpen)
    {
        if (isCompositeFile)
        {
            const FileSystemEntry* entries[] = {&entry, satelliteDirectory};
            RenderCompositeFile(entries);
        }
        ui::TreePop();
    }

    if (isContextMenuOpen)
        ui::OpenPopup(contextMenuId.c_str());

    // Render context menu and popups
    RenderEntryContextMenu(entry);
}

void ResourceBrowserTab::RenderCompositeFile(ea::span<const FileSystemEntry*> entries)
{
    tempEntryList_.clear();
    for (const FileSystemEntry* entry : entries)
    {
        if (!entry || !entry->isDirectory_)
            continue;

        entry->ForEach([&](const FileSystemEntry& childEntry)
        {
            if (&childEntry != entry && childEntry.isFile_)
            {
                const ea::string localResourceName = childEntry.resourceName_.substr(entry->resourceName_.size() + 1);
                tempEntryList_.push_back(TempEntry{&childEntry, localResourceName});
            }
        });
    }

    const auto compare = [](const TempEntry& lhs, const TempEntry& rhs) //
    { //
        return FileSystemEntry::ComparePathFilesFirst(lhs.localName_, rhs.localName_);
    };
    ea::sort(tempEntryList_.begin(), tempEntryList_.end(), compare);

    for (const TempEntry& item : tempEntryList_)
        RenderCompositeFileEntry(*item.entry_, item.localName_);
}

void ResourceBrowserTab::RenderCompositeFileEntry(const FileSystemEntry& entry, const ea::string& localResourceName)
{
    const auto project = GetProject();
    if (IsFileNameIgnored(entry, project, localResourceName))
        return;

    const IdScopeGuard guard(entry.resourceName_.c_str());

    // Render the element itself
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf;
    if (IsRightSelected(entry.resourceName_))
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool isOpen = ui::TreeNodeEx(GetDisplayName(entry, false), flags);
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);
    const bool toggleSelection = ui::IsKeyDown(KEY_LCTRL) || ui::IsKeyDown(KEY_RCTRL);

    if (ui::IsItemClicked(MOUSEB_LEFT) && ui::IsMouseDoubleClicked(MOUSEB_LEFT))
    {
        ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
        OpenEntryInEditor(entry);
    }
    else if (ui::IsItemHovered() && ui::IsMouseReleased(MOUSEB_LEFT) && !ui::IsMouseDragPastThreshold(MOUSEB_LEFT))
    {
        ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
    }
    else if (isContextMenuOpen)
    {
        if (!IsRightSelected(entry.resourceName_))
        {
            suppressInspector_ = true;
            ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
            suppressInspector_ = false;
        }
    }

    // Process drag&drop from this element
    if (ui::BeginDragDropSource())
    {
        if (!IsRightSelected(entry.resourceName_))
        {
            suppressInspector_ = true;
            ChangeRightPanelSelection(entry.resourceName_, toggleSelection);
            suppressInspector_ = false;
        }

        BeginRightSelectionDrag();
        ui::EndDragDropSource();
    }

    if (isOpen)
        ui::TreePop();

    if (isContextMenuOpen)
        ui::OpenPopup(contextMenuId.c_str());

    // Render context menu and popups
    RenderEntryContextMenu(entry);
}

void ResourceBrowserTab::RenderCreateButton(const FileSystemEntry& entry)
{
    static const ea::string popupId = "##CreateButtonPopup";
    ui::Indent();
    if (ui::Button(ICON_FA_SQUARE_PLUS " Create..."))
        ui::OpenPopup(popupId.c_str());
    if (ui::IsItemHovered())
        ui::SetTooltip("Create new file or directory");
    ui::Unindent();

    if (ui::BeginPopup(popupId.c_str()))
    {
        const auto createPending = RenderEntryCreateContextMenu(entry);

        if (createPending && *createPending < factories_.size())
            BeginEntryCreate(entry, factories_[*createPending]);
        ui::EndPopup();
    }
}

void ResourceBrowserTab::RenderRenameDialog()
{
    const FileSystemEntry* entry = GetEntry(rename_.entryRef_);
    if (!entry || !entry->parent_)
        return;

    if (!ui::BeginPopupModal(rename_.popupTitle_.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    const bool justOpened = rename_.openPending_;
    rename_.openPending_ = false;

    const auto [isEnabled, extraLine] = IsFileNameAvailable(
        *entry->parent_, entry->localName_, rename_.inputBuffer_);
    const char* formatString = "Would you like to rename '%s'?\n";
    ui::Text(formatString, entry->absolutePath_.c_str(), extraLine.c_str());

    if (justOpened)
        ui::SetKeyboardFocusHere();
    const bool done = ui::InputText("##Rename", &rename_.inputBuffer_,
        ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

    ui::BeginDisabled(!isEnabled);
    if (ui::Button(ICON_FA_CHECK " Rename") || (isEnabled && done))
    {
        if (rename_.inputBuffer_ != entry->localName_)
            RenameEntry(*entry, rename_.inputBuffer_);
        ui::CloseCurrentPopup();
    }
    ui::EndDisabled();

    ui::SameLine();

    if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
        ui::CloseCurrentPopup();

    ui::EndPopup();
}

void ResourceBrowserTab::RenderDeleteDialog()
{
    const auto entries = GetEntries(delete_.entryRefs_);
    if (entries.empty())
        return;

    if (!ui::BeginPopupModal(delete_.popupTitle_.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    //const bool justOpened = delete_.openPending_;
    delete_.openPending_ = false;

    ea::string displayText;
    if (entries.size() == 1)
        displayText += Format("Are you sure you want to PERMANENTLY delete '{}'?\n", entries[0]->absolutePath_);
    else
    {
        displayText += Format("Are you sure you want to PERMANENTLY delete {} items?\n\n", entries.size());
        const unsigned maxItems = ea::min<unsigned>(10u, entries.size());
        for (unsigned i = 0; i < maxItems; ++i)
            displayText += Format("* {}\n", entries[i]->absolutePath_);
        if (maxItems < entries.size())
            displayText += Format("(and {} more items)\n", entries.size() - maxItems);
        displayText += "\n";
    }
    displayText += ICON_FA_TRIANGLE_EXCLAMATION " This action cannot be undone!";
    ui::Text("%s", displayText.c_str());

    if (ui::Button(ICON_FA_CHECK " Delete") || ui::IsKeyPressed(KEY_RETURN))
    {
        for (const FileSystemEntry* entry : entries)
        {
            if (entry->parent_)
                DeleteEntry(*entry);
        }
        ui::CloseCurrentPopup();
    }

    ui::SameLine();

    if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
        ui::CloseCurrentPopup();

    ui::EndPopup();
}

void ResourceBrowserTab::RenderCreateDialog()
{
    const FileSystemEntry* parentEntry = GetEntry(create_.parentEntryRef_);
    if (!parentEntry)
        return;

    if (!ui::BeginPopupModal(create_.popupTitle_.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    const bool justOpened = create_.openPending_;
    create_.openPending_ = false;

    if (justOpened)
    {
        const ea::string baseFilePath = parentEntry->absolutePath_.empty()
            ? GetRoot(*parentEntry).activeDirectory_
            : parentEntry->absolutePath_ + "/";
        const ea::string baseResourcePath = parentEntry->resourceName_.empty()
            ? ""
            : parentEntry->resourceName_ + "/";

        create_.factory_->Open(baseFilePath, baseResourcePath);
    }

    const auto checkFileName = [this](const ea::string& filePath, const ea::string& fileName) -> ResourceFactory::CheckResult
    {
        const ea::string fullFileName = AddTrailingSlash(filePath) + fileName;

        auto fs = GetSubsystem<FileSystem>();
        if (fs->FileExists(fullFileName) || fs->DirExists(fullFileName))
            return {false, ICON_FA_TRIANGLE_EXCLAMATION " File or directory with this name already exists"};

        return IsFileNameValid(fileName);
    };

    bool canCommit{};
    bool shouldCommit{};
    create_.factory_->Render(checkFileName, canCommit, shouldCommit);

    ui::BeginDisabled(!canCommit);
    if (ui::Button(ICON_FA_CHECK " Create") || (canCommit && shouldCommit))
    {
        create_.factory_->CommitAndClose();
        create_ = {};
        ui::CloseCurrentPopup();
    }
    ui::EndDisabled();

    ui::SameLine();

    if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
    {
        create_.factory_->DiscardAndClose();
        create_ = {};
        ui::CloseCurrentPopup();
    }

    ui::EndPopup();
}

void ResourceBrowserTab::AddEntryToPayload(ResourceDragDropPayload& payload, const FileSystemEntry& entry) const
{
    auto project = GetProject();
    const auto desc = project->GetResourceDescriptor(entry.resourceName_, entry.absolutePath_);
    payload.resources_.push_back(desc);
}

SharedPtr<ResourceDragDropPayload> ResourceBrowserTab::CreatePayloadFromEntry(const FileSystemEntry& entry) const
{
    auto payload = MakeShared<ResourceDragDropPayload>();
    AddEntryToPayload(*payload, entry);
    return payload;
}

SharedPtr<ResourceDragDropPayload> ResourceBrowserTab::CreatePayloadFromRightSelection() const
{
    auto payload = MakeShared<ResourceDragDropPayload>();

    for (const ea::string& resourcePath : right_.selectedPaths_)
    {
        const FileSystemEntry* entry = GetEntry({left_.selectedRoot_, resourcePath});
        if (entry)
        {
            AddEntryToPayload(*payload, *entry);
            const auto [_, satelliteDirectory] = IsCompositeFile(*entry);
            if (satelliteDirectory && !IsEntryFromCache(*satelliteDirectory))
                AddEntryToPayload(*payload, *satelliteDirectory);
        }
    }

    // Last selected resource is the first in the payload
    const auto isLastSelected = [&](const ResourceFileDescriptor& desc) { return desc.resourceName_ == right_.lastSelectedPath_; };
    const auto lastSelectedIter = ea::find_if(payload->resources_.begin(), payload->resources_.end(), isLastSelected);
    if (lastSelectedIter != payload->resources_.end())
        ea::swap(*lastSelectedIter, payload->resources_.front());

    return payload;
}

void ResourceBrowserTab::BeginEntryDrag(const FileSystemEntry& entry)
{
    DragDropPayload::UpdateSource([&]()
    {
        return CreatePayloadFromEntry(entry);
    });
}

void ResourceBrowserTab::BeginRightSelectionDrag()
{
    DragDropPayload::UpdateSource([&]()
    {
        return CreatePayloadFromRightSelection();
    });
}

void ResourceBrowserTab::DropPayloadToFolder(const FileSystemEntry& entry)
{
    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    const ResourceRoot& root = GetRoot(entry);
    auto payload = dynamic_cast<ResourceDragDropPayload*>(DragDropPayload::Get());
    if (payload && IsPayloadMovable(*payload))
    {
        if (ui::AcceptDragDropPayload(DragDropPayloadType.c_str()))
        {
            const char* separator = entry.resourceName_.empty() ? "" : "/";
            for (const ResourceFileDescriptor& desc : payload->resources_)
            {
                const ea::string newResourceName = Format("{}{}{}", entry.resourceName_, separator, desc.localName_);
                const ea::string newFileName = Format("{}{}", root.activeDirectory_, newResourceName);
                RenameOrMove(desc.fileName_, newFileName, desc.resourceName_, newResourceName);
            }
        }
    }
}

const char* ResourceBrowserTab::GetDisplayName(const FileSystemEntry& entry, bool isCompositeFile) const
{
    CachedEntryData& cachedData = GetCachedEntryData(entry);
    return isCompositeFile ? cachedData.compositeDisplayName_.c_str() : cachedData.simpleDisplayName_.c_str();
}

bool ResourceBrowserTab::IsFileNameIgnored(const FileSystemEntry& entry, const Project* project, const ea::string& name) const
{
    CachedEntryData& cachedData = GetCachedEntryData(entry);
    return cachedData.isFileNameIgnored_;
}

const char* ResourceBrowserTab::GetEntryIcon(const FileSystemEntry& entry, bool isCompositeFile) const
{
    if (isCompositeFile)
        return ICON_FA_FILE_ZIPPER;
    else if (!entry.isFile_)
        return ICON_FA_FOLDER;
    else if (!entry.isDirectory_)
        return ICON_FA_FILE;
    else
        return ICON_FA_CIRCLE_QUESTION;
}

unsigned ResourceBrowserTab::GetRootIndex(const FileSystemEntry& entry) const
{
    FileSystemReflection* owner = entry.owner_;
    const auto iter = ea::find_if(roots_.begin(), roots_.end(),
        [owner](const ResourceRoot& root) { return root.reflection_ == owner; });
    return iter != roots_.end() ? iter - roots_.begin() : 0;
}

const ResourceBrowserTab::ResourceRoot& ResourceBrowserTab::GetRoot(const FileSystemEntry& entry) const
{
    return roots_[GetRootIndex(entry)];
}

bool ResourceBrowserTab::IsEntryFromCache(const FileSystemEntry& entry) const
{
    return entry.directoryIndex_ > 0;
}

ResourceBrowserTab::EntryReference ResourceBrowserTab::GetReference(const FileSystemEntry& entry) const
{
    return {GetRootIndex(entry), entry.resourceName_};
}

const FileSystemEntry* ResourceBrowserTab::GetEntry(const EntryReference& ref) const
{
    if (ref.rootIndex_ < roots_.size())
    {
        FileSystemReflection* reflection = roots_[ref.rootIndex_].reflection_;
        return reflection->FindEntry(ref.resourcePath_);
    }
    return nullptr;
}

const FileSystemEntry* ResourceBrowserTab::GetSelectedEntryForCursor() const
{
    return GetEntry(EntryReference{left_.selectedRoot_, cursor_.selectedPath_});
}

const FileSystemEntry* ResourceBrowserTab::GetCurrentFolderEntry() const
{
    return GetEntry(EntryReference{left_.selectedRoot_, left_.selectedPath_});
}

bool ResourceBrowserTab::IsRightSelected(const ea::string& path) const
{
    return right_.selectedPaths_.contains(path);
}

void ResourceBrowserTab::SelectLeftPanel(const ea::string& path, ea::optional<unsigned> rootIndex)
{
    const ea::string newPath = RemoveTrailingSlash(path);
    const unsigned newRoot = rootIndex.value_or(left_.selectedRoot_);

    if (newPath == left_.selectedPath_ && newRoot == left_.selectedRoot_)
        return;

    left_.selectedPath_ = RemoveTrailingSlash(path);
    left_.selectedRoot_ = rootIndex.value_or(left_.selectedRoot_);

    right_.lastSelectedPath_ = "";
    right_.selectedPaths_ = {};

    cursor_.selectedPath_ = path;
    cursor_.isLeftPanel_ = true;

    OnSelectionChanged();
}

void ResourceBrowserTab::SelectRightPanel(const ea::string& path, bool clearSelection)
{
    right_.lastSelectedPath_ = RemoveTrailingSlash(path);

    if (clearSelection)
        right_.selectedPaths_.clear();
    if (!right_.lastSelectedPath_.empty())
    {
        right_.selectedPaths_.insert(right_.lastSelectedPath_);
        cursor_.selectedPath_ = right_.lastSelectedPath_;
        cursor_.isLeftPanel_ = false;
    }

    OnSelectionChanged();
}

void ResourceBrowserTab::DeselectRightPanel(const ea::string& path)
{
    right_.selectedPaths_.erase(path);

    if (right_.lastSelectedPath_ == path)
        right_.lastSelectedPath_ = !right_.selectedPaths_.empty() ? *right_.selectedPaths_.begin() : "";

    if (cursor_.selectedPath_ == path)
        cursor_.selectedPath_ = right_.lastSelectedPath_;

    OnSelectionChanged(true);
}

void ResourceBrowserTab::ChangeRightPanelSelection(const ea::string& path, bool toggleSelection)
{
    if (toggleSelection && IsRightSelected(path))
        DeselectRightPanel(path);
    else
        SelectRightPanel(path, !toggleSelection);
}

void ResourceBrowserTab::OnSelectionChanged(bool sendEmptyEvent)
{
    selectionDirty_ = true;
    if (!suppressInspector_ && (sendEmptyEvent || !right_.selectedPaths_.empty()))
    {
        auto project = GetProject();
        auto request = MakeShared<InspectResourceRequest>(context_, StringVector{right_.selectedPaths_.begin(), right_.selectedPaths_.end()});
        project->ProcessRequest(request, this);
    }
}

void ResourceBrowserTab::AdjustSelectionOnRename(unsigned oldRootIndex, const ea::string& oldResourceName,
        unsigned newRootIndex, const ea::string& newResourceName)
{
    if (left_.selectedRoot_ != oldRootIndex)
        return;

    // Cache results because following calls may change values
    const ea::string lastSelectedRightPath = right_.lastSelectedPath_;
    StringVector selectedRightPaths{right_.selectedPaths_.begin(), right_.selectedPaths_.end()};

    if (auto newPath = TryAdjustPathOnRename(left_.selectedPath_, oldResourceName, newResourceName))
        SelectLeftPanel(*newPath, newRootIndex);

    if (auto newPath = TryAdjustPathOnRename(lastSelectedRightPath, oldResourceName, newResourceName))
        SelectRightPanel(*newPath);

    for (ea::string& selectedPath : selectedRightPaths)
    {
        if (auto newPath = TryAdjustPathOnRename(selectedPath, oldResourceName, newResourceName))
            selectedPath = *newPath;
    }
    right_.selectedPaths_ = {selectedRightPaths.begin(), selectedRightPaths.end()};

    ScrollToSelection();
}

ea::pair<bool, ea::string> ResourceBrowserTab::IsFileNameValid(const ea::string& name) const
{
    const bool isEmptyName = name.empty();
    const bool isInvalidName = GetSanitizedName(name) != name;

    if (isInvalidName)
        return {false, ICON_FA_TRIANGLE_EXCLAMATION " Name contains forbidden characters"};
    else if (isEmptyName)
        return {false, ICON_FA_TRIANGLE_EXCLAMATION " Name must not be empty"};
    else
        return {true, ICON_FA_CIRCLE_CHECK " Name is OK"};
}

ea::pair<bool, ea::string> ResourceBrowserTab::IsFileNameAvailable(
    const FileSystemEntry& parentEntry, const ea::string& oldName, const ea::string& newName) const
{
    const bool isUsedName = newName != oldName && parentEntry.FindChild(newName);
    if (isUsedName)
        return {false, ICON_FA_TRIANGLE_EXCLAMATION " File or directory with this name already exists"};

    return IsFileNameValid(newName);
}

ea::vector<const FileSystemEntry*> ResourceBrowserTab::GetEntries(const ea::vector<EntryReference>& refs) const
{
    ea::vector<const FileSystemEntry*> result;
    for (const EntryReference& entryRef : delete_.entryRefs_)
    {
        const FileSystemEntry* entry = GetEntry(entryRef);
        if (entry)
            result.push_back(entry);
    }
    return result;
}

ea::optional<unsigned> ResourceBrowserTab::GetRootIndex(const ea::string& fileName) const
{
    for (unsigned rootIndex = 0; rootIndex < roots_.size(); ++rootIndex)
    {
        const ResourceRoot& root = roots_[rootIndex];
        for (const ea::string& rootPath : root.watchedDirectories_)
        {
            if (fileName.starts_with(rootPath))
                return rootIndex;
        }
    }
    return ea::nullopt;
}

bool ResourceBrowserTab::IsNormalDirectory(const FileSystemEntry& entry) const
{
    if (entry.isFile_)
        return false;
    if (entry.parent_ && entry.resourceName_.ends_with(satelliteDirectoryExtension))
    {
        const ea::string primaryFileName =
            entry.localName_.substr(0, entry.localName_.size() - satelliteDirectoryExtension.size());
        const FileSystemEntry* primaryFileEntry = entry.parent_->FindChild(primaryFileName);
        if (primaryFileEntry && primaryFileEntry->isFile_)
            return false;
    }
    return true;
}

bool ResourceBrowserTab::IsLeafDirectory(const FileSystemEntry& entry) const
{
    for (const FileSystemEntry& childEntry : entry.children_)
    {
        if (IsNormalDirectory(childEntry))
            return false;
    }
    return true;
}

ea::pair<bool, const FileSystemEntry*> ResourceBrowserTab::IsCompositeFile(const FileSystemEntry& entry) const
{
    const ResourceRoot& root = GetRoot(entry);
    if (!root.supportCompositeFiles_ || !entry.isFile_)
        return {false, nullptr};

    const FileSystemEntry* satelliteDirectoryEntry = nullptr;
    if (entry.parent_)
    {
        const FileSystemEntry* otherEntry = entry.parent_->FindChild(entry.localName_ + satelliteDirectoryExtension);
        if (otherEntry && otherEntry->isDirectory_)
            satelliteDirectoryEntry = otherEntry;
    }

    const bool isCompositeFile = satelliteDirectoryEntry || entry.isDirectory_;
    return {isCompositeFile, satelliteDirectoryEntry};
}

void ResourceBrowserTab::BeginEntryDelete(const FileSystemEntry& entry)
{
    delete_.entryRefs_ = {GetReference(entry)};
    delete_.popupTitle_ = Format("Delete '{}'?##DeleteDialog", entry.localName_);
    delete_.openPending_ = true;
}

void ResourceBrowserTab::BeginRightSelectionDelete()
{
    delete_.entryRefs_.clear();
    ea::transform(right_.selectedPaths_.begin(), right_.selectedPaths_.end(), std::back_inserter(delete_.entryRefs_),
        [this](const ea::string& resourcePath) { return EntryReference{left_.selectedRoot_, resourcePath}; });
    ea::sort(delete_.entryRefs_.begin(), delete_.entryRefs_.end());

    delete_.popupTitle_ = Format("Delete {} items?##DeleteDialog", right_.selectedPaths_.size());
    delete_.openPending_ = true;
}

void ResourceBrowserTab::BeginEntryRename(const FileSystemEntry& entry)
{
    rename_.entryRef_ = GetReference(entry);
    rename_.popupTitle_ = Format("Rename '{}'?##RenameDialog", entry.localName_);
    rename_.inputBuffer_ = entry.localName_;
    rename_.openPending_ = true;
}

void ResourceBrowserTab::BeginEntryCreate(const FileSystemEntry& entry, ResourceFactory* factory)
{
    if (entry.isFile_)
        return;

    create_.parentEntryRef_ = GetReference(entry);
    create_.popupTitle_ = Format("Create {}...##CreateDialog", factory->GetTitle());
    create_.factory_ = factory;
    create_.openPending_ = true;
}

void ResourceBrowserTab::RefreshContents()
{
    ScrollToSelection();
    waitingForUpdate_ = false;
    cachedEntryData_.clear();
}

void ResourceBrowserTab::RevealInExplorer(const ea::string& path)
{
    auto fs = GetSubsystem<FileSystem>();
    fs->Reveal(path);
}

void ResourceBrowserTab::RenameEntry(const FileSystemEntry& entry, const ea::string& newName)
{
    const ea::string newFileName = GetPath(entry.absolutePath_) + newName;
    const ea::string newResourceName = GetPath(entry.resourceName_) + newName;
    RenameOrMove(entry.absolutePath_, newFileName, entry.resourceName_, newResourceName);

    const auto [_, satelliteDirectory] = IsCompositeFile(entry);
    if (satelliteDirectory && !IsEntryFromCache(*satelliteDirectory))
        RenameEntry(*satelliteDirectory, newName + satelliteDirectoryExtension);
}

void ResourceBrowserTab::RenameOrMove(const ea::string& oldFileName, const ea::string& newFileName,
    const ea::string& oldResourceName, const ea::string& newResourceName, bool suppressUndo)
{
    if (oldFileName == newFileName)
        return;

    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    const unsigned oldRootIndex = GetRootIndex(oldFileName).value_or(0);
    const unsigned newRootIndex = GetRootIndex(newFileName).value_or(0);

    const bool isFile = fs->FileExists(oldFileName);

    const bool renamed = fs->Rename(oldFileName, newFileName);
    if (renamed)
    {
        // Show tooltip if waiting for refresh
        waitingForUpdate_ = true;

        // Keep selection on dragged element
        AdjustSelectionOnRename(oldRootIndex, oldResourceName, newRootIndex, newResourceName);

        // If file is moved and there's directory in cache with the same name, remove it
        if (isFile)
            CleanupResourceCache(oldResourceName);

        if (!suppressUndo)
        {
            auto undoManager = GetUndoManager();
            auto action = MakeShared<RenameResourceAction>(this, oldFileName, newFileName, oldResourceName, newResourceName);
            undoManager->PushAction(action);
        }
    }
}

void ResourceBrowserTab::DeleteEntry(const FileSystemEntry& entry)
{
    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    DeselectRightPanel(entry.resourceName_);

    const bool isFile = fs->FileExists(entry.absolutePath_);

    const bool deleted = isFile
        ? fs->Delete(entry.absolutePath_)
        : fs->RemoveDir(entry.absolutePath_, true);

    if (deleted)
    {
        waitingForUpdate_ = true;

        if (isFile)
            CleanupResourceCache(entry.resourceName_);
    }

    const auto [_, satelliteDirectory] = IsCompositeFile(entry);
    if (satelliteDirectory && !IsEntryFromCache(*satelliteDirectory))
        DeleteEntry(*satelliteDirectory);
}

void ResourceBrowserTab::CleanupResourceCache(const ea::string& resourceName)
{
    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    const ea::string matchingDirectoryInCache = project->GetCachePath() + resourceName;
    if (fs->DirExists(matchingDirectoryInCache))
        fs->RemoveDir(matchingDirectoryInCache, true);
}

void ResourceBrowserTab::OpenEntryInEditor(const FileSystemEntry& entry)
{
    auto project = GetProject();

    const auto request = MakeShared<OpenResourceRequest>(context_, entry.resourceName_);
    request->QueueProcessCallback([=]()
    {
        auto fs = GetSubsystem<FileSystem>();
        fs->SystemOpen(entry.absolutePath_);
    }, M_MIN_INT);

    project->ProcessRequest(request, this);
}

void ResourceBrowserTab::RenderContextMenuItems()
{
    const FileSystemEntry* entry = GetSelectedEntryForCursor();
    if (!entry)
        entry = &roots_[defaultRoot_].reflection_->GetRoot();

    RenderEntryContextMenuItems(*entry);
}

ChangeResourceSelectionAction::ChangeResourceSelectionAction(ResourceBrowserTab* tab,
    const ResourceBrowserTab::Selection& oldSelection, const ResourceBrowserTab::Selection& newSelection)
    : tab_(tab)
    , oldSelection_(oldSelection)
    , newSelection_(newSelection)
{
}

void ChangeResourceSelectionAction::Redo() const
{
    if (tab_)
        tab_->SetSelection(newSelection_);
}

void ChangeResourceSelectionAction::Undo() const
{
    if (tab_)
        tab_->SetSelection(oldSelection_);
}

bool ChangeResourceSelectionAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const ChangeResourceSelectionAction*>(&other);
    if (!otherAction)
        return false;

    newSelection_ = otherAction->newSelection_;
    return true;
}

RenameResourceAction::RenameResourceAction(ResourceBrowserTab* tab,
    const ea::string& oldFileName, const ea::string& newFileName,
    const ea::string& oldResourceName, const ea::string& newResourceName)
    : tab_(tab)
    , oldFileName_(oldFileName)
    , newFileName_(newFileName)
    , oldResourceName_(oldResourceName)
    , newResourceName_(newResourceName)
{
}

bool RenameResourceAction::CanRedo() const
{
    return CanRenameTo(newFileName_);
}

void RenameResourceAction::Redo() const
{
    tab_->RenameOrMove(oldFileName_, newFileName_, oldResourceName_, newResourceName_, true);
}

bool RenameResourceAction::CanUndo() const
{
    return CanRenameTo(oldFileName_);
}

void RenameResourceAction::Undo() const
{
    tab_->RenameOrMove(newFileName_, oldFileName_, newResourceName_, oldResourceName_, true);
}

bool RenameResourceAction::CanRenameTo(const ea::string& fileName) const
{
    if (tab_)
    {
        if (Context* context = tab_->GetContext())
        {
            auto fs = context->GetSubsystem<FileSystem>();
            return !fs->FileExists(fileName) && !fs->DirExists(fileName);
        }
    }
    return false;
}

}
