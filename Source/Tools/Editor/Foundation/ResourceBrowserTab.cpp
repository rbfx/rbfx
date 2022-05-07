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

#include "../Foundation/ResourceBrowserTab.h"

#include <Urho3D/IO/FileSystem.h>

#include <EASTL/sort.h>
#include <EASTL/tuple.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const ea::string contextMenuId = "ResourceBrowserTab_PopupDirectory";

bool IsLeafDirectory(const FileSystemEntry& entry)
{
    for (const FileSystemEntry& childEntry : entry.children_)
    {
        if (!childEntry.isFile_)
            return false;
    }
    return true;
}

}

void Foundation_ResourceBrowserTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<ResourceBrowserTab>(context));
    // TODO(editor): Remove this
    projectEditor->AddTab(MakeShared<EditorTab>(context, "Scene", "1",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter));
    projectEditor->AddTab(MakeShared<EditorTab>(context, "Hierarchy", "2",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockLeft));
    projectEditor->AddTab(MakeShared<EditorTab>(context, "Inspector", "3",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockRight));
}

ResourceBrowserFactory::ResourceBrowserFactory(Context* context,
    int group, const ea::string& title, const ea::string& fileName)
    : Object(context)
    , group_(group)
    , title_(title)
    , fileName_(fileName)
{
}

ResourceBrowserFactory::ResourceBrowserFactory(Context* context,
    int group, const ea::string& title, const ea::string& fileName, const Callback& callback)
    : ResourceBrowserFactory(context, group, title, fileName)
{
    callback_ = callback;
}

void ResourceBrowserFactory::EndCreate(const ea::string& fileName)
{
    if (callback_)
        callback_(fileName);
}

bool ResourceBrowserFactory::Compare(
    const SharedPtr<ResourceBrowserFactory>& lhs, const SharedPtr<ResourceBrowserFactory>& rhs)
{
    return ea::tie(lhs->group_, lhs->title_) < ea::tie(rhs->group_, rhs->title_);
}

ResourceBrowserTab::ResourceBrowserTab(Context* context)
    : EditorTab(context, "Resource Browser", "96c69b8e-ee83-43de-885c-8a51cef65d59",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockBottom)
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

    {
        AddFactory(MakeShared<ResourceBrowserFactory>(
            context_, M_MIN_INT, ICON_FA_FOLDER " Folder", "New Folder",
            [this](const ea::string& fileName)
        {
            auto fs = GetSubsystem<FileSystem>();
            fs->CreateDirsRecursive(fileName);
        }));
    }

    for (ResourceRoot& root : roots_)
    {
        root.reflection_ = MakeShared<FileSystemReflection>(context_, root.watchedDirectories_);
        root.reflection_->OnListUpdated.Subscribe(this, &ResourceBrowserTab::RefreshContents);
    }
}

ResourceBrowserTab::~ResourceBrowserTab()
{
}

void ResourceBrowserTab::AddFactory(SharedPtr<ResourceBrowserFactory> factory)
{
    factories_.push_back(factory);
    sortFactories_ = true;
}

void ResourceBrowserTab::ScrollToSelection()
{
    left_.scrollToSelection_ = true;
    right_.scrollToSelection_ = true;
}

void ResourceBrowserTab::RenderContentUI()
{
    for (ResourceRoot& root : roots_)
        root.reflection_->Update();

    if (waitingForUpdate_ && ui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
        ui::SetTooltip("%s", "Waiting for update...");

    if (ui::BeginTable("##ResourceBrowserTab", 2, ImGuiTableFlags_Resizable))
    {
        ui::TableSetupColumn("Left", ImGuiTableColumnFlags_WidthStretch, 0.35f);
        ui::TableSetupColumn("Right", ImGuiTableColumnFlags_WidthStretch, 0.65f);

        ui::TableNextRow();

        ui::TableSetColumnIndex(0);
        if (ui::BeginChild("##DirectoryTree", ui::GetContentRegionAvail()))
        {
            for (ResourceRoot& root : roots_)
                RenderDirectoryTree(root.reflection_->GetRoot(), root.name_);
            left_.scrollToSelection_ = false;
        }
        ui::EndChild();

        ui::TableSetColumnIndex(1);
        if (ui::BeginChild("##DirectoryContent", ui::GetContentRegionAvail()))
        {
            RenderDirectoryContent();
            right_.scrollToSelection_ = false;
        }
        ui::EndChild();

        ui::EndTable();
    }
}

void ResourceBrowserTab::RenderDirectoryTree(const FileSystemEntry& entry, const ea::string& displayedName)
{
    ui::PushID(displayedName.c_str());

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
            if (!childEntry.isFile_)
                RenderDirectoryTree(childEntry, childEntry.localName_);
        }
        ui::TreePop();
    }

    if (isContextMenuOpen)
        ui::OpenPopup(contextMenuId.c_str());

    // Render context menu and popups
    RenderEntryContextMenu(entry);
    RenderRenameDialog(entry);
    RenderDeleteDialog(entry);
    RenderCreateDialog(entry);

    ui::PopID();
}

