//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "ResourceBrowser.h"
#include "ImGuiDock.h"
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/FileSystem.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>
#include "Widgets.h"
#include "IO/ContentUtilities.h"


namespace Urho3D
{

bool ResourceBrowserWindow(String& selected)
{
    struct State
    {
        String path;
        String selected;
    };

    bool result = false;
    auto systemUI = (SystemUI*)ui::GetIO().UserData;
    auto fs = systemUI->GetFileSystem();
    if (ui::BeginDock("Resources"))
    {
        State* state = ui::GetUIState<State>();

        Vector<String> mergedDirs;
        Vector<String> mergedFiles;

        String cacheDir;
        for (const auto& dir: systemUI->GetCache()->GetResourceDirs())
        {
            if (dir.EndsWith("/Cache/"))
            {
                cacheDir = dir;
                continue;
            }

            Vector<String> items;
            fs->ScanDir(items, dir + state->path, "", SCAN_FILES, false);
            for (const auto& item: items)
            {
                if (!mergedFiles.Contains(item))
                    mergedFiles.Push(item);
            }

            items.Clear();
            fs->ScanDir(items, dir + state->path, "", SCAN_DIRS, false);
            items.Remove(".");
            items.Remove("..");
            for (const auto& item: items)
            {
                if (!mergedDirs.Contains(item))
                    mergedDirs.Push(item);
            }
        }

        switch (ui::DoubleClickSelectable("..", state->selected == ".."))
        {
        case 1:
            state->selected = "..";
            break;
        case 2:
            state->path = GetParentPath(state->path);
            break;
        default:
            break;
        }

        Sort(mergedDirs.Begin(), mergedDirs.End());
        for (const auto& item: mergedDirs)
        {
            switch (ui::DoubleClickSelectable((ICON_FA_FOLDER " " + item).CString(), state->selected == item))
            {
            case 1:
                state->selected = item;
                break;
            case 2:
                state->path += AddTrailingSlash(item);
                state->selected.Clear();
                break;
            default:
                break;
            }
        }

        auto renderAssetEntry = [&](const String& item) {
            auto title = GetFileIcon(item) + " " + GetFileNameAndExtension(item);
            switch (ui::DoubleClickSelectable(title.CString(), state->selected == item))
            {
            case 1:
                state->selected = item;
                break;
            case 2:
                selected = state->path + item;
                result = true;
                break;
            default:
                break;
            }

            if (ui::IsItemActive())
            {
                if (ui::BeginDragDropSource())
                {
                    String path = state->path + item;
                    ui::SetDragDropVariant("path", path);

                    // TODO: show actual preview of a resource.
                    ui::Text("%s", path.CString());

                    ui::EndDragDropSource();
                }
            }
        };

        Sort(mergedFiles.Begin(), mergedFiles.End());
        for (const auto& item: mergedFiles)
        {
            if (fs->DirExists(cacheDir + state->path + item))
            {
                // File is converted asset.
                std::function<void(const String&)> renderCacheAssetTree = [&](const String& subPath)
                {
                    String targetPath = cacheDir + state->path + subPath;

                    if (fs->DirExists(targetPath))
                    {
                        ui::TextUnformatted(ICON_FA_FOLDER_OPEN);
                        ui::SameLine();
                        if (ui::TreeNode(GetFileNameAndExtension(subPath).CString()))
                        {
                            Vector<String> files;
                            Vector<String> dirs;
                            fs->ScanDir(files, targetPath, "", SCAN_FILES, false);
                            fs->ScanDir(dirs, targetPath, "", SCAN_DIRS, false);
                            dirs.Remove(".");
                            dirs.Remove("..");
                            Sort(files.Begin(), files.End());
                            Sort(dirs.Begin(), dirs.End());

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
    }
    ui::EndDock();
    return result;
}

}
