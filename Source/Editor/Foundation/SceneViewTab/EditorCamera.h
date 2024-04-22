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
#include "Editor/Foundation/Shared/CameraController.h"

namespace Urho3D
{

void Foundation_EditorCamera(Context* context, SceneViewTab* sceneViewTab);

/// Camera controller used by Scene View.
class EditorCamera : public SceneViewAddon
{
    URHO3D_OBJECT(EditorCamera, SceneViewAddon);

public:
    using SettingsPage = SimpleSettingsPage<CameraController::Settings>;

    EditorCamera(SceneViewTab* owner, CameraController::SettingsPage* settings);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "Camera"; }
    int GetInputPriority() const override { return M_MAX_INT; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    void SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const override;
    /// @}

private:
    CameraController::PageState& GetOrInitializeState(SceneViewPage& scenePage) const;
    void LookAtPosition(SceneViewPage& scenePage, const Vector3& position) const;

    const WeakPtr<CameraController::SettingsPage> settings_;
    SharedPtr<CameraController> cameraController_;
    bool isActive_{};
};

}
