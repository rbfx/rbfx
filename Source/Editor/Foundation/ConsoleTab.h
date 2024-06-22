// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Project/EditorTab.h"
#include "../Project/Project.h"

namespace Urho3D
{

void Foundation_ConsoleTab(Context* context, Project* project);

/// Tab that displays application log and enables console input.
class ConsoleTab : public EditorTab
{
    URHO3D_OBJECT(ConsoleTab, EditorTab)

public:
    explicit ConsoleTab(Context* context);

    /// Implement EditorTab
    /// @{
    void RenderContent() override;
    void RenderContextMenuItems() override;

    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    /// @}
};

} // namespace Urho3D
