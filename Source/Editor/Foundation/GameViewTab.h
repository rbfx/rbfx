// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Project/EditorTab.h"
#include "../Project/Project.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Utility/SceneRendererToTexture.h>

namespace Urho3D
{

void Foundation_GameViewTab(Context* context, Project* project);

/// Tab that renders Scene and enables Scene manipulation.
class GameViewTab : public EditorTab
{
    URHO3D_OBJECT(GameViewTab, EditorTab);

public:
    Signal<void()> OnSimulationStarted;
    Signal<void()> OnSimulationStopped;

    explicit GameViewTab(Context* context);
    ~GameViewTab() override;

    void Play();
    bool IsPlaying() const { return !!state_; }
    bool IsInputGrabbed() const;

    /// Commands
    /// @{
    void Stop();
    void TogglePlayed();
    void ReleaseInput();
    /// @}

    /// Implement EditorTab
    /// @{
    void RenderToolbar() override;
    void RenderContent() override;
    void RenderContextMenuItems() override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

private:
    void QuitApplication();

    class PlayState;

    SharedPtr<CustomBackbufferTexture> backbuffer_;

    ea::unique_ptr<PlayState> state_;
    bool hudVisible_{};
};

} // namespace Urho3D
