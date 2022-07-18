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
