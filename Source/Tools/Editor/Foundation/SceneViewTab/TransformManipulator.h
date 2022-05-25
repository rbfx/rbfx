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

#include <Urho3D/SystemUI/TransformGizmo.h>

#include <EASTL/optional.h>

namespace Urho3D
{

void Foundation_TransformManipulator(Context* context, SceneViewTab* sceneViewTab);

/// Addon to manage scene selection with mouse and render debug geometry.
class TransformManipulator : public SceneViewAddon
{
    URHO3D_OBJECT(TransformManipulator, SceneViewAddon);

public:
    struct Settings
    {
        ea::string GetUniqueName() { return "SceneView.TransformGizmo"; }

        void SerializeInBlock(Archive& archive);
        void RenderSettings();

        float GetSnapValue(TransformGizmoOperation op) const;

        float snapPosition_{0.5f};
        float snapRotation_{5.0f};
        float snapScale_{0.1f};
    };
    using SettingsPage = SimpleSettingsPage<Settings>;

    TransformManipulator(SceneViewTab* owner, SettingsPage* settings);

    /// Commands
    /// @{
    void ToggleSpace() { isLocal_ = !isLocal_; }
    void TogglePivoted() { isPivoted_ = !isPivoted_; }
    void SetSelect() { operation_ = TransformGizmoOperation::None; }
    void SetTranslate() { operation_ = TransformGizmoOperation::Translate; }
    void SetRotate() { operation_ = TransformGizmoOperation::Rotate; }
    void SetScale() { operation_ = TransformGizmoOperation::Scale; }
    /// @}

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "TransformGizmo"; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    void UpdateAndRender(SceneViewPage& scenePage) override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;

    bool NeedTabContextMenu() const override { return true; }
    ea::string GetContextMenuTitle() const override { return "Transform Gizmo"; }
    void RenderTabContextMenu() override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

private:
    void EnsureGizmoInitialized(SceneViewPage& scenePage);
    void OnNodeTransformChanged(Node* node, const Transform& oldTransform);

    const WeakPtr<SettingsPage> settings_;

    WeakPtr<Scene> selectionScene_;
    unsigned selectionRevision_{};
    ea::optional<TransformNodesGizmo> transformNodesGizmo_;

    bool isLocal_{};
    bool isPivoted_{};
    TransformGizmoOperation operation_{TransformGizmoOperation::Translate};
};

}
