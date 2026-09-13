// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Core/IniHelpers.h"
#include "../../Foundation/SceneViewTab/SceneDebugInfo.h"

#include <Urho3D/Graphics/Camera.h>
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
    auto hud = GetSubsystem<DebugHud>();
    if (hud && hudVisible_)
    {
        const IntVector2 position = owner_->GetContentPosition();
        ui::SetCursorScreenPos(ToImGui(position.ToVector2()));
        hud->RenderUI(DEBUGHUD_SHOW_ALL);
    }

    if (Camera* camera = scenePage.renderer_->GetCamera())
        camera->SetFillMode(drawWireframe_ ? FILL_WIREFRAME : FILL_SOLID);
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
    if (Widgets::ToolbarButton(ICON_FA_BORDER_ALL, "Toggle Wireframe", drawWireframe_))
        drawWireframe_ = !drawWireframe_;

    Widgets::ToolbarSeparator();

    return true;
}

void SceneDebugInfo::WriteIniSettings(ImGuiTextBuffer& output)
{
    WriteIntToIni(output, "SceneDebugInfo.HudVisible", hudVisible_);
    WriteIntToIni(output, "SceneDebugInfo.DrawWireframe", drawWireframe_);
}

void SceneDebugInfo::ReadIniSettings(const char* line)
{
    if (const auto value = ReadIntFromIni(line, "SceneDebugInfo.HudVisible"))
        hudVisible_ = *value != 0;

    if (const auto value = ReadIntFromIni(line, "SceneDebugInfo.DrawWireframe"))
        drawWireframe_ = *value != 0;
}

}
