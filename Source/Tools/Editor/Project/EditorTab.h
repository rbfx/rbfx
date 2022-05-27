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

#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Signal.h>
#include <Urho3D/SystemUI/SystemUI.h>

namespace Urho3D
{

class HotkeyManager;
class ProjectEditor;

enum class EditorTabFlag
{
    None = 0,
    NoContentPadding = 1 << 0,
    OpenByDefault = 1 << 1,
    UndoSupported = 1 << 2
};
URHO3D_FLAGSET(EditorTabFlag, EditorTabFlags);

enum class EditorTabPlacement
{
    Floating,
    DockCenter,
    DockLeft,
    DockRight,
    DockBottom,
};

/// Interface for entity configurable via INI file.
class EditorConfigurable : public Object
{
    URHO3D_OBJECT(EditorConfigurable, Object);

public:
    explicit EditorConfigurable(Context* context) : Object(context) {}

    /// Write all UI settings to text INI file.
    virtual void WriteIniSettings(ImGuiTextBuffer& output) = 0;
    /// Read one line of text INI file. May be called several times.
    virtual void ReadIniSettings(const char* line) = 0;
    /// Return entry that should be read.
    virtual ea::string GetIniEntry() const = 0;
};

/// Helper class to spawn separators only once.
class SeparatorHelper
{
public:
    void Add()
    {
        if (!added_)
            ui::Separator();
        added_ = true;
    }

    void Reset()
    {
        added_ = false;
    }

private:
    bool added_{};
};

/// Base class for any Editor tab.
/// It's recommended to create exactly one instance of the tab for the project lifetime.
class EditorTab : public EditorConfigurable
{
    URHO3D_OBJECT(EditorTab, EditorConfigurable);

public:
    Signal<void()> OnRenderContextMenu;

    EditorTab(Context* context, const ea::string& title, const ea::string& guid,
        EditorTabFlags flags, EditorTabPlacement placement);
    ~EditorTab() override;

    /// Render contents of the tab. Returns false if closed.
    void UpdateAndRender();
    /// Open tab without focusing.
    void Open() { openPending_ = true; }
    /// Open tab if it's closed and focus on it.
    void Focus() { focusPending_ = true; }
    /// Close tab.
    void Close() { open_ = false; }

    /// Render main menu of the tab.
    virtual void UpdateAndRenderMenu() {}
    /// Called when all tabs are created and multi-tab plugins can be safely applied.
    virtual void ApplyPlugins();
    /// Called when project is fully loaded.
    virtual void OnProjectLoaded() {}
    /// Enumerates all unsaved items corresponding to this tab.
    virtual void EnumerateUnsavedItems(ea::vector<ea::string>& items) {}

    /// Implement EditorConfigurable
    /// @{
    void WriteIniSettings(ImGuiTextBuffer& output) override;
    void ReadIniSettings(const char* line) override;
    ea::string GetIniEntry() const override { return GetUniqueId(); }
    /// @}

    /// Return properties of the tab.
    /// @{
    const ea::string& GetTitle() const { return title_; }
    const ea::string& GetUniqueId() const { return uniqueId_; }
    EditorTabFlags GetFlags() const { return flags_; }
    EditorTabPlacement GetPlacement() const { return placement_; }
    bool IsOpen() const { return open_; }
    bool IsUndoSupported() const { return flags_.Test(EditorTabFlag::UndoSupported); }
    /// @}

    /// Return current project.
    ProjectEditor* GetProject() const;

protected:
    /// Render contents of the tab.
    virtual void UpdateAndRenderContent() {}
    /// Render context menu of the tab.
    virtual void UpdateAndRenderContextMenuItems() {}

    /// Return whether the document is modified and prompt to save should be shown.
    virtual bool IsMarkedUnsaved() { return false; }
    /// Update tab in focus.
    virtual void UpdateFocused() {};
    /// Apply scoped hotkeys for this tab.
    virtual void ApplyHotkeys(HotkeyManager* hotkeyManager);

    /// Helper for building menu
    SeparatorHelper contextMenuSeparator_;

private:
    void UpdateAndRenderWindow();
    void UpdateAndRenderContextMenu();

    const ea::string title_;
    const ea::string guid_;
    const ea::string uniqueId_;
    const EditorTabFlags flags_;
    const EditorTabPlacement placement_;

    bool focusPending_{};
    bool openPending_{};
    bool wasOpen_{};
    bool open_{};

    ImGuiWindowFlags windowFlags_{};
};

}
