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

#include "../../Core/IniHelpers.h"
#include "../../Foundation/SceneViewTab/SceneDebugInfo.h"

#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_ToggleHud = EditorHotkey{"DebugInfo.ToggleHud"};

}

void Foundation_SceneDebugInfo(Context* context, SceneViewTab* sceneViewTab)
{
    auto project = sceneViewTab->GetProject();
    auto settingsManager = project->GetSettingsManager();

    auto settingsPage = MakeShared<SceneDebugInfo::SettingsPage>(context);
    settingsManager->AddPage(settingsPage);

    sceneViewTab->RegisterAddon<SceneDebugInfo>(settingsPage);
}

void SceneDebugInfo::Settings::SerializeInBlock(Archive& archive)
{
}

void SceneDebugInfo::Settings::RenderSettings()
{
}

SceneDebugInfo::SceneDebugInfo(SceneViewTab* owner, SettingsPage* settings)
    : SceneViewAddon(owner)
    , settings_(settings)
{
    auto hotkeyManager = owner_->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_ToggleHud, &SceneDebugInfo::ToggleHud);
}

void SceneDebugInfo::Render(SceneViewPage& scenePage)
{
    const ImGuiContext& g = *GImGui;
    const ImGuiWindow* window = g.CurrentWindow;
    const ImRect rect = ImRound(window->ContentRegionRect);

    auto hud = GetSubsystem<DebugHud>();
    if (hud && hudVisible_)
    {
        ImGuiWindow* window = ui::GetCurrentWindow();
        ui::SetCursorScreenPos(rect.Min);
        hud->RenderUI(DEBUGHUD_SHOW_ALL);
    }
}

void SceneDebugInfo::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

bool SceneDebugInfo::RenderTabContextMenu()
{
    return true;
}

bool SceneDebugInfo::RenderToolbar()
{
    if (Widgets::ToolbarButton(ICON_FA_BUG, "Toggle Debug HUD", hudVisible_))
        ToggleHud();

    Widgets::ToolbarSeparator();

    return true;
}

void SceneDebugInfo::WriteIniSettings(ImGuiTextBuffer& output)
{
    WriteIntToIni(output, "SceneDebugInfo.HudVisible", hudVisible_);
}

void SceneDebugInfo::ReadIniSettings(const char* line)
{
    if (const auto value = ReadIntFromIni(line, "SceneDebugInfo.HudVisible"))
        hudVisible_ = *value != 0;
}

}
