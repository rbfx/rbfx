// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Foundation/InspectorTab.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_InspectorTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<InspectorTab>(context));
}

InspectorTab::InspectorTab(Context* context)
    : EditorTab(context, "Inspector", "bd959865-8929-4f92-a20f-97ff867d6ba6",
        EditorTabFlag::OpenByDefault, EditorTabPlacement::DockRight)
{
}

void InspectorTab::RegisterAddon(const SharedPtr<Object>& addon)
{
    addons_.push_back(addon);
}

void InspectorTab::ConnectToSource(Object* source, InspectorSource* sourceInterface)
{
    source_ = source;
    sourceInterface_ = sourceInterface;
}

void InspectorTab::RenderMenu()
{
    if (source_)
        sourceInterface_->RenderMenu();
}

void InspectorTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    EditorTab::ApplyHotkeys(hotkeyManager);
    if (source_)
        sourceInterface_->ApplyHotkeys(hotkeyManager);
}

void InspectorTab::RenderContent()
{
    if (source_)
        sourceInterface_->RenderContent();
}

void InspectorTab::RenderContextMenuItems()
{
    if (source_)
        sourceInterface_->RenderContextMenuItems();
}

}
