// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
