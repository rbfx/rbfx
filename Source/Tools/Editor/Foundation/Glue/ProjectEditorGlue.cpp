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

#include "../../Foundation/Glue/ProjectEditorGlue.h"

#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_Play, "Global.Launch", QUAL_CTRL, KEY_P);

class InternalState
{
public:
    explicit InternalState(ProjectEditor* project)
        : project_{project}
        , gameViewTab_{project->FindTab<GameViewTab>()}
        , sceneViewTab_{project->FindTab<SceneViewTab>()}
        , pluginManager_{project->GetSubsystem<PluginManager>()}
        , launchManager_{project->GetLaunchManager()}
    {
    }

    bool IsPlaying() const { return gameViewTab_ && gameViewTab_->IsPlaying(); }

    void TogglePlayedDefault()
    {
        const LaunchConfiguration* currentConfig = project_->GetLaunchConfiguration();
        TogglePlayed(currentConfig);
    }

    void TogglePlayed(const LaunchConfiguration* config)
    {
        if (!gameViewTab_)
            return;

        if (!gameViewTab_->IsPlaying())
        {
            tabToFocusAfter_ = sceneViewTab_;
            sceneViewTab_->SetupPluginContext();
            project_->Save();
            gameViewTab_->Focus();

            if (config)
                pluginManager_->SetParameter(Plugin_MainPlugin, config->mainPlugin_);
            else
                pluginManager_->SetParameter(Plugin_MainPlugin, Variant::EMPTY);
        }
        else if (tabToFocusAfter_)
        {
            tabToFocusAfter_->Focus();
            tabToFocusAfter_ = nullptr;
        }
        gameViewTab_->TogglePlayed();
    }

private:
    const WeakPtr<ProjectEditor> project_;
    const WeakPtr<GameViewTab> gameViewTab_;
    const WeakPtr<SceneViewTab> sceneViewTab_;

    const WeakPtr<PluginManager> pluginManager_;
    const WeakPtr<LaunchManager> launchManager_;

    WeakPtr<EditorTab> tabToFocusAfter_;
};

}

void Foundation_ProjectEditorGlue(Context* context, ProjectEditor* project)
{
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    const auto state = ea::make_shared<InternalState>(project);

    hotkeyManager->BindHotkey(hotkeyManager, Hotkey_Play, [state] { state->TogglePlayedDefault(); });

    project->OnRenderProjectMenu.Subscribe(project, [state](ProjectEditor* project)
    {
        auto hotkeyManager = project->GetHotkeyManager();
        auto launchManager = project->GetLaunchManager();

        const char* title = state->IsPlaying() ? ICON_FA_STOP " Stop" : ICON_FA_EJECT " Launch Current";
        if (ui::MenuItem(title, hotkeyManager->GetHotkeyLabel(Hotkey_Play).c_str()))
            state->TogglePlayedDefault();

        if (ui::BeginMenu("Launch Other"))
        {
            for (const ea::string& name : launchManager->GetSortedConfigurations())
            {
                const LaunchConfiguration* config = launchManager->FindConfiguration(name);
                if (config && ui::MenuItem(config->name_.c_str()))
                    state->TogglePlayed(config);
            }
            ui::EndMenu();
        }
    });

    project->OnRenderProjectToolbar.Subscribe(project, [state](ProjectEditor* project)
    {
        auto launchManager = project->GetLaunchManager();

        const bool isPlaying = state->IsPlaying();

        {
            ui::BeginDisabled(isPlaying);

            const LaunchConfiguration* currentConfig = project->GetLaunchConfiguration();
            const char* previewValue = currentConfig ? currentConfig->name_.c_str() : "(undefined)";
            const auto previewSize = ui::CalcTextSize(previewValue);

            Widgets::ToolbarSeparator();
            ui::SetNextItemWidth(previewSize.x + 2 * previewSize.y);
            if (ui::BeginCombo("##Config", previewValue))
            {
                for (const ea::string& name : launchManager->GetSortedConfigurations())
                {
                    const LaunchConfiguration* config = launchManager->FindConfiguration(name);
                    if (config && ui::Selectable(config->name_.c_str(), config == currentConfig))
                        project->SetLaunchConfigurationName(config->name_);
                }
                ui::EndCombo();
            }
            if (ui::IsItemHovered())
                ui::SetTooltip("Select a launch configuration, see Settings->Project->Launch");
            ui::SameLine();

            ui::EndDisabled();
        }

        {
            const char* title = isPlaying ? ICON_FA_STOP : ICON_FA_EJECT;
            const char* tooltip = isPlaying ? "Stop" : "Launch";
            if (Widgets::ToolbarButton(title, tooltip))
                state->TogglePlayedDefault();
        }
    });
}

}
