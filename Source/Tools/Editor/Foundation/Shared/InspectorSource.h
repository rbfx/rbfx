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
