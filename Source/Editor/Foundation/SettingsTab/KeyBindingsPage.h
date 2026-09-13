// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/SettingsTab.h"

namespace Urho3D
{

void Foundation_KeyBindingsPage(Context* context, SettingsTab* settingsTab);

/// Tab that displays project settings.
class KeyBindingsPage : public SettingsPage
{
    URHO3D_OBJECT(KeyBindingsPage, SettingsPage)

public:
    explicit KeyBindingsPage(Context* context);

    /// Implement SettingsPage
    /// @{
    ea::string GetUniqueName() override { return "Editor.KeyBindings"; }
    bool IsSerializable() override { return false; }

    void SerializeInBlock(Archive& archive) override {}
    void RenderSettings() override;
    /// @}

private:

};

}
