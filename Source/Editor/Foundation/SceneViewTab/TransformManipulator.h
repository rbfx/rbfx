// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
        ea::string GetUniqueName() { return "Editor.Scene:TransformGizmo"; }

        void SerializeInBlock(Archive& archive);
        void RenderSettings();

        Vector3 GetSnapValue(TransformGizmoOperation op) const;

        Vector3 snapPosition_{0.5f * Vector3::ONE};
        float snapRotation_{5.0f};
        float snapScale_{0.1f};
        bool screenRotation_{};
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

    /// Getters
    /// @{
    bool IsLocal() const { return isLocal_; }
    bool IsPivoted() const { return isPivoted_; }
    bool IsSelect() const { return operation_ == TransformGizmoOperation::None; }
    bool IsTranslate() const { return operation_ == TransformGizmoOperation::Translate; }
    bool IsRotate() const { return operation_ == TransformGizmoOperation::Rotate; }
    bool IsScale() const { return operation_ == TransformGizmoOperation::Scale; }
    /// @}

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "TransformGizmo"; }
    int GetToolbarPriority() const override { return 0; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    void Render(SceneViewPage& scenePage) override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    bool RenderTabContextMenu() override;
    bool RenderToolbar() override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

private:
    void EnsureGizmoInitialized(SceneViewPage& scenePage);
    void OnNodeTransformChanged(Node* node, const Transform& oldTransform);
    TransformGizmoAxes GetCurrentAxes() const;

    const WeakPtr<SettingsPage> settings_;

    WeakPtr<Scene> selectionScene_;
    unsigned selectionRevision_{};
    ea::optional<TransformNodesGizmo> transformNodesGizmo_;

    bool isLocal_{};
    bool isPivoted_{};
    TransformGizmoOperation operation_{TransformGizmoOperation::Translate};
};

} // namespace Urho3D