void ResourceBrowserTab::RenderEntryContextMenu(const FileSystemEntry& entry)
{
    bool renamePending = false;
    bool deletePending = false;
    ea::optional<unsigned> createPending;

    if (ui::BeginPopup(contextMenuId.c_str()))
    {
        bool needSeparator = false;

        const ResourceRoot& root = GetRoot(entry);

        if (!entry.isFile_ && !IsEntryFromCache(entry))
        {
            needSeparator = true;
            if (ui::BeginMenu("Create"))
            {
                createPending = RenderEntryCreateContextMenu(entry);
                ui::EndMenu();
            }
        }

        if (needSeparator)
            ui::Separator();
        needSeparator = false;

        if (ui::MenuItem("Reveal in Explorer"))
        {
            if (entry.resourceName_.empty())
                RevealInExplorer(root.activeDirectory_);
            else
                RevealInExplorer(entry.absolutePath_);
        }

        if (!entry.resourceName_.empty() && !IsEntryFromCache(entry))
        {
            if (ui::MenuItem("Rename"))
                renamePending = true;

            if (ui::MenuItem("Delete"))
                deletePending = true;
        }

        ui::EndPopup();
    }

    if (renamePending)
    {
        renamePopupTitle_ = Format("Rename '{}'?", entry.localName_);
        renameBuffer_ = entry.localName_;
        ui::OpenPopup(renamePopupTitle_.c_str());
    }

    if (deletePending)
    {
        deletePopupTitle_ = Format("Delete '{}'?", entry.localName_);
        ui::OpenPopup(deletePopupTitle_.c_str());
    }

    if (createPending && *createPending < factories_.size())
    {
        ResourceBrowserFactory* factory = factories_[*createPending];
        createPopupTitle_ = Format("Create {}...", factory->GetTitle());
        createFactory_ = factory;
        createNameBuffer_ = factory->GetFileName();
        factory->BeginCreate();
        ui::OpenPopup(createPopupTitle_.c_str());
    }
}

ea::optional<unsigned> ResourceBrowserTab::RenderEntryCreateContextMenu(const FileSystemEntry& entry)
{
    ea::optional<unsigned> result;

    if (sortFactories_)
    {
        ea::sort(factories_.begin(), factories_.end(), ResourceBrowserFactory::Compare);
        sortFactories_ = false;
    }

    ea::optional<int> previousGroup;
    unsigned index = 0;
    for (const ResourceBrowserFactory* factory : factories_)
    {
        ui::PushID(index);

        if (previousGroup && *previousGroup != factory->GetGroup())
            ui::Separator();
        previousGroup = factory->GetGroup();

        const bool isEnabled = factory->IsEnabled(entry);
        ui::BeginDisabled(!isEnabled);
        if (ui::MenuItem(factory->GetTitle().c_str()))
            result = index;
        ui::EndDisabled();

        ui::PopID();
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
    ui::PushID("..");

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

    ui::PopID();
}

void ResourceBrowserTab::RenderDirectoryContentEntry(const FileSystemEntry& entry)
{
    ui::PushID(entry.localName_.c_str());

    ImGuiContext& g = *GImGui;
    const ResourceRoot& root = GetRoot(entry);
    const bool isNormalDirectory = !entry.isFile_;
    const bool isCompositeFile = root.supportCompositeFiles_ && entry.isFile_ && entry.isDirectory_;
    const bool isSelected = entry.resourceName_ == right_.selectedPath_;

    // Scroll to selection if requested
    if (right_.scrollToSelection_ && isSelected)
        ui::SetScrollHereY();

    // Render the element itself
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;
    flags |= isCompositeFile ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_Leaf;

    const ea::string name = Format("{} {}", GetEntryIcon(entry), entry.localName_);
    const bool isOpen = ui::TreeNodeEx(name.c_str(), flags);
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);

    if (ui::IsItemClicked(MOUSEB_LEFT))
    {
        if (isNormalDirectory && ui::IsMouseDoubleClicked(MOUSEB_LEFT))
        {
            SelectLeftPanel(entry.resourceName_);
            ScrollToSelection();
        }
        else
        {
            SelectRightPanel(entry.resourceName_);
        }
    }

    // Process drag&drop from this element
    if (ui::BeginDragDropSource())
    {
        BeginEntryDrag(entry);
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
            RenderCompositeFile(entry);
        ui::TreePop();
    }

    if (isContextMenuOpen)
        ui::OpenPopup(contextMenuId.c_str());

    // Render context menu and popups
    RenderEntryContextMenu(entry);
    RenderRenameDialog(entry);
    RenderDeleteDialog(entry);
    RenderCreateDialog(entry);

    ui::PopID();
}

