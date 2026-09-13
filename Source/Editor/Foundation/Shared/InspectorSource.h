// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Signal.h>

namespace Urho3D
{

class EditorTab;
class HotkeyManager;

/// Interface used to provide content for inspector tab.
class InspectorSource
{
public:
    Signal<void()> OnActivated;

    /// Return owner tab.
    virtual EditorTab* GetOwnerTab() { return nullptr; }
    /// Return whether the inspector is connected to undo stack.
    virtual bool IsUndoSupported() { return false; }

    /// Update and render inspector contents.
    virtual void RenderContent() {}
    /// Update and render tab context menu.
    virtual void RenderContextMenuItems() {}
    /// Render main menu when the inspector tab is focused.
    virtual void RenderMenu() {}
    /// Apply hotkeys when the inspector tab is focused.
    virtual void ApplyHotkeys(HotkeyManager* hotkeyManager) {}
};

}
