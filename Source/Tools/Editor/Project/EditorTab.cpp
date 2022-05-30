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

#include "../Core/EditorPluginManager.h"
#include "../Core/IniHelpers.h"
#include "../Project/EditorTab.h"
#include "../Project/ProjectEditor.h"

#include <Urho3D/Input/Input.h>

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_Undo, "Global.Undo", QUAL_CTRL, KEY_Z);
URHO3D_EDITOR_HOTKEY(Hotkey_Redo, "Global.Redo", QUAL_CTRL, KEY_Y);

}

EditorTab::EditorTab(Context* context, const ea::string& title, const ea::string& guid,
    EditorTabFlags flags, EditorTabPlacement placement)
    : EditorConfigurable(context)
    , title_(title)
    , guid_(guid)
    , uniqueId_(Format("{}###{}", title_, guid_))
    , flags_(flags)
    , placement_(placement)
{
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    hotkeyManager->BindHotkey(this, Hotkey_Undo, &EditorTab::Undo);
    hotkeyManager->BindHotkey(this, Hotkey_Redo, &EditorTab::Redo);
}

EditorTab::~EditorTab()
{
}

void EditorTab::ApplyPlugins()
{
    auto editorPluginManager = GetSubsystem<EditorPluginManager>();
    editorPluginManager->Apply(this);
}

void EditorTab::WriteIniSettings(ImGuiTextBuffer& output)
{
    WriteIntToIni(output, "IsOpen", open_ ? 1 : 0);
}

void EditorTab::ReadIniSettings(const char* line)
{
    if (const auto isOpen = ReadIntFromIni(line, "IsOpen"))
        open_ = *isOpen != 0;
}

void EditorTab::Render()
{
    wasOpen_ = open_;

    if (focusPending_ || openPending_)
        open_ = true;

    if (open_)
        RenderWindow();

    focusPending_ = false;
    openPending_ = false;
}

void EditorTab::Focus(bool force)
{
    auto project = GetProject();
    if (force || project->GetRootFocusedTab() != this)
        focusPending_ = true;
}

void EditorTab::RenderWindow()
{
    // TODO(editor): Hide this dependency
    auto input = GetSubsystem<Input>();
    auto project = GetProject();

    const bool noContentPadding = flags_.Test(EditorTabFlag::NoContentPadding);

    if (noContentPadding)
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    if (IsMarkedUnsaved())
        windowFlags_ |= ImGuiWindowFlags_UnsavedDocument;
    else
        windowFlags_ &= ~ImGuiWindowFlags_UnsavedDocument;

    if (focusPending_)
        ui::SetNextWindowFocus();

    ui::Begin(uniqueId_.c_str(), &open_, windowFlags_);

    if (noContentPadding)
        ui::PopStyleVar();

    if (ui::BeginPopupContextItem("EditorTab_ContextMenu"))
    {
        RenderContextMenu();
        ui::EndPopup();
    }

    if (ui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
    {
        project->SetFocusedTab(this);
        ApplyHotkeys(project->GetHotkeyManager());
        UpdateFocused();
    }
    else
    {
        if (input->IsMouseVisible() && ui::IsAnyMouseDown())
        {
            if (ui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
                ui::SetWindowFocus();
        }
    }

    RenderContent();

    if (noContentPadding)
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    ui::End();

    if (noContentPadding)
        ui::PopStyleVar();
}

void EditorTab::RenderContextMenu()
{
    contextMenuSeparator_.Reset();
    RenderContextMenuItems();
    contextMenuSeparator_.Add();

    contextMenuSeparator_.Reset();
    const int oldY = ui::GetCursorPosY();
    OnRenderContextMenu(this);
    if (oldY != ui::GetCursorPosY())
        contextMenuSeparator_.Add();

    if (ui::MenuItem("Close Tab"))
        Close();
}

void EditorTab::Undo()
{
    if (IsUndoSupported())
    {
        auto project = GetProject();
        UndoManager* undoManager = project->GetUndoManager();
        undoManager->Undo();
    }
}

void EditorTab::Redo()
{
    if (IsUndoSupported())
    {
        auto project = GetProject();
        UndoManager* undoManager = project->GetUndoManager();
        undoManager->Redo();
    }
}

void EditorTab::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

IntVector2 EditorTab::GetContentSize() const
{
    const ImGuiContext& g = *GImGui;
    const ImGuiWindow* window = g.CurrentWindow;
    const ImRect rect = ImRound(window->ContentRegionRect);
    return {RoundToInt(rect.GetWidth()), RoundToInt(rect.GetHeight())};
}

void EditorTab::RenderEditMenuItems()
{
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    if (ui::MenuItem("Undo", hotkeyManager->GetHotkeyLabel(Hotkey_Undo).c_str()))
        Undo();
    if (ui::MenuItem("Redo", hotkeyManager->GetHotkeyLabel(Hotkey_Redo).c_str()))
        Redo();
    ui::Separator();
}

ProjectEditor* EditorTab::GetProject() const
{
    return GetSubsystem<ProjectEditor>();
}

}
