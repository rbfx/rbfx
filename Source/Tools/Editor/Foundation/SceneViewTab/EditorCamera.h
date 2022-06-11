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

void Foundation_EditorCamera(Context* context, SceneViewTab* sceneViewTab);

/// Camera controller used by Scene View.
class EditorCamera : public SceneViewAddon
{
    URHO3D_OBJECT(EditorCamera, SceneViewAddon);

public:
    struct Settings
    {
        ea::string GetUniqueName() { return "SceneView.Camera"; }

        void SerializeInBlock(Archive& archive);
        void RenderSettings();

        float mouseSensitivity_{0.25f};
        float minSpeed_{2.0f};
        float maxSpeed_{10.0f};
        float acceleration_{1.0f};
        float shiftFactor_{4.0f};
    };
    using SettingsPage = SimpleSettingsPage<Settings>;

    struct PageState
    {
        Vector3 lastCameraPosition_;
        Quaternion lastCameraRotation_;
        float yaw_{};
        float pitch_{};
        float currentMoveSpeed_{};

        PageState();
        void LookAt(const Vector3& position, const Vector3& target);
        void SerializeInBlock(Archive& archive);
    };

    EditorCamera(SceneViewTab* owner, SettingsPage* settings);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "Camera"; }
    int GetInputPriority() const override { return M_MAX_INT; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    void SerializePageState(Archive& archive, const char* name, ea::any& stateWrapped) const override;
    /// @}

private:
    PageState& GetOrInitializeState(SceneViewPage& scenePage) const;
    void UpdateState(SceneViewPage& scenePage, PageState& state);

    Vector2 GetMouseMove() const;
    Vector3 GetMoveDirection() const;
    bool GetMoveAccelerated() const;

    const WeakPtr<SettingsPage> settings_;
    bool isActive_{};
};

}
