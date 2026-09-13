// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

#include <Urho3D/SystemUI/DebugHud.h>

#include <EASTL/optional.h>

namespace Urho3D
{

void Foundation_SceneDebugInfo(Context* context, SceneViewTab* sceneViewTab);

/// Addon to manage debug HUD in scene.
class SceneDebugInfo : public SceneViewAddon
{
    URHO3D_OBJECT(SceneDebugInfo, SceneViewAddon);

public:
    struct Settings
    {
        ea::string GetUniqueName() { return "Editor.Scene:DebugInfo"; }

        void SerializeInBlock(Archive& archive);
        void RenderSettings();
    };
    using SettingsPage = SimpleSettingsPage<Settings>;

    SceneDebugInfo(SceneViewTab* owner, SettingsPage* settings);

    /// Commands
    /// @{
    void ToggleHud() { hudVisible_ = !hudVisible_; }
    /// @}

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "DebugInfo"; }
    int GetToolbarPriority() const override { return 100; }
    void Render(SceneViewPage& scenePage) override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    bool RenderTabContextMenu() override;
    bool RenderToolbar() override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

private:
    const WeakPtr<SettingsPage> settings_;

    bool hudVisible_{};
    bool drawWireframe_{};
};

}
