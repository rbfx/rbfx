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
#include <Urho3D/IO/Log.h>

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <ImGui/imgui_stdlib.h>
#include <ImGui/imgui_internal.h>

#include "ResourceBrowser.h"
#include "Widgets.h"
#include "IO/ContentUtilities.h"


namespace Urho3D
{

ResourceBrowserResult ResourceBrowserWidget(const ea::string& resourcePath, const ea::string& cacheDir,
    ea::string& path, ea::string& selected, ResourceBrowserFlags flags)
{
    struct State
    {
        bool rescanDirs = true;
        bool isEditing = false;
        bool wasEditing = false;
        bool deletionPending = false;
        ea::string editBuffer;
        ea::string editStartItem;
        ea::vector<ea::string> directories;
        ea::vector<ea::string> files;
        Timer updateTimer;
        std::function<void()> singleClickPending;
    };

    auto result = RBR_NOOP;
    auto systemUI = (SystemUI*)ui::GetIO().UserData;
    auto fs = systemUI->GetFileSystem();
    auto& state = *ui::GetUIState<State>();

    if (!selected.empty() && !ui::IsAnyItemActive() && ui::IsWindowFocused())
    {
        if (flags & RBF_RENAME_CURRENT)
        {
            state.isEditing = true;
            state.deletionPending = false;
            state.editStartItem = selected;
            state.editBuffer = RemoveTrailingSlash(selected);
        }
        if (flags & RBF_DELETE_CURRENT)
        {
            state.isEditing = false;
            state.deletionPending = true;
            state.editStartItem = selected;
        }
    }

    if (state.isEditing || state.deletionPending)
    {
        if (ui::IsKeyReleased(SCANCODE_ESCAPE) || state.editStartItem != selected)
        {
            state.isEditing = false;
            state.deletionPending = false;
        }
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

    auto moveFileDropTarget = [&](const ea::string& destination) {
        if (ui::BeginDragDropTarget())
        {
            const Variant& dropped = ui::AcceptDragDropVariant("path");
            if (dropped.GetType() == VAR_STRING)
            {
                const ea::string& source = dropped.GetString();
                bool isDir = source.ends_with("/");
                ea::string destinationName;
                if (destination == "..")
                    destinationName = GetParentPath(path);
                else
                    destinationName = path + destination;
                destinationName += GetFileNameAndExtension(RemoveTrailingSlash(source));
                if (isDir)
                    destinationName = AddTrailingSlash(destinationName);

                if (source != destinationName)
                {
                    fs->GetCache()->RenameResource(resourcePath + source, resourcePath + destinationName);
                    state.rescanDirs = true;
                }
            }
            ui::EndDragDropTarget();
        }
    };

    if (!path.empty())
    {
        switch (ui::DoubleClickSelectable("..", selected == ".."))
        {
        case 1:
            state.singleClickPending = [&]()
            {
                selected = "..";
            };
            break;
        case 2:
            state.singleClickPending = nullptr;
            path = GetParentPath(path);
            state.rescanDirs = true;
            break;
        default:
            break;
        }

        moveFileDropTarget("..");
    }

    auto renameWidget = [&](const ea::string& item, const ea::string& icon) {
        if (selected == item && state.isEditing)
        {
            ui::IdScope idScope("Rename");
            ui::TextUnformatted(icon.c_str());
            ui::SameLine();

            ui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, 0});
            ui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

            if (ui::InputText("", &state.editBuffer, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (selected.ends_with("/"))
                    state.editBuffer = AddTrailingSlash(state.editBuffer);

                if (selected != state.editBuffer)
                {
                    fs->GetCache()->RenameResource(resourcePath + path + selected, resourcePath + path + state.editBuffer);
                    selected = state.editBuffer;
                    state.isEditing = false;
                    state.rescanDirs = true;
                }
            }

            if (!state.wasEditing)
                ui::GetCurrentContext()->FocusRequestNextCounterTab = ui::GetCurrentContext()->ActiveId;

            ui::PopStyleVar(2);

            return true;
        }
        return false;
    };

    if (state.updateTimer.GetMSec(false) >= 1000 || state.rescanDirs)
    {
        state.rescanDirs = false;
        state.updateTimer.Reset();

        // Find resource files
        fs->ScanDir(state.files, resourcePath + path, "", SCAN_FILES, false);
        // Remove internal files
        for (auto it = state.files.begin(); it != state.files.end();)
        {
            // Internal files
            const ea::string& name = *it;
            if (name.ends_with(".asset"))
                it = state.files.erase(it);
            else
                ++it;
        }
        ea::quick_sort(state.files.begin(), state.files.end());

        // Find resource dirs
        fs->ScanDir(state.directories, resourcePath + path, "", SCAN_DIRS, false);
        state.directories.erase_first(".");
        state.directories.erase_first("..");
        ea::quick_sort(state.directories.begin(), state.directories.end());

        for (ea::string& dir : state.directories)
            dir = AddTrailingSlash(dir);
    }

    // Render dirs first
    for (const ea::string& item: state.directories)
    {
        if (!renameWidget(item, ICON_FA_FOLDER))
        {
            bool isSelected = selected == item;

            if (flags & RBF_SCROLL_TO_CURRENT && isSelected)
                ui::SetScrollHereY();

            switch (ui::DoubleClickSelectable((ICON_FA_FOLDER " " + RemoveTrailingSlash(item)).c_str(), isSelected))
            {
            case 1:
                state.singleClickPending = [&]()
                {
                    selected = item;
                    result = RBR_ITEM_SELECTED;
                };
                break;
            case 2:
                state.singleClickPending = nullptr;
                path += AddTrailingSlash(item);
                selected.clear();
                state.rescanDirs = true;
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

    // Render files after dirs
    for (const auto& item: state.files)
    {
        auto icon = GetFileIcon(item);
        if (!renameWidget(item, icon))
        {
            auto isSelected = selected == item;

            if (flags & RBF_SCROLL_TO_CURRENT && isSelected)
                ui::SetScrollHereY();

            switch (ui::DoubleClickSelectable((icon + " " + item).c_str(), isSelected))
            {
            case 1:
                selected = item;
                result = RBR_ITEM_SELECTED;
                using namespace ResourceBrowserSelect;
                fs->SendEvent(E_RESOURCEBROWSERSELECT, P_NAME, path + selected);
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

            // Render cache items
            ea::string cacheItem = GetFileName(item);
            ea::string resourceCachePath = cacheDir + path + cacheItem;
            if (fs->DirExists(resourceCachePath))
            {
                StringVector cacheFiles;
                fs->ScanDir(cacheFiles, resourceCachePath, "", SCAN_FILES, true);

                if (!cacheFiles.empty())
                {
                    ui::PushID(resourceCachePath.c_str());
                    ui::Indent();

                    for (const ea::string& cachedFile : cacheFiles)
                    {
                        icon = GetFileIcon(resourceCachePath + cachedFile);
                        isSelected = selected == cacheItem + "/" + cachedFile;

                        if (flags & RBF_SCROLL_TO_CURRENT && isSelected)
                            ui::SetScrollHereY();

                        switch (ui::DoubleClickSelectable((icon + " " + cachedFile).c_str(), isSelected))
                        {
                        case 1:
                            state.singleClickPending = [&]()
                            {
                                selected = cacheItem + "/" + cachedFile;
                                result = RBR_ITEM_SELECTED;
                                using namespace ResourceBrowserSelect;
                                fs->SendEvent(E_RESOURCEBROWSERSELECT, P_NAME, path + selected);
                            };
                            break;
                        case 2:
                            state.singleClickPending = nullptr;
                            selected = cacheItem + "/" + cachedFile;
                            result = RBR_ITEM_OPEN;
                            break;
                        default:
                            break;
                        }

                        if (ui::IsItemActive())
                        {
                            if (ui::BeginDragDropSource())
                            {
                                ui::SetDragDropVariant("path", path + cacheItem + "/" + cachedFile);

                                // TODO: show actual preview of a resource.
                                ui::Text("%s%s/%s", path.c_str(), cacheItem.c_str(), cachedFile.c_str());

                                ui::EndDragDropSource();
                            }
                        }
                    }

                    ui::Unindent();
                    ui::PopID();
                }
            }
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

    if (state.singleClickPending)
    {
        auto& g = *ui::GetCurrentContext();
        if ((float)(g.Time - g.IO.MouseClickedTime[0]) > g.IO.MouseDoubleClickTime)
        {
            state.singleClickPending();
            state.singleClickPending = nullptr;
        }
    }

    return result;
}

}
