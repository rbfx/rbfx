// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

namespace Urho3D
{

void Foundation_SceneScreenshot(Context* context, SceneViewTab* sceneViewTab);

/// Addon to manage scene selection with mouse and render debug geometry.
class SceneScreenshot : public SceneViewAddon
{
    URHO3D_OBJECT(SceneScreenshot, SceneViewAddon);

public:
    explicit SceneScreenshot(SceneViewTab* owner);

    void TakeScreenshotWithPopup();
    void TakeScreenshotDelayed(Scene* scene, Camera* camera, const IntVector2& resolution);
    void TakeScreenshotNow(Scene* scene, Camera* camera, const IntVector2& resolution);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "Screenshot"; }
    void Render(SceneViewPage& page) override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    bool RenderTabContextMenu() override;
    /// @}

private:
    Camera* FindCameraInSelection(const SceneSelection& selection) const;
    void InitializePopup(SceneViewPage& page);
    void RenderPopup();

    ea::string GenerateFileName(const IntVector2& size) const;

    bool openPending_{};

    bool keepPopupOpen_{};
    bool useInSceneCamera_{true};
    unsigned resolutionOption_{0};
    IntVector2 resolution_{1920, 1080};

    WeakPtr<Scene> scene_;
    WeakPtr<Camera> sceneCamera_;
    WeakPtr<Camera> editorCamera_;
};

}
