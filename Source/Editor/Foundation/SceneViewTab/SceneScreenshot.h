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
