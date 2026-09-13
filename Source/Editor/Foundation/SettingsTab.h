// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/SettingsManager.h"
#include "../Project/Project.h"
#include "../Project/EditorTab.h"

namespace Urho3D
{

void Foundation_SettingsTab(Context* context, Project* project);

/// Tab that displays project settings.
class SettingsTab : public EditorTab
{
    URHO3D_OBJECT(SettingsTab, EditorTab)

public:
    explicit SettingsTab(Context* context);

    /// Implement EditorTab
    /// @{
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;

    void RenderContent() override;
    /// @}

protected:
    /// Implement EditorTab
    /// @{
    bool IsMarkedUnsaved() override;
    /// @}

private:
    void RenderSettingsTree();
    void RenderSettingsSubtree(const SettingsPageGroup& group, const ea::string& shortName);

    void RenderCurrentGroup();
    void RenderPage(const ea::string& section, SettingsPage* page);

    bool selectNextValidGroup_{};
    const SettingsPageGroup* selectedGroup_{};

    ea::optional<bool> useVerticalLayout_;
};

}
