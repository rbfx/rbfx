//
// Copyright (c) 2018 Rokas Kupstys
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

ResourceBrowserResult ResourceBrowserWindow(String& path, String& selected)
{
    auto result = RBR_NOOP;
    if (ui::BeginDock("Resources"))
    {
        result = ResourceBrowserWidget(path, selected);
    }
    ui::EndDock();
    return result;
}

ResourceBrowserResult ResourceBrowserWidget(String& path, String& selected)
{
    auto result = RBR_NOOP;
    auto systemUI = (SystemUI*)ui::GetIO().UserData;
    auto fs = systemUI->GetFileSystem();

    Vector<String> mergedDirs;
    Vector<String> mergedFiles;

    String cacheDir;
    for (const auto& dir: systemUI->GetCache()->GetResourceDirs())
    {
        if (dir.EndsWith("/EditorData/"))
            continue;

        if (dir.EndsWith("/Cache/"))
        {
            cacheDir = dir;
            continue;
        }

        Vector<String> items;
        fs->ScanDir(items, dir + path, "", SCAN_FILES, false);
        for (const auto& item: items)
        {
            if (!mergedFiles.Contains(item))
                mergedFiles.Push(item);
        }

        items.Clear();
        fs->ScanDir(items, dir + path, "", SCAN_DIRS, false);
        items.Remove(".");
        items.Remove("..");
        for (const auto& item: items)
        {
            if (!mergedDirs.Contains(item))
                mergedDirs.Push(item);
        }
    }

    auto moveFileDropTarget = [&](const String& item) {
        if (ui::BeginDragDropTarget())
        {
            auto dropped = ui::AcceptDragDropVariant("path");
            if (dropped.GetType() == VAR_STRING)
            {
                auto cache = systemUI->GetCache();
                auto dragSource = cache->GetResourceFileName(dropped.GetString());
                auto dropTarget = AddTrailingSlash(item);

                // Find existing drop target
                const auto& resourceDirs = cache->GetResourceDirs();
                String mainResourceDir;
                for (const auto& resourceDir : resourceDirs)
                {
                    if (resourceDir.EndsWith("/EditorData/") || resourceDir.EndsWith("/Cache/"))
                        continue;

                    if (mainResourceDir.Empty())
                    {
                        mainResourceDir = resourceDir;
                    }

                    if (fs->DirExists(resourceDir + dropTarget))
                    {
                        dropTarget = resourceDir + dropTarget + GetFileNameAndExtension(dragSource);
                        break;
                    }
                }

                cache->SetAutoReloadResourcesSuspended(true);
                if (fs->Rename(dragSource, dropTarget))
                {
                    using namespace ResourceRenamed;
                    fs->SendEvent(E_RESOURCERENAMED, P_FROM, dragSource, P_TO, dropTarget);
                }
                cache->SetAutoReloadResourcesSuspended(false);
            }
            ui::EndDragDropTarget();
        }
    };

    if (!path.Empty())
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

    Sort(mergedDirs.Begin(), mergedDirs.End());
    for (const auto& item: mergedDirs)
    {
        switch (ui::DoubleClickSelectable((ICON_FA_FOLDER " " + item).CString(), selected == item))
        {
        case 1:
            selected = item;
            break;
        case 2:
            path += AddTrailingSlash(item);
            selected.Clear();
            break;
        default:
            break;
        }

        moveFileDropTarget(path + item);
    }

    auto renderAssetEntry = [&](const String& item) {
        auto title = GetFileIcon(item) + " " + GetFileNameAndExtension(item);
        switch (ui::DoubleClickSelectable(title.CString(), selected == item))
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
                ui::Text("%s%s", path.CString(), item.CString());

                ui::EndDragDropSource();
            }
        }
    };

    Sort(mergedFiles.Begin(), mergedFiles.End());
    for (const auto& item: mergedFiles)
    {
        if (fs->DirExists(cacheDir + path + item))
        {
            // File is converted asset.
            std::function<void(const String&)> renderCacheAssetTree = [&](const String& subPath)
            {
                String targetPath = cacheDir + path + subPath;

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

    if (ui::IsWindowHovered())
    {
        if (ui::IsMouseClicked(MOUSEB_RIGHT))
            result = RBR_ITEM_CONTEXT_MENU;

        if ((ui::IsMouseClicked(MOUSEB_LEFT) || ui::IsMouseClicked(MOUSEB_RIGHT)) && !ui::IsAnyItemHovered())
            // Clicking empty area unselects item.
            selected.Clear();
    }

    return result;
}

}
