// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/SceneViewTab/SceneRmlUiPreview.h"

#include "../../Core/IniHelpers.h"

#include <Urho3D/RmlUI/RmlUI.h>
#include <Urho3D/RmlUI/RmlUIManager.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_Toggle = EditorHotkey{"RmlUiPreview.Toggle"}.Ctrl().Press(KEY_U);

} // namespace

void Foundation_SceneRmlUiPreview(Context* context, SceneViewTab* sceneViewTab)
{
    sceneViewTab->RegisterAddon<SceneRmlUiPreview>();
}

SceneRmlUiPreview::SceneRmlUiPreview(SceneViewTab* owner)
    : SceneViewAddon(owner)
{
    auto hotkeyManager = owner_->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_Toggle, &SceneRmlUiPreview::Toggle);
}

void SceneRmlUiPreview::ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed)
{
    const auto manager = scenePage.scene_->GetComponent<RmlUIManager>();
    if (!manager)
        return;

    manager->SetOwner(isEnabled_ ? scenePage.rmlUi_ : nullptr);

    if (!isEnabled_ || mouseConsumed || !ui::IsItemHovered())
        return;

    static constexpr ea::pair<MouseButton, int> mouseButtons[]{
        {MOUSEB_LEFT, 0},
        {MOUSEB_RIGHT, 1},
        {MOUSEB_MIDDLE, 2},
    };

    auto* rmlContext = scenePage.rmlUi_->GetRmlContext();
    if (!rmlContext)
        return;

    const IntVector2 mousePosition = scenePage.mousePosition_.ToIntVector2();
    const float mouseWheel = ui::GetMouseWheel();

    rmlContext->ProcessMouseMove(mousePosition.x_, mousePosition.y_, 0);
    rmlContext->ProcessMouseWheel({0.0f, -mouseWheel}, 0);
    for (const auto& [mouseButton, index] : mouseButtons)
    {
        if (ui::IsMouseClicked(mouseButton))
            rmlContext->ProcessMouseButtonDown(index, 0);
        if (ui::IsMouseReleased(mouseButton))
            rmlContext->ProcessMouseButtonUp(index, 0);
    }

    mouseConsumed = true;
}

void SceneRmlUiPreview::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
    hotkeyManager->InvokeFor(this);
}

bool SceneRmlUiPreview::RenderToolbar()
{
    const char* tooltip = isEnabled_ ? "Disable RML UI Preview" : "Enable RML UI Preview";
    if (Widgets::ToolbarButton(ICON_FA_TABLE_COLUMNS, tooltip, isEnabled_))
        isEnabled_ = !isEnabled_;

    Widgets::ToolbarSeparator();

    return true;
}

void SceneRmlUiPreview::WriteIniSettings(ImGuiTextBuffer& output)
{
    WriteIntToIni(output, "RmlUiPreview.IsEnabled", isEnabled_);
}

void SceneRmlUiPreview::ReadIniSettings(const char* line)
{
    if (const auto value = ReadIntFromIni(line, "RmlUiPreview.IsEnabled"))
        isEnabled_ = *value != 0;
}

} // namespace Urho3D
