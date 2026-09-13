// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/SettingsTab.h"

#include <EASTL/set.h>

namespace Urho3D
{

void Foundation_PluginsPage(Context* context, SettingsTab* settingsTab);

/// Tab that displays project settings.
class PluginsPage : public SettingsPage
{
    URHO3D_OBJECT(PluginsPage, SettingsPage)

public:
    explicit PluginsPage(Context* context);

    /// Commands
    /// @{
    void Apply();
    void Discard();
    /// @}

    /// Implement SettingsPage
    /// @{
    ea::string GetUniqueName() override { return "Project.Plugins"; }
    bool IsSerializable() override { return false; }
    bool CanResetToDefault() override { return true; }

    void SerializeInBlock(Archive& archive) override {}
    void RenderSettings() override;
    void ResetToDefaults() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    void UpdateAvailablePlugins();
    void UpdateLoadedPlugins();

    const unsigned refreshInterval_{3000};
    bool refreshPlugins_{true};
    Timer refreshTimer_;
    ea::set<ea::string> availablePlugins_;

    unsigned revision_{};
    bool hasChanges_{};
    StringVector loadedPlugins_;
};

}
