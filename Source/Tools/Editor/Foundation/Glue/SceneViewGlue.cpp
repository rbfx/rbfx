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

#include "../../Foundation/Glue/SceneViewGlue.h"
#include "../../Foundation/SceneViewTab/SceneHierarchy.h"

namespace Urho3D
{

namespace
{

URHO3D_EDITOR_HOTKEY(Hotkey_Play, "ScenePlayerLauncher.Play", QUAL_CTRL, KEY_P);

}

void Foundation_SceneViewGlue(Context* context, SceneViewTab* sceneViewTab)
{
    auto project = sceneViewTab->GetProject();
    const WeakPtr<HierarchyBrowserTab> hierarchyBrowserTab{project->FindTab<HierarchyBrowserTab>()};
    const WeakPtr<GameViewTab> gameViewTab{project->FindTab<GameViewTab>()};

    sceneViewTab->RegisterAddon<ScenePlayerLauncher>(gameViewTab);

    auto source = MakeShared<SceneHierarchy>(sceneViewTab);
    sceneViewTab->OnFocused.Subscribe(source.Get(), [source, hierarchyBrowserTab](Object* receiver)
    {
        if (hierarchyBrowserTab)
            hierarchyBrowserTab->ConnectToSource(source);
    });
}

ScenePlayerLauncher::ScenePlayerLauncher(SceneViewTab* owner, GameViewTab* gameViewTab)
    : SceneViewAddon(owner)
    , gameViewTab_(gameViewTab)
{
    auto project = owner_->GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();
    hotkeyManager->BindHotkey(this, Hotkey_Play, &ScenePlayerLauncher::PlayCurrentScene);

    gameViewTab_->OnSimulationStopped.Subscribe(this, &ScenePlayerLauncher::FocusSceneViewTab);
}

ScenePlayerLauncher::~ScenePlayerLauncher()
{
}

void ScenePlayerLauncher::PlayCurrentScene()
{
    SceneViewPage* activePage = owner_->GetActivePage();
    if (!gameViewTab_ || !activePage)
        return;

    auto project = owner_->GetProject();
    project->Save();
    gameViewTab_->Focus();
    gameViewTab_->PlayScene(owner_->GetActiveResourceName());
}

bool ScenePlayerLauncher::RenderTabContextMenu()
{
    auto project = owner_->GetProject();
    HotkeyManager* hotkeyManager = project->GetHotkeyManager();

    const bool enabled = owner_->GetActivePage() != nullptr;
    if (ui::MenuItem("Play Current Scene", hotkeyManager->GetHotkeyLabel(Hotkey_Play).c_str(), false, enabled))
        PlayCurrentScene();

    return true;
}

void ScenePlayerLauncher::FocusSceneViewTab()
{
    if (owner_)
        owner_->Focus();
}

}
