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
    {
    }

    bool IsPlaying() const { return gameViewTab_ && gameViewTab_->IsPlaying(); }

    void TogglePlayed()
    {
        if (!gameViewTab_)
            return;

        if (!gameViewTab_->IsPlaying())
        {
            tabToFocusAfter_ = sceneViewTab_;
            sceneViewTab_->SetupPluginContext();
            project_->Save();
            gameViewTab_->Focus();
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
    WeakPtr<EditorTab> tabToFocusAfter_;
};

}

void Foundation_ProjectEditorGlue(Context* context, ProjectEditor* project)
{
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    const auto state = ea::make_shared<InternalState>(project);

    hotkeyManager->BindHotkey(hotkeyManager, Hotkey_Play, [state] { state->TogglePlayed(); });

    project->OnRenderProjectMenu.Subscribe(project, [state](ProjectEditor* project)
    {
        HotkeyManager* hotkeyManager = project->GetHotkeyManager();
        const char* title = state->IsPlaying() ? ICON_FA_STOP " Stop" : ICON_FA_EJECT " Launch";
        if (ui::MenuItem(title, hotkeyManager->GetHotkeyLabel(Hotkey_Play).c_str()))
            state->TogglePlayed();
    });
}

}
