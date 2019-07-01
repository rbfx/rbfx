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

ResourceBrowserResult ResourceBrowserWidget(const ea::string& resourcePath, const ea::string& cacheDir,
    ea::string& path, ea::string& selected, ResourceBrowserFlags flags)
{
    struct State
    {
        bool isEditing = false;
        bool wasEditing = false;
        bool deletionPending = false;
        char editBuffer[250]{};
        ea::string editStartItem;
        ea::vector<ea::string> mergedDirs;
        ea::vector<ea::string> mergedFiles;
    };

    auto result = RBR_NOOP;
    auto systemUI = (SystemUI*)ui::GetIO().UserData;
    auto fs = systemUI->GetFileSystem();
    auto& state = *ui::GetUIState<State>();

    if (!selected.empty() && !ui::IsAnyItemActive() && ui::IsWindowFocused())
    {
        if (ui::IsKeyReleased(SCANCODE_F2)|| flags & RBF_RENAME_CURRENT)
        {
            state.isEditing = true;
            state.deletionPending = false;
            state.editStartItem = selected;
            strcpy(state.editBuffer, selected.c_str());
        }
        if (ui::IsKeyReleased(SCANCODE_DELETE) || flags & RBF_DELETE_CURRENT)
        {
            state.isEditing = false;
            state.deletionPending = true;
            state.editStartItem = selected;
        }
    }
    if (ui::IsKeyReleased(SCANCODE_ESCAPE) || state.editStartItem != selected)
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

    auto moveFileDropTarget = [&](const ea::string& item) {
        if (ui::BeginDragDropTarget())
        {
            const Variant& dropped = ui::AcceptDragDropVariant("path");
            if (dropped.GetType() == VAR_STRING)
            {
                using namespace ResourceBrowserRename;
                auto newName = AddTrailingSlash(item) + GetFileNameAndExtension(dropped.GetString());
                if (dropped != newName && !fs->GetCache()->GetResourceFileName(dropped.GetString()).starts_with(cacheDir))
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

    // Find resource files
    fs->ScanDir(state.mergedFiles, resourcePath + path, "", SCAN_FILES, false);
    // Remove internal files
    for (auto it = state.mergedFiles.begin(); it != state.mergedFiles.end();)
    {
        // Internal files
        const ea::string& name = *it;
        if (name.ends_with(".asset"))
            it = state.mergedFiles.erase(it);
        else
            ++it;
    }
    ea::quick_sort(state.mergedFiles.begin(), state.mergedFiles.end());

    // Find resource dirs
    fs->ScanDir(state.mergedDirs, resourcePath + path, "", SCAN_DIRS, false);
    state.mergedDirs.erase_first(".");
    state.mergedDirs.erase_first("..");
    ea::quick_sort(state.mergedDirs.begin(), state.mergedDirs.end());

    // Render dirs first
    for (const auto& item: state.mergedDirs)
    {
        if (!renameWidget(item, ICON_FA_FOLDER))
        {
            // Folder selection always ends with /, but `item` does not have / appended yet.
            auto isSelected = selected.compare(0, selected.length() - 1, item) == 0;

            if (flags & RBF_SCROLL_TO_CURRENT && isSelected)
                ui::SetScrollHereY();

            switch (ui::DoubleClickSelectable((ICON_FA_FOLDER " " + item).c_str(), isSelected))
            {
            case 1:
                selected = AddTrailingSlash(item);
                result = RBR_ITEM_SELECTED;
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

    // Render files after dirs
    for (const auto& item: state.mergedFiles)
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
            ea::string resourceCachePath = cacheDir + path + item;
            if (fs->DirExists(resourceCachePath))
            {
                StringVector cacheFiles;
                fs->ScanDir(cacheFiles, resourceCachePath, "", SCAN_FILES, true);

                if (!cacheFiles.empty())
                {
                    ui::Indent();

                    for (const ea::string& cachedFile : cacheFiles)
                    {
                        icon = GetFileIcon(resourceCachePath + cachedFile);
                        isSelected = selected == item + "/" + cachedFile;

                        if (flags & RBF_SCROLL_TO_CURRENT && isSelected)
                            ui::SetScrollHereY();

                        switch (ui::DoubleClickSelectable((icon + " " + cachedFile).c_str(), isSelected))
                        {
                        case 1:
                            selected = item + "/" + cachedFile;
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
                                ui::SetDragDropVariant("path", path + item + "/" + cachedFile);

                                // TODO: show actual preview of a resource.
                                ui::Text("%s%s", path.c_str(), cachedFile.c_str());

                                ui::EndDragDropSource();
                            }
                        }
                    }

                    ui::Unindent();
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

    return result;
}

}
