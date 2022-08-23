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

#include "../Foundation/HierarchyBrowserTab.h"

namespace Urho3D
{

void Foundation_HierarchyBrowserTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<HierarchyBrowserTab>(context));
}

HierarchyBrowserTab::HierarchyBrowserTab(Context* context)
    : EditorTab(context, "Hierarchy", "38ee90af-0a65-4d7d-93e2-d446ae54dffd",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockLeft)
{
}

void HierarchyBrowserTab::ConnectToSource(Object* source, HierarchyBrowserSource* sourceInterface)
{
    source_ = source;
    sourceInterface_ = sourceInterface;
}

void HierarchyBrowserTab::RenderMenu()
{
    if (source_)
        sourceInterface_->RenderMenu();
}

void HierarchyBrowserTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    if (source_)
        sourceInterface_->ApplyHotkeys(hotkeyManager);
}

void HierarchyBrowserTab::RenderContent()
{
    if (source_)
        sourceInterface_->RenderContent();
}

void HierarchyBrowserTab::RenderContextMenuItems()
{
    if (source_)
        sourceInterface_->RenderContextMenuItems();
}

}
