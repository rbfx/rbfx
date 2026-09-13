// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

namespace Urho3D
{

class EditorTab;
class HotkeyManager;

/// Interface used to provide content for hierarchy browser tab.
class HierarchyBrowserSource
{
public:
    /// Return owner tab.
    virtual EditorTab* GetOwnerTab() = 0;

    /// Update and render tab contents.
    virtual void RenderContent() {}
    /// Update and render tab context menu.
    virtual void RenderContextMenuItems() {}
    /// Render main menu when the hierarchy browser tab is focused.
    virtual void RenderMenu() {}
    /// Apply hotkeys when the hierarchy browser tab is focused.
    virtual void ApplyHotkeys(HotkeyManager* hotkeyManager) {}
};

}
