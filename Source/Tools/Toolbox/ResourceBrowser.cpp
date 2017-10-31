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
#include "ContentUtilities.h"


namespace Urho3D
{

bool ResourceBrowserWindow(Context* context, String& selected, bool* open)
{
    static struct
    {
        String path;
        String selected;
    } state;

    bool result = false;
    auto fs = context->GetFileSystem();
    if (ui::BeginDock("Resources"))
    {
        Vector<String> mergedDirs;
        Vector<String> mergedFiles;

        for (const auto& dir: context->GetCache()->GetResourceDirs())
        {
            Vector<String> items;
            fs->ScanDir(items, dir + state.path, "", SCAN_FILES, false);
            for (const auto& item: items)
            {
                if (item == "." || item == ".." || mergedFiles.Contains(item))
                    continue;
                mergedFiles.Push(item);
            }

            items.Clear();
            fs->ScanDir(items, dir + state.path, "", SCAN_DIRS, false);
            for (const auto& item: items)
            {
                if (item == "." || item == ".." || mergedDirs.Contains(item))
                    continue;
                mergedDirs.Push(item);
            }
        }

        switch (ui::DoubleClickSelectable("..", state.selected == ".."))
        {
        case 1:
            state.selected = "..";
            break;
        case 2:
            state.path = GetParentPath(state.path);
            break;
        default:
            break;
        }

        Sort(mergedDirs.Begin(), mergedDirs.End());
        for (const auto& item: mergedDirs)
        {
            switch (ui::DoubleClickSelectable((ICON_FA_FOLDER " " + item).CString(), state.selected == item))
            {
            case 1:
                state.selected = item;
                break;
            case 2:
                state.path += AddTrailingSlash(item);
                state.selected.Clear();
                break;
            default:
                break;
            }
        }

        Sort(mergedFiles.Begin(), mergedFiles.End());
        for (const auto& item: mergedFiles)
        {
            auto title = GetFileIcon(item) + " " + item;
            switch (ui::DoubleClickSelectable(title.CString(), state.selected == item))
            {
            case 1:
                state.selected = item;
                break;
            case 2:
                selected = state.path + item;
                result = true;
                break;
            default:
                break;
            }

            if (ui::IsItemHovered() && ui::IsMouseDragging() && !context->GetSystemUI()->HasDragData())
                context->GetSystemUI()->SetDragData(state.path + item);
        }
    }
    ui::EndDock();
    return result;
}

}
