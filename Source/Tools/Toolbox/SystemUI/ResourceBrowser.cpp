//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <EASTL/sort.h>

#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Input/Input.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Urho3D/IO/Log.h>
#include "ResourceBrowser.h"
#include "Widgets.h"
#include "IO/ContentUtilities.h"


namespace Urho3D
{

ResourceBrowserResult ResourceBrowserWidget(ea::string& path, ea::string& selected, ResourceBrowserFlags flags)
{
    struct State
    {
        bool isEditing = false;
        bool wasEditing = false;
        bool deletionPending = false;
        char editBuffer[250]{};
        ea::string editStartItem;
    };

    auto result = RBR_NOOP;
    auto systemUI = (SystemUI*)ui::GetIO().UserData;
    auto fs = systemUI->GetFileSystem();
    auto& state = *ui::GetUIState<State>();

    if (!selected.empty() && !ui::IsAnyItemActive() && ui::IsWindowFocused())
    {
        if (fs->GetInput()->GetKeyPress(KEY_F2) || flags & RBF_RENAME_CURRENT)
        {
            state.isEditing = true;
            state.deletionPending = false;
            state.editStartItem = selected;
            strcpy(state.editBuffer, selected.c_str());
        }
        if (fs->GetInput()->GetKeyPress(KEY_DELETE) || flags & RBF_DELETE_CURRENT)
        {
            state.isEditing = false;
            state.deletionPending = true;
            state.editStartItem = selected;
        }
    }
    if (fs->GetInput()->GetKeyPress(KEY_ESCAPE) || state.editStartItem != selected)
    {
        state.isEditing = false;
        state.deletionPending = false;
    }

    if (state.deletionPending)
    {
        if (ui::Begin("Delete?", &state.deletionPending))
        {
            ui::Text("Would you like to delete '%s%s'?", path.c_str(), selected.c_str());
            ui::TextUnformatted(ICON_FA_EXCLAMATION_TRIANGLE " This action can not be undone!");
            ui::NewLine();

            if (ui::Button("Delete Permanently"))
            {
                using namespace ResourceBrowserDelete;
                fs->SendEvent(E_RESOURCEBROWSERDELETE, P_NAME, path + selected);
                state.deletionPending = false;
            }
        }
        ui::End();
    }

    ea::vector<ea::string> mergedDirs;
    ea::vector<ea::string> mergedFiles;

    ea::string cacheDir;
    for (const auto& dir: systemUI->GetCache()->GetResourceDirs())
    {
        if (dir.ends_with("/EditorData/"))
            continue;

        if (dir.ends_with("/Cache/"))
        {
            cacheDir = dir;
            continue;
        }

        ea::vector<ea::string> items;
        fs->ScanDir(items, dir + path, "", SCAN_FILES, false);
        for (const auto& item: items)
        {
            if (!mergedFiles.contains(item))
                mergedFiles.push_back(item);
        }

        items.clear();
        fs->ScanDir(items, dir + path, "", SCAN_DIRS, false);
        items.erase_first(".");
        items.erase_first("..");
        for (const auto& item: items)
        {
            if (!mergedDirs.contains(item))
                mergedDirs.push_back(item);
        }
    }

    auto moveFileDropTarget = [&](const ea::string& item) {
        if (ui::BeginDragDropTarget())
        {
            auto dropped = ui::AcceptDragDropVariant("path");
            if (dropped.GetType() == VAR_STRING)
            {
                using namespace ResourceBrowserRename;
                auto newName = AddTrailingSlash(item) + GetFileNameAndExtension(dropped.GetString());
                if (dropped != newName)
                    fs->SendEvent(E_RESOURCEBROWSERRENAME, P_FROM, dropped, P_TO, newName);
            }
            ui::EndDragDropTarget();
        }
    };

    if (!path.empty())
    {
        switch (ui::DoubleClickSelectable("..", selected == ".."))
        {
        case 1:
            selected = "..";
            break;
        case 2:
            path = GetParentPath(path);
            break;
        default:
            break;
        }

        moveFileDropTarget(GetParentPath(path));
    }

    auto renameWidget = [&](const ea::string& item, const ea::string& icon) {
        if (selected == item && state.isEditing)
        {
            ui::IdScope idScope("Rename");
            ui::TextUnformatted(icon.c_str());
            ui::SameLine();

            ui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
            ui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

            if (ui::InputText("", state.editBuffer, sizeof(state.editBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                using namespace ResourceBrowserRename;
                auto oldName = path + selected;
                auto newName = path + state.editBuffer;
                if (oldName != newName)
                    fs->SendEvent(E_RESOURCEBROWSERRENAME, P_FROM, oldName, P_TO, newName);
                state.isEditing = false;
            }

            if (!state.wasEditing)
                ui::GetCurrentContext()->FocusRequestNextCounterTab = ui::GetCurrentContext()->ActiveId;

            ui::PopStyleVar(2);

            return true;
        }
        return false;
    };

    ea::quick_sort(mergedDirs.begin(), mergedDirs.end());
    for (const auto& item: mergedDirs)
    {
        if (!renameWidget(item, ICON_FA_FOLDER))
        {
            auto isSelected = selected == item;

            if (flags & RBF_SCROLL_TO_CURRENT && isSelected)
                ui::SetScrollHereY();

            switch (ui::DoubleClickSelectable((ICON_FA_FOLDER " " + item).c_str(), isSelected))
            {
            case 1:
                selected = item;
                break;
            case 2:
                path += AddTrailingSlash(item);
                selected.clear();
                break;
            default:
                break;
            }

            if (ui::IsItemActive())
            {
                if (ui::BeginDragDropSource())
                {
                    ui::SetDragDropVariant("path", path + item);

                    // TODO: show actual preview of a resource.
                    ui::Text("%s%s", path.c_str(), item.c_str());

                    ui::EndDragDropSource();
                }
            }

            moveFileDropTarget(path + item);
        }
    }

    auto renderAssetEntry = [&](const ea::string& item) {
        auto icon = GetFileIcon(item);
        if (!renameWidget(item, icon))
        {
            if (flags & RBF_SCROLL_TO_CURRENT && selected == item)
                ui::SetScrollHereY();
            auto title = icon + " " + GetFileNameAndExtension(item);
            switch (ui::DoubleClickSelectable(title.c_str(), selected == item))
            {
            case 1:
                selected = item;
                result = RBR_ITEM_SELECTED;
                break;
            case 2:
                result = RBR_ITEM_OPEN;
                break;
            default:
                break;
            }

            if (ui::IsItemActive())
            {
                if (ui::BeginDragDropSource())
                {
                    ui::SetDragDropVariant("path", path + item);

                    // TODO: show actual preview of a resource.
                    ui::Text("%s%s", path.c_str(), item.c_str());

                    ui::EndDragDropSource();
                }
            }
        }
    };

    ea::quick_sort(mergedFiles.begin(), mergedFiles.end());
    for (const auto& item: mergedFiles)
    {
        if (fs->DirExists(cacheDir + path + item))
        {
            // File is converted asset.
            std::function<void(const ea::string&)> renderCacheAssetTree = [&](const ea::string& subPath)
            {
                ea::string targetPath = cacheDir + path + subPath;

                if (fs->DirExists(targetPath))
                {
                    ui::TextUnformatted(ICON_FA_FOLDER_OPEN);
                    ui::SameLine();
                    if (ui::TreeNode(GetFileNameAndExtension(subPath).c_str()))
                    {
                        ea::vector<ea::string> files;
                        ea::vector<ea::string> dirs;
                        fs->ScanDir(files, targetPath, "", SCAN_FILES, false);
                        fs->ScanDir(dirs, targetPath, "", SCAN_DIRS, false);
                        dirs.erase_first(".");
                        dirs.erase_first("..");
                        ea::quick_sort(files.begin(), files.end());
                        ea::quick_sort(dirs.begin(), dirs.end());

                        for (const auto& dir : dirs)
                            renderCacheAssetTree(subPath + "/" + dir);

                        for (const auto& file : files)
                            renderAssetEntry(subPath + "/" + file);

                        ui::TreePop();
                    }
                }
                else
                    renderAssetEntry(subPath);
            };
            renderCacheAssetTree(item);
        }
        else
        {
            // File exists only in data directories.
            renderAssetEntry(item);
        }
    }

    if (ui::IsWindowHovered())
    {
        if (ui::IsMouseClicked(MOUSEB_RIGHT))
            result = RBR_ITEM_CONTEXT_MENU;

        if ((ui::IsMouseClicked(MOUSEB_LEFT) || ui::IsMouseClicked(MOUSEB_RIGHT)) && !ui::IsAnyItemHovered())
            // Clicking empty area unselects item.
            selected.clear();
    }

    state.wasEditing = state.isEditing;

    return result;
}

}
