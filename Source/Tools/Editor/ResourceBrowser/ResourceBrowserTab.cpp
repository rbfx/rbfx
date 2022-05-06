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

#include "../ResourceBrowser/ResourceBrowserTab.h"
#include "../ResourceBrowser/ResourceDragDropPayload.h"
#include "../Project/ProjectEditor.h"

#include <Urho3D/IO/FileSystem.h>

#include <EASTL/sort.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const ea::string popupDirectoryId = "ResourceBrowserTab_PopupDirectory";

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

    for (ResourceRoot& root : roots_)
        root.reflection_ = MakeShared<FileSystemReflection>(context_, root.watchedDirectories_);
}

ResourceBrowserTab::~ResourceBrowserTab()
{
}

void ResourceBrowserTab::RenderContentUI()
{
    for (ResourceRoot& root : roots_)
        root.reflection_->Update();

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
        }
        ui::EndChild();

        // Reset it mid-frame because scrolling may be triggered from RenderDirectoryContent
        scrollDirectoryTreeToSelection_ = false;

        ui::TableSetColumnIndex(1);
        if (ui::BeginChild("##DirectoryContent", ui::GetContentRegionAvail()))
            RenderDirectoryContent();
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
    if (scrollDirectoryTreeToSelection_ && rootIndex == selectedRoot_
        && selectedPath_.starts_with(entry.resourceName_))
    {
        if (selectedPath_ != entry.resourceName_)
            ui::SetNextItemOpen(true);
        ui::SetScrollHereY();
    }

    // Render the element itself
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;

    if (IsLeafDirectory(entry))
        flags |= ImGuiTreeNodeFlags_Leaf;
    if (entry.resourceName_ == selectedPath_ && rootIndex == selectedRoot_)
        flags |= ImGuiTreeNodeFlags_Selected;
    if (entry.resourceName_.empty() && root.openByDefault_)
        flags |= ImGuiTreeNodeFlags_DefaultOpen;

    const bool isOpen = ui::TreeNodeEx(displayedName.c_str(), flags);

    // Process clicking
    const bool isContextMenuOpen = ui::IsItemClicked(MOUSEB_RIGHT);
    if (ui::IsItemClicked(MOUSEB_LEFT))
    {
        selectedRoot_ = rootIndex;
        selectedPath_ = entry.resourceName_;
        selectedDirectoryContent_ = "";
    }

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
        ui::OpenPopup(popupDirectoryId.c_str());

    // Render context menu
    if (ui::BeginPopup(popupDirectoryId.c_str()))
    {
        RenderDirectoryContextMenu(entry);
        ui::EndPopup();
    }

    ui::PopID();
}

void ResourceBrowserTab::RenderDirectoryContextMenu(const FileSystemEntry& entry)
{
    const ResourceRoot& root = GetRoot(entry);

    if (ui::MenuItem("Reveal in Explorer"))
    {
        if (entry.resourceName_.empty())
            RevealInExplorer(root.activeDirectory_);
        else
            RevealInExplorer(entry.absolutePath_);
    }

    // TODO(editor): Implement
    if (!entry.resourceName_.empty())
    {
        if (ui::MenuItem("Rename"))
        {

        }

        if (ui::MenuItem("Delete"))
        {

        }
    }
}

void ResourceBrowserTab::RenderDirectoryContent()
{
    const ResourceRoot& root = roots_[selectedRoot_];
    const FileSystemEntry* entry = root.reflection_->FindEntry(selectedPath_);
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
        auto parts = selectedPath_.split('/');
        if (!parts.empty())
            parts.pop_back();

        selectedPath_ = ea::string::joined(parts, "/");
        scrollDirectoryTreeToSelection_ = true;
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

    // Render the element itself
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;
    if (entry.localName_ == selectedDirectoryContent_)
        flags |= ImGuiTreeNodeFlags_Selected;
    flags |= isCompositeFile ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_Leaf;

    const ea::string name = Format("{} {}", GetEntryIcon(entry), entry.localName_);
    const bool isOpen = ui::TreeNodeEx(name.c_str(), flags);

    if (ui::IsItemClicked(MOUSEB_LEFT))
    {
        selectedDirectoryContent_ = entry.localName_;
        if (isNormalDirectory && ui::IsMouseDoubleClicked(MOUSEB_LEFT))
        {
            selectedPath_ = entry.resourceName_;
            scrollDirectoryTreeToSelection_ = true;
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
    const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnDoubleClick
        | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Leaf;

    const ea::string localResourceName = entry.resourceName_.substr(ownerEntry.resourceName_.size() + 1);
    const ea::string name = Format("{} {}", GetEntryIcon(entry), localResourceName);

    const bool isOpen = ui::TreeNodeEx(name.c_str(), flags);

    // Process drag&drop from this element
    if (ui::BeginDragDropSource())
    {
        BeginEntryDrag(entry);
        ui::EndDragDropSource();
    }

    if (isOpen)
        ui::TreePop();

    ui::PopID();
}

SharedPtr<ResourceDragDropPayload> ResourceBrowserTab::CreateDragDropPayload(const FileSystemEntry& entry) const
{
    auto payload = MakeShared<ResourceDragDropPayload>();
    payload->localName_ = entry.localName_;
    payload->resourceName_ = entry.resourceName_;
    payload->fileName_ = entry.absolutePath_;
    payload->isMovable_ = !IsEntryFromCache(entry);
    payload->isSelectable_ = !entry.isFile_;
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
            const ea::string destination = Format("{}{}{}{}",
                root.activeDirectory_, entry.resourceName_, separator, payload->localName_);
            const ea::string source = payload->fileName_;
            const bool isFile = fs->FileExists(source);

            const bool moved = fs->Rename(source, destination);

            // Keep selection on dragged element
            if (moved && payload->isSelectable_)
                selectedPath_ = Format("{}{}{}", entry.resourceName_, separator, payload->localName_);

            // If file is moved and there's directory in cache with the same name, remove it
            if (moved && isFile)
            {
                const ea::string matchingDirectoryInCache = project->GetCachePath() + payload->resourceName_;
                if (fs->DirExists(matchingDirectoryInCache))
                    fs->RemoveDir(matchingDirectoryInCache, true);
            }
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

void ResourceBrowserTab::RevealInExplorer(const ea::string& path)
{
    auto fs = GetSubsystem<FileSystem>();
#ifdef _WIN32
    fs->SystemCommand(Format("start explorer.exe /select,{}", GetNativePath(path)));
#endif
}


}
