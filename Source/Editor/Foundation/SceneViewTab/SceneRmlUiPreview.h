// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/SettingsManager.h"
#include "../../Foundation/SceneViewTab.h"

#include <Urho3D/Graphics/Drawable.h>
#include <Urho3D/Graphics/OctreeQuery.h>

namespace Urho3D
{

void Foundation_SceneRmlUiPreview(Context* context, SceneViewTab* sceneViewTab);

/// Addon to manage RML UI preview in scene view.
class SceneRmlUiPreview : public SceneViewAddon
{
    URHO3D_OBJECT(SceneRmlUiPreview, SceneViewAddon);

public:
    explicit SceneRmlUiPreview(SceneViewTab* owner);

    /// Commands
    /// @{
    void Toggle() { isEnabled_ = !isEnabled_; }
    /// @}

    /// Getters
    /// @{
    bool IsEnabled() const { return isEnabled_; }
    /// @}

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "RmlUiPreview"; }
    int GetInputPriority() const override { return M_MAX_INT; }
    void ProcessInput(SceneViewPage& scenePage, bool& mouseConsumed) override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    bool RenderToolbar() override;
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}

private:
    bool isEnabled_{};
};

} // namespace Urho3D
