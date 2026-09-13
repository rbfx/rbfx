// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/SettingsTab.h"
#include "../../Project/LaunchManager.h"

#include <EASTL/set.h>

namespace Urho3D
{

void Foundation_LaunchPage(Context* context, SettingsTab* settingsTab);

/// Tab that displays project settings.
class LaunchPage : public SettingsPage
{
    URHO3D_OBJECT(LaunchPage, SettingsPage)

public:
    explicit LaunchPage(Context* context);

    /// Implement SettingsPage
    /// @{
    ea::string GetUniqueName() override { return "Project.Launch"; }
    bool IsSerializable() override { return false; }

    void SerializeInBlock(Archive& archive) override {}
    void RenderSettings() override;
    /// @}

private:
    ea::string GetUnusedConfigurationName() const;

    void RenderConfiguration(unsigned index, LaunchConfiguration& config);
    void RenderMainPlugin(ea::string& mainPlugin);

    WeakPtr<LaunchManager> launchManager_;

    ea::vector<unsigned> pendingConfigurationsRemoved_;
};

}
