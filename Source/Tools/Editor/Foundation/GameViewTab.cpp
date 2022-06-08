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

#include "../Foundation/GameViewTab.h"

#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Plugins/PluginManager.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Scene.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

// TODO(editor): Disable hotkeys during play

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_TogglePlay, "GameViewTab.TogglePlay", QUAL_CTRL, KEY_P);
URHO3D_EDITOR_HOTKEY(Hotkey_ReleaseInput, "GameViewTab.ReleaseInput", QUAL_SHIFT, KEY_ESCAPE);

}

void Foundation_GameViewTab(Context* context, ProjectEditor* projectEditor)
{
    projectEditor->AddTab(MakeShared<GameViewTab>(context));
}

class GameViewTab::PlayState : public Object
{
    URHO3D_OBJECT(PlayState, Object);

public:
    PlayState(Context* context, CustomBackbufferTexture* backbuffer)
        : Object(context)
        , renderer_(GetSubsystem<Renderer>())
        , pluginManager_(GetSubsystem<PluginManager>())
        , input_(GetSubsystem<Input>())
        , systemUi_(GetSubsystem<SystemUI>())
        , project_(GetSubsystem<ProjectEditor>())
        , backbuffer_(backbuffer)
    {
        renderer_->SetBackbufferRenderSurface(backbuffer_->GetTexture()->GetRenderSurface());
        backbuffer_->SetActive(true);
        GrabInput();
        pluginManager_->StartApplication();
        UpdatePreferredMouseSetup();
    }

    void GrabInput()
    {
        if (inputGrabbed_)
            return;

        input_->SetMouseVisible(preferredMouseVisible_);
        input_->SetMouseMode(preferredMouseMode_);
        input_->SetEnabled(true);
        systemUi_->SetPassThroughEvents(true);
        project_->SetGlobalHotkeysEnabled(false);
        project_->SetHighlightEnabled(true);

        inputGrabbed_ = true;
    }

    void ReleaseInput()
    {
        if (!inputGrabbed_)
            return;

        UpdatePreferredMouseSetup();
        input_->SetMouseVisible(true);
        input_->SetMouseMode(MM_ABSOLUTE);
        input_->SetEnabled(false);
        systemUi_->SetPassThroughEvents(false);
        project_->SetGlobalHotkeysEnabled(true);
        project_->SetHighlightEnabled(false);

        inputGrabbed_ = false;
    }

    bool IsInputGrabbed() const { return inputGrabbed_; }

    ~PlayState()
    {
        ReleaseInput();
        pluginManager_->StopApplication();
        backbuffer_->SetActive(false);
        renderer_->SetBackbufferRenderSurface(nullptr);
        renderer_->SetNumViewports(0);
    }

private:
    void UpdatePreferredMouseSetup()
    {
        preferredMouseVisible_ = input_->IsMouseVisible();
        preferredMouseMode_ = input_->GetMouseMode();
    }

    Renderer* renderer_{};
    PluginManager* pluginManager_{};
    Input* input_{};
    SystemUI* systemUi_{};
    ProjectEditor* project_{};

    CustomBackbufferTexture* backbuffer_{};

    bool inputGrabbed_{};

    bool preferredMouseVisible_{true};
    MouseMode preferredMouseMode_{MM_FREE};
};

GameViewTab::GameViewTab(Context* context)
    : EditorTab(context, "Game", "212a6577-8a2a-42d6-aaed-042d226c724c",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault,
        EditorTabPlacement::DockCenter)
    , backbuffer_(MakeShared<CustomBackbufferTexture>(context_))
{
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_TogglePlay, &GameViewTab::ToggleScenePlayed);
    hotkeyManager->BindHotkey(this, Hotkey_ReleaseInput, &GameViewTab::ReleaseInput);
}

GameViewTab::~GameViewTab()
{
}

bool GameViewTab::IsInputGrabbed() const
{
    return state_ && state_->IsInputGrabbed();
}

void GameViewTab::PlayScene(const ea::string& sceneName)
{
    lastPlayedScene_ = sceneName;
    if (state_)
        Stop();

    state_ = ea::make_unique<PlayState>(context_, backbuffer_);
    OnSimulationStarted(this);
}

void GameViewTab::Stop()
{
    if (!state_)
        return;

    state_ = nullptr;
    OnSimulationStopped(this);
}

void GameViewTab::PlayLastScene()
{
    PlayScene(lastPlayedScene_);
}

void GameViewTab::ToggleScenePlayed()
{
    if (IsPlaying())
        Stop();
    else
        PlayLastScene();
}

void GameViewTab::ReleaseInput()
{
    if (state_)
        state_->ReleaseInput();
}

void GameViewTab::RenderContent()
{
    backbuffer_->SetTextureSize(GetContentSize());
    backbuffer_->Update();

    if (state_)
    {
        Texture2D* sceneTexture = backbuffer_->GetTexture();
        ui::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));
    }

    if (state_)
    {
        const bool wasGrabbed = state_->IsInputGrabbed();
        const bool needGrab = ui::IsItemHovered() && ui::IsMouseClicked(MOUSEB_ANY);
        const bool needRelease = !ui::IsItemHovered() && ui::IsMouseClicked(MOUSEB_ANY);
        if (!wasGrabbed && needGrab)
            state_->GrabInput();
        else if (wasGrabbed && needRelease)
            state_->ReleaseInput();
    }
}

void GameViewTab::RenderContextMenuItems()
{
    auto project = GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    const char* togglePlayTitle = IsPlaying() ? ICON_FA_STOP " Stop" : ICON_FA_PLAY " Play";
    if (ui::MenuItem(togglePlayTitle, hotkeyManager->GetHotkeyLabel(Hotkey_TogglePlay).c_str()))
        ToggleScenePlayed();

    contextMenuSeparator_.Add();
}

}
