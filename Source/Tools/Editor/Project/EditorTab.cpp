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

#include "../Project/EditorTab.h"
#include "../Project/ProjectEditor.h"

#include <Urho3D/Input/Input.h>

#if 0
#include <Urho3D/Core/ProcessUtils.h>
#include "EditorEvents.h"
#include "Editor.h"
#include "Tab.h"
#include "PreviewTab.h"
#endif

namespace Urho3D
{

EditorTab::EditorTab(Context* context, const ea::string& title, const ea::string& guid,
    EditorTabFlags flags, EditorTabPlacement placement)
    : Object(context)
    , title_(title)
    , guid_(guid)
    , uniqueId_(Format("{}###{}", title_, guid_))
    , flags_(flags)
    , placement_(placement)
{
}

EditorTab::~EditorTab()
{
}

void EditorTab::WriteIniSettings(ImGuiTextBuffer* output)
{
    output->appendf("\n[Project][%s]\n", GetUniqueId().c_str());
    output->appendf("IsOpen=%d\n", open_ ? 1 : 0);
}

void EditorTab::ReadIniSettings(const char* line)
{
    int isOpen{};
    if (sscanf(line, "IsOpen=%d\n", &isOpen) == 1)
        open_ = isOpen != 0;
}

void EditorTab::RenderUI()
{
    wasOpen_ = open_;

    if (focusPending_ || openPending_)
        open_ = true;

    if (open_)
        RenderWindowUI();

    focusPending_ = false;
    openPending_ = false;
}

void EditorTab::RenderWindowUI()
{
    // TODO(editor): Hide this dependency
    Input* input = GetSubsystem<Input>();

    const bool noContentPadding = flags_.Test(EditorTabFlag::NoContentPadding);

    if (noContentPadding)
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    if (IsModified())
        windowFlags_ |= ImGuiWindowFlags_UnsavedDocument;
    else
        windowFlags_ &= ~ImGuiWindowFlags_UnsavedDocument;

    if (focusPending_)
        ui::SetNextWindowFocus();

    ui::Begin(uniqueId_.c_str(), &open_, windowFlags_);

    if (noContentPadding)
        ui::PopStyleVar();

    if (OnRenderContextMenu.HasSubscriptions())
    {
        if (ui::BeginPopupContextItem("Tab context menu"))
        {
            OnRenderContextMenu(this);
            ui::EndPopup();
        }
    }

    if (ui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
        UpdateFocused();
    else
    {
        if (input->IsMouseVisible() && ui::IsAnyMouseDown())
        {
            if (ui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
                ui::SetWindowFocus();
        }
    }

    RenderContentUI();

    if (noContentPadding)
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    ui::End();

    if (noContentPadding)
        ui::PopStyleVar();
}

ProjectEditor* EditorTab::GetProject() const
{
    return GetSubsystem<ProjectEditor>();
}

}
