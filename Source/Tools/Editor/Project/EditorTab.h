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

class ProjectEditor;

enum class EditorTabFlag
{
    None = 0,
    NoContentPadding = 1 << 0,
    OpenByDefault = 1 << 1,
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

class EditorTab : public Object
{
    URHO3D_OBJECT(EditorTab, Object);

public:
    Signal<void()> OnRenderContextMenu;

    EditorTab(Context* context, const ea::string& title, const ea::string& guid,
        EditorTabFlags flags, EditorTabPlacement placement);
    ~EditorTab() override;

    /// Render contents of the tab. Returns false if closed.
    void RenderUI();
    /// Open tab without focusing.
    void Open() { openPending_ = true; }
    /// Open tab if it's closed and focus on it.
    void Focus() { focusPending_ = true; }

    /// Serialize UI settings.
    /// @{
    virtual void WriteIniSettings(ImGuiTextBuffer* output);
    virtual void ReadIniSettings(const char* line);
    /// @}

    /// Return properties of the tab.
    /// @{
    const ea::string& GetTitle() const { return title_; }
    const ea::string& GetUniqueId() const { return uniqueId_; }
    EditorTabFlags GetFlags() const { return flags_; }
    EditorTabPlacement GetPlacement() const { return placement_; }
    bool IsOpen() const { return open_; }
    /// @}

protected:
    /// Render contents of the tab.
    virtual void RenderContentUI() {}
    /// Return whether the document is modified and prompt to save should be shown.
    virtual bool IsModified() { return false; }
    /// Update tab in focus.
    virtual void UpdateFocused() {};

    /// Return current project.
    ProjectEditor* GetProject() const;

private:
    void RenderWindowUI();

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