void ResourceBrowserTab::RenderCompositeFile(const FileSystemEntry& entry)
{
    tempEntryList_.clear();
    entry.ForEach([&](const FileSystemEntry& childEntry)
    {
        if (&childEntry != &entry && childEntry.isFile_)
            tempEntryList_.push_back(&childEntry);
    });

    const auto compare = [](const FileSystemEntry* lhs, const FileSystemEntry* rhs)
    {
        return FileSystemEntry::CompareFilesFirst(*lhs, *rhs);
    };
    ea::sort(tempEntryList_.begin(), tempEntryList_.end(), compare);

    for (const FileSystemEntry* childEntry : tempEntryList_)
        RenderCompositeFileEntry(*childEntry, entry);
}

void ResourceBrowserTab::RenderCompositeFileEntry(const FileSystemEntry& entry, const FileSystemEntry& ownerEntry)
{
    ui::PushID(entry.resourceName_.c_str());

    // Render the element itself
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf;
    if (entry.resourceName_ == right_.selectedPath_)
        flags |= ImGuiTreeNodeFlags_Selected;

    const ea::string localResourceName = entry.resourceName_.substr(ownerEntry.resourceName_.size() + 1);
    const ea::string name = Format("{} {}", GetEntryIcon(entry), localResourceName);

    const bool isOpen = ui::TreeNodeEx(name.c_str(), flags);
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);

    if (ui::IsItemClicked(MOUSEB_LEFT))
        SelectRightPanel(entry.resourceName_);

    // Process drag&drop from this element
    if (ui::BeginDragDropSource())
    {
        BeginEntryDrag(entry);
        ui::EndDragDropSource();
    }

    if (isOpen)
        ui::TreePop();

    if (isContextMenuOpen)
        ui::OpenPopup(contextMenuId.c_str());

    // Render context menu and popups
    RenderEntryContextMenu(entry);
    RenderRenameDialog(entry);
    RenderDeleteDialog(entry);
    RenderCreateDialog(entry);
    ui::PopID();
}

