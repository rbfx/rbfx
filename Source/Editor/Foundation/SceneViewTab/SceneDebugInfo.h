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
};

}
