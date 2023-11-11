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
#include "../Project/Project.h"

#include <Urho3D/Input/Input.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_Undo = EditorHotkey{"Global.Undo"}.Ctrl().Press(KEY_Z);
const auto Hotkey_Redo = EditorHotkey{"Global.Redo"}.Ctrl().Press(KEY_Y);

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
    BindHotkey(Hotkey_Undo, &EditorTab::Undo);
    BindHotkey(Hotkey_Redo, &EditorTab::Redo);
}

EditorTab::~EditorTab()
{
}

void EditorTab::ApplyPlugins()
{
    auto editorPluginManager = GetSubsystem<EditorPluginManager>();
    editorPluginManager->Apply(this);
}

ea::optional<EditorActionFrame> EditorTab::PushAction(SharedPtr<EditorAction> action)
{
    UndoManager* undoManager = GetUndoManager();
    return undoManager->PushAction(action);
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
    const bool noContentPadding = flags_.Test(EditorTabFlag::NoContentPadding);

    if (noContentPadding)
        ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

    if (IsMarkedUnsaved())
        windowFlags_ |= ImGuiWindowFlags_UnsavedDocument;
    else
        windowFlags_ &= ~ImGuiWindowFlags_UnsavedDocument;

    if (focusPending_)
        ui::SetNextWindowFocus();

    if (ui::Begin(uniqueId_.c_str(), &open_, windowFlags_))
    {
        if (noContentPadding)
            ui::PopStyleVar();

        if (ui::BeginPopupContextItem("EditorTab_ContextMenu"))
        {
            RenderContextMenu();
            ui::EndPopup();
        }

        if (ui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
        {
            auto project = GetProject();
            project->SetFocusedTab(this);
        }
        else
        {
            // TODO(editor): Why do we need this?
            auto input = GetSubsystem<Input>();
            if (input->IsMouseVisible() && ui::IsAnyMouseDown())
            {
                if (ui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
                    ui::SetWindowFocus();
            }
        }

        RenderContent();

        if (noContentPadding)
            ui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    }
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
    auto undoManager = GetProject()->GetUndoManager();
    if (ui::MenuItem("Undo", GetHotkeyLabel(Hotkey_Undo).c_str(), false, undoManager->CanUndo()))
        Undo();
    if (ui::MenuItem("Redo", GetHotkeyLabel(Hotkey_Redo).c_str(), false, undoManager->CanRedo()))
        Redo();
    ui::Separator();
}

Project* EditorTab::GetProject() const
{
    return GetSubsystem<Project>();
}

HotkeyManager* EditorTab::GetHotkeyManager() const
{
    auto project = GetProject();
    return project->GetHotkeyManager();
}

UndoManager* EditorTab::GetUndoManager() const
{
    auto project = GetProject();
    return project->GetUndoManager();
}

ea::string EditorTab::GetHotkeyLabel(const EditorHotkey& info) const
{
    auto hotkeyManager = GetHotkeyManager();
    return hotkeyManager->GetHotkeyLabel(info);
}

}