void ResourceBrowserTab::RenderRenameDialog(const FileSystemEntry& entry)
{
    if (ui::BeginPopupModal(renamePopupTitle_.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const auto [isEnabled, extraLine] = CheckFileNameInput(*entry.parent_, entry.localName_, renameBuffer_);
        ui::Text("Would you like to rename '%s'?\n%s", entry.absolutePath_.c_str(), extraLine.c_str());

        ui::SetKeyboardFocusHere();
        const bool done = ui::InputText("##Rename", &renameBuffer_,
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

        ui::BeginDisabled(!isEnabled);
        if (ui::Button(ICON_FA_CHECK " Rename") || (isEnabled && done))
        {
            if (renameBuffer_ != entry.localName_)
                RenameEntry(entry, renameBuffer_);
            ui::CloseCurrentPopup();
        }
        ui::EndDisabled();

        ui::SameLine();

        if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
            ui::CloseCurrentPopup();

        ui::EndPopup();
    }
}

void ResourceBrowserTab::RenderDeleteDialog(const FileSystemEntry& entry)
{
    if (ui::BeginPopupModal(deletePopupTitle_.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const char* formatString = "Would you like to PERMANENTLY delete '%s'?\n"
            ICON_FA_TRIANGLE_EXCLAMATION " This action cannot be undone!";
        ui::Text(formatString, entry.absolutePath_.c_str());

        if (ui::Button(ICON_FA_CHECK " Delete") || ui::IsKeyPressed(KEY_RETURN))
        {
            DeleteEntry(entry);
            ui::CloseCurrentPopup();
        }

        ui::SameLine();

        if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
            ui::CloseCurrentPopup();

        ui::EndPopup();
    }
}

void ResourceBrowserTab::RenderCreateDialog(const FileSystemEntry& parentEntry)
{
    if (ui::BeginPopupModal(createPopupTitle_.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        const ea::string basePath = parentEntry.absolutePath_.empty()
            ? GetRoot(parentEntry).activeDirectory_
            : parentEntry.absolutePath_ + "/";

        const auto [isEnabled, extraLine] = CheckFileNameInput(parentEntry, "", createNameBuffer_);
        const ea::string fileName = basePath + createNameBuffer_;
        ui::Text("Would you like to create '%s'?\n%s", fileName.c_str(), extraLine.c_str());

        ui::SetKeyboardFocusHere();
        const bool done = ui::InputText("##Create", &createNameBuffer_,
            ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue);

        createFactory_->RenderUI();

        ui::BeginDisabled(!isEnabled);
        if (ui::Button(ICON_FA_CHECK " Create") || (isEnabled && done))
        {
            createFactory_->EndCreate(fileName);
            ui::CloseCurrentPopup();
        }
        ui::EndDisabled();

        ui::SameLine();

        if (ui::Button(ICON_FA_BAN " Cancel") || ui::IsKeyPressed(KEY_ESCAPE))
            ui::CloseCurrentPopup();

        ui::EndPopup();
    }

}

SharedPtr<ResourceDragDropPayload> ResourceBrowserTab::CreateDragDropPayload(const FileSystemEntry& entry) const
{
    auto payload = MakeShared<ResourceDragDropPayload>();
    payload->localName_ = entry.localName_;
    payload->resourceName_ = entry.resourceName_;
    payload->fileName_ = entry.absolutePath_;
    payload->isMovable_ = !IsEntryFromCache(entry);
    return payload;
}

void ResourceBrowserTab::BeginEntryDrag(const FileSystemEntry& entry)
{
    ImGuiContext& g = *GImGui;

    ui::SetDragDropPayload(DragDropPayloadType.c_str(), nullptr, 0, ImGuiCond_Once);

    if (!g.DragDropPayload.Data)
    {
        const auto payload = CreateDragDropPayload(entry);
        DragDropPayload::Set(payload);
        g.DragDropPayload.Data = payload;
    }

    ui::TextUnformatted(entry.localName_.c_str());
}

void ResourceBrowserTab::DropPayloadToFolder(const FileSystemEntry& entry)
{
    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    const ResourceRoot& root = GetRoot(entry);
    auto payload = dynamic_cast<ResourceDragDropPayload*>(DragDropPayload::Get());
    if (payload && payload->isMovable_)
    {
        if (ui::AcceptDragDropPayload(DragDropPayloadType.c_str()))
        {
            const char* separator = entry.resourceName_.empty() ? "" : "/";
            const ea::string newResourceName = Format("{}{}{}", entry.resourceName_, separator, payload->localName_);
            const ea::string newFileName = Format("{}{}", root.activeDirectory_, newResourceName);
            RenameOrMoveEntry(payload->fileName_, newFileName, payload->resourceName_, newResourceName, true);
        }
    }
}

ea::string ResourceBrowserTab::GetEntryIcon(const FileSystemEntry& entry) const
{
    if (!entry.isFile_)
        return ICON_FA_FOLDER;
    else if (!entry.isDirectory_)
        return ICON_FA_FILE;
    else
        return ICON_FA_FILE_ZIPPER;
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

void ResourceBrowserTab::SelectLeftPanel(const ea::string& path, ea::optional<unsigned> rootIndex)
{
    left_.selectedPath_ = path;
    left_.selectedRoot_ = rootIndex.value_or(left_.selectedRoot_);
    right_.selectedPath_ = "";
}

void ResourceBrowserTab::SelectRightPanel(const ea::string& path)
{
    right_.selectedPath_ = path;
}

void ResourceBrowserTab::AdjustSelectionOnRename(const ea::string& oldResourceName, const ea::string& newResourceName)
{
    const ea::string oldSelectedRightPath = right_.selectedPath_;
    if (left_.selectedPath_.starts_with(oldResourceName))
    {
        const ea::string pathSuffix = left_.selectedPath_.substr(oldResourceName.size());
        if (pathSuffix.empty() || pathSuffix.front() == '/')
            SelectLeftPanel(newResourceName + pathSuffix);
    }

    if (oldSelectedRightPath.starts_with(oldResourceName))
    {
        const ea::string pathSuffix = oldSelectedRightPath.substr(oldResourceName.size());
        if (pathSuffix.empty() || pathSuffix.front() == '/')
            SelectRightPanel(newResourceName + pathSuffix);
    }

    ScrollToSelection();
}

ea::pair<bool, ea::string> ResourceBrowserTab::CheckFileNameInput(
    const FileSystemEntry& parentEntry, const ea::string& oldName, const ea::string& newName) const
{
    const bool isEmptyName = newName.empty();
    const bool isInvalidName = GetSanitizedName(newName) != newName;
    const bool isUsedName = newName != oldName && parentEntry.FindChild(newName);
    const bool isDisabled = isEmptyName || isInvalidName || isUsedName;

    ea::string extraLine;
    if (isInvalidName)
        extraLine = ICON_FA_TRIANGLE_EXCLAMATION " Name contains forbidden characters";
    else if (isUsedName)
        extraLine = ICON_FA_TRIANGLE_EXCLAMATION " File or directory with this name already exists";
    else if (isEmptyName)
        extraLine = ICON_FA_TRIANGLE_EXCLAMATION " Name must not be empty";
    else
        extraLine = ICON_FA_CIRCLE_CHECK " Name is OK";
    return {!isDisabled, extraLine};
}

void ResourceBrowserTab::RefreshContents()
{
    ScrollToSelection();
    waitingForUpdate_ = false;
}

void ResourceBrowserTab::RevealInExplorer(const ea::string& path)
{
    auto fs = GetSubsystem<FileSystem>();
#ifdef _WIN32
    fs->SystemCommand(Format("start explorer.exe /select,{}", GetNativePath(path)));
#endif
}

void ResourceBrowserTab::RenameEntry(const FileSystemEntry& entry, const ea::string& newName)
{
    const ea::string newFileName = GetPath(entry.absolutePath_) + newName;
    const ea::string newResourceName = GetPath(entry.resourceName_) + newName;
    const bool thisRootSelected = GetRootIndex(entry) == left_.selectedRoot_;
    RenameOrMoveEntry(entry.absolutePath_, newFileName, entry.resourceName_, newResourceName, thisRootSelected);
}

void ResourceBrowserTab::RenameOrMoveEntry(const ea::string& oldFileName, const ea::string& newFileName,
    const ea::string& oldResourceName, const ea::string& newResourceName, bool adjustSelection)
{
    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    const bool isFile = fs->FileExists(oldFileName);

    const bool renamed = fs->Rename(oldFileName, newFileName);

    // Show tooltip if waiting for refresh
    if (renamed)
        waitingForUpdate_ = true;

    // Keep selection on dragged element
    if (renamed && adjustSelection)
        AdjustSelectionOnRename(oldResourceName, newResourceName);

    // If file is moved and there's directory in cache with the same name, remove it
    if (renamed && isFile)
        CleanupResourceCache(oldResourceName);
}

void ResourceBrowserTab::DeleteEntry(const FileSystemEntry& entry)
{
    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    const bool isFile = fs->FileExists(entry.absolutePath_);

    const bool deleted = isFile
        ? fs->Delete(entry.absolutePath_)
        : fs->RemoveDir(entry.absolutePath_, true);

    if (deleted && isFile)
        CleanupResourceCache(entry.resourceName_);
}

void ResourceBrowserTab::CleanupResourceCache(const ea::string& resourceName)
{
    auto fs = GetSubsystem<FileSystem>();
    auto project = GetProject();

    const ea::string matchingDirectoryInCache = project->GetCachePath() + resourceName;
    if (fs->DirExists(matchingDirectoryInCache))
        fs->RemoveDir(matchingDirectoryInCache, true);

}

}
