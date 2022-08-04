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
