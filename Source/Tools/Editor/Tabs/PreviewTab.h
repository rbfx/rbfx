//
// Copyright (c) 2017-2019 the rbfx project.
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

#pragma once


#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Scene/Scene.h>
#include "Tabs/Scene/SceneTab.h"


namespace Urho3D
{

enum SceneSimulationStatus
{
    SCENE_SIMULATION_STOPPED,
    SCENE_SIMULATION_RUNNING,
    SCENE_SIMULATION_PAUSED,
};

class PreviewTab : public Tab
{
    URHO3D_OBJECT(PreviewTab, Tab)
public:
    explicit PreviewTab(Context* context);

    bool RenderWindowContent() override;
    /// Set color of view texture to black.
    void Clear();

    /// Render play/pause/restore/step/store buttons.
    void RenderButtons();

    /// Start playing a scene. If scene is already playing this does nothing.
    void Play();
    /// Pause playing a scene. If scene is stopped or paused this does nothing.
    void Pause();
    /// Toggle between play/pause states.
    void Toggle();
    /// Simulate single frame. If scene is not paused this does nothing.
    void Step(float timeStep);
    /// Stop scene simulation. If scene is already stopped this does nothing.
    void Stop();
    /// Take a snapshot of current scene state and use it as "master" state. Stopping simulation will revert to this new state. Clears all scene undo actions!
    void Snapshot();
    /// Returns true when scene is playing or paysed.
    bool IsScenePlaying() const { return simulationStatus_ != SCENE_SIMULATION_STOPPED; }
    /// Returns current scene simulation status.
    SceneSimulationStatus GetSceneSimulationStatus() const { return simulationStatus_; }

protected:
    ///
    IntRect UpdateViewRect() override;
    ///
    void OnBeforeBegin() override;
    ///
    void OnAfterEnd() override;
    /// Goes through scene, finds CameraViewport components and creates required viewports in the editor.
    void UpdateViewports();
    /// Handle addition or removal of CameraViewport component.
    void OnComponentUpdated(Component* component);
    /// Preview tab grabs input. Scene simulation assumes full control of the input.
    void GrabInput();
    /// Release input to the editor. Game components should not interfere with the input when Input::ShouldIgnoreInput() returns true.
    void ReleaseInput();
    ///
    void RenderUI();

    /// Last view rectangle.
    IntRect viewRect_{};
    /// Texture used to display preview.
    SharedPtr<Texture2D> view_{};

    /// Flag controlling scene updates in the viewport.
    SceneSimulationStatus simulationStatus_ = SCENE_SIMULATION_STOPPED;
    /// Temporary storage of scene data used in play/pause functionality.
    SceneState sceneState_;
    /// Temporary storage of scene data used when plugins are being reloaded.
    SceneState sceneReloadState_;
    /// Time since ESC was last pressed. Used for double-press ESC to exit scene simulation.
    unsigned lastEscPressTime_ = 0;
    /// Flag indicating game view assumed control of the input.
    bool inputGrabbed_ = false;
    /// Mouse visibility expected by the played scene. Will be set when input is grabbed.
    bool sceneMouseVisible_ = true;
    /// Mouse mode expected by the played scene. Will be set when input is grabbed.
    MouseMode sceneMouseMode_ = MM_FREE;
    /// Ratio for dimming non-scene windows when scene is being played.
    float dim_ = 0.0f;
};

}
