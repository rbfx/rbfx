// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/SettingsTab/KeyBindingsPage.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_KeyBindingsPage(Context* context, SettingsTab* settingsTab)
{
    auto project = settingsTab->GetProject();
    auto settingsManager = project->GetSettingsManager();
    settingsManager->AddPage(MakeShared<KeyBindingsPage>(context));
}

KeyBindingsPage::KeyBindingsPage(Context* context)
    : SettingsPage(context)
{
}

void KeyBindingsPage::RenderSettings()
{
    auto project = GetSubsystem<Project>();
    auto hotkeyManager = project->GetHotkeyManager();

    ui::Text("TODO: No, you cannot rebind those yet. PRs are welcome.");

    if (ui::BeginTable("Hotkeys", 3))
    {
        ui::TableSetupColumn("Command");
        ui::TableSetupColumn("Press");
        ui::TableSetupColumn("Hold");
        ui::TableHeadersRow();

        for (const auto& [command, _] : hotkeyManager->GetBindings())
        {
            const EditorHotkey& hotkey = hotkeyManager->GetHotkey(command);
            const ea::string qualifiers = hotkey.GetQualifiersString();
            const ea::string press = hotkey.GetPressString();
            const ea::string hold = hotkey.GetHoldString();

            ui::TableNextRow();

            ui::TableNextColumn();
            ui::Text("%s", command.c_str());

            ui::TableNextColumn();
            ui::Text("%s", !press.empty() ? (qualifiers + press).c_str() : "");

            ui::TableNextColumn();
            ui::Text("%s", !hold.empty() ? (qualifiers + hold).c_str() : "");
        }
        ui::EndTable();
    }
}

}
