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

#include <IconFontCppHeaders/IconsFontAwesome5.h>

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

    ui::Columns(2);
    if (ui::BeginChild("##DirectoryTree", ui::GetContentRegionAvail()))
    {
        unsigned rootIndex = 0;
        for (ResourceRoot& root : roots_)
            RenderDirectoryTree(root.reflection_->GetRoot(), root.name_, rootIndex++);
    }
    ui::EndChild();

    // Reset it mid-frame because scrolling may be triggered from RenderDirectoryContent
    scrollDirectoryTreeToSelection_ = false;

    ui::SameLine();
    ui::NextColumn();
    if (ui::BeginChild("##DirectoryContent", ui::GetContentRegionAvail()))
        RenderDirectoryContent();
    ui::EndChild();
}

void ResourceBrowserTab::RenderDirectoryTree(const FileSystemEntry& entry,
    const ea::string& displayedName, unsigned rootIndex)
{
    ui::PushID(displayedName.c_str());

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
        ui::SetDragDropPayload(DragDropPayloadType.c_str(), nullptr, 0, ImGuiCond_Once);

        if (!g.DragDropPayload.Data)
        {
            auto payload = MakeShared<ResourceDragDropPayload>();
            payload->localName_ = entry.localName_;
            payload->resourceName_ = entry.resourceName_;
            payload->fileName_ = entry.absolutePath_;
            payload->rootIndex_ = rootIndex;

            DragDropPayload::Set(payload);
            g.DragDropPayload.Data = payload;
        }

        ui::TextUnformatted(entry.localName_.c_str());
        ui::EndDragDropSource();
    }

    // Process drag&drop to this element
    if (ui::BeginDragDropTarget())
    {
        auto payload = dynamic_cast<ResourceDragDropPayload*>(DragDropPayload::Get());
        if (payload)
        {
            if (ui::AcceptDragDropPayload(DragDropPayloadType.c_str()))
            {
                const ea::string destination = Format("{}{}/{}",
                    root.activeDirectory_, entry.resourceName_, payload->localName_);
                const ea::string source = payload->fileName_;

                auto fs = GetSubsystem<FileSystem>();
                fs->Rename(source, destination);
            }
        }
        ui::EndDragDropTarget();
    }

    // Render children
    if (isOpen)
    {
        for (const FileSystemEntry& childEntry : entry.children_)
        {
            if (!childEntry.isFile_)
                RenderDirectoryTree(childEntry, childEntry.localName_, rootIndex);
        }
        ui::TreePop();
    }

    if (isContextMenuOpen)
        ui::OpenPopup(popupDirectoryId.c_str());

    // Render context menu
    if (ui::BeginPopup(popupDirectoryId.c_str()))
    {
        RenderDirectoryContextMenu(entry, root);
        ui::EndPopup();
    }

    ui::PopID();
}

void ResourceBrowserTab::RenderDirectoryContextMenu(const FileSystemEntry& entry, const ResourceRoot& root)
{
    if (ui::MenuItem("Browse in Explorer"))
    {
        if (entry.resourceName_.empty())
            BrowseInExplorer(root.activeDirectory_);
        else
            BrowseInExplorer(entry.absolutePath_);
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

void ResourceBrowserTab::RenderDirectoryContentEntry(const FileSystemEntry& entry)
{
    const bool canBeOpenedInTree = !entry.isFile_;

    ui::PushID(entry.localName_.c_str());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;
    if (!entry.isFile_ || !entry.isDirectory_)
        flags |= ImGuiTreeNodeFlags_Leaf;
    else
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (entry.localName_ == selectedDirectoryContent_)
        flags |= ImGuiTreeNodeFlags_Selected;

    const ea::string name = Format("{} {}", GetEntryIcon(entry), entry.localName_);
    const bool isOpen = ui::TreeNodeEx(name.c_str(), flags);

    if (ui::IsItemClicked(MOUSEB_LEFT))
    {
        selectedDirectoryContent_ = entry.localName_;
        if (canBeOpenedInTree && ui::IsMouseDoubleClicked(MOUSEB_LEFT))
        {
            selectedPath_ = entry.resourceName_;
            scrollDirectoryTreeToSelection_ = true;
        }
    }

    if (isOpen)
    {
        ui::TreePop();
    }

    ui::PopID();
}

ea::string ResourceBrowserTab::GetEntryIcon(const FileSystemEntry& entry) const
{
    if (!entry.isFile_)
        return ICON_FA_FOLDER;
    else if (!entry.isDirectory_)
        return ICON_FA_FILE;
    else
        return ICON_FA_FILE_ARCHIVE;
}

void ResourceBrowserTab::BrowseInExplorer(const ea::string& path)
{
    auto fs = GetSubsystem<FileSystem>();
#ifdef _WIN32
    fs->SystemCommand(Format("start explorer.exe /select,{}", GetNativePath(path)));
#endif
}


}
