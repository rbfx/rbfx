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

#include "../../Foundation/Glue/ProjectGlue.h"

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/SystemUI/Widgets.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace
{

const auto Hotkey_Play = EditorHotkey{"Global.Launch"}.Ctrl().Press(KEY_P);

struct InternalStateContext
{
    WeakPtr<Project> project_;
    WeakPtr<GameViewTab> gameViewTab_;
    WeakPtr<SceneViewTab> sceneViewTab_;

    operator bool() const { return project_ && gameViewTab_ && sceneViewTab_; }
};

class InternalState : public InternalStateContext
{
public:
    explicit InternalState(const InternalStateContext& ctx)
        : InternalStateContext{ctx}
        , engine_{project_->GetSubsystem<Engine>()}
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
            closeGameViewTabAfter_ = !gameViewTab_->IsOpen();
            gameViewTab_->Open();

            tabToFocusAfter_ = sceneViewTab_;
            sceneViewTab_->SetupPluginContext();

            // Save modified resources so the game can load them
            const bool forceSaveResources = false;
            project_->SaveResourcesOnly(forceSaveResources);

            gameViewTab_->Focus();

            if (config)
                engine_->SetParameter(EP_MAIN_PLUGIN, config->mainPlugin_);
            else
                engine_->SetParameter(EP_MAIN_PLUGIN, Variant::EMPTY);
        }
        else if (tabToFocusAfter_)
        {
            tabToFocusAfter_->Focus();
            tabToFocusAfter_ = nullptr;

            if (closeGameViewTabAfter_)
                gameViewTab_->Close();
        }
        gameViewTab_->TogglePlayed();
    }

private:
    const WeakPtr<Engine> engine_;

    WeakPtr<EditorTab> tabToFocusAfter_;
    bool closeGameViewTabAfter_{};
};

}

void Foundation_ProjectGlue(Context* context, Project* project)
{
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    InternalStateContext ctx;
    ctx.project_ = project;
    ctx.gameViewTab_ = project->FindTab<GameViewTab>();
    ctx.sceneViewTab_ = project->FindTab<SceneViewTab>();
    if (!ctx)
        return;

    const auto state = ea::make_shared<InternalState>(ctx);

    hotkeyManager->BindHotkey(hotkeyManager, Hotkey_Play, [state] { state->TogglePlayedDefault(); });

    project->OnRenderProjectMenu.Subscribe(project, [state](Project* project)
    {
        auto hotkeyManager = project->GetHotkeyManager();
        auto launchManager = project->GetLaunchManager();

        const LaunchConfiguration* currentConfig = project->GetLaunchConfiguration();

        ui::Separator();

        const ea::string title = state->IsPlaying()
            ? ICON_FA_STOP " Stop"
            : Format(ICON_FA_PLAY " Launch \"{}\"", currentConfig ? currentConfig->name_ : LaunchConfiguration::UnspecifiedName);
        if (ui::MenuItem(title.c_str(), hotkeyManager->GetHotkeyLabel(Hotkey_Play).c_str()))
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

    project->OnRenderProjectToolbar.Subscribe(project, [state](Project* project)
    {
        auto launchManager = project->GetLaunchManager();

        const bool isPlaying = state->IsPlaying();

        {
            ui::BeginDisabled(isPlaying);

            const LaunchConfiguration* currentConfig = project->GetLaunchConfiguration();
            const char* previewValue = currentConfig ? currentConfig->name_.c_str() : LaunchConfiguration::UnspecifiedName.c_str();
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
            const char* title = isPlaying ? ICON_FA_STOP : ICON_FA_PLAY;
            const char* tooltip = isPlaying ? "Stop" : "Launch";
            if (Widgets::ToolbarButton(title, tooltip))
                state->TogglePlayedDefault();
        }
    });
}

}
