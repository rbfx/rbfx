//
// Copyright (c) 2017-2020 the rbfx project.
// Copyright (c) 2017 Eugene Kozlov
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


#include <Urho3D/Precompiled.h>
#include <Urho3D/Core/Signal.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <Toolbox/SystemUI/AttributeInspector.h>


namespace Urho3D
{

class UndoStack;

class IHierarchyProvider
{
public:
    /// Render hierarchy window.
    virtual void RenderHierarchy() = 0;
};

class Tab : public Object
{
    URHO3D_OBJECT(Tab, Object);
public:
    /// Construct.
    explicit Tab(Context* context);
    /// Destruct.
    ~Tab() override;
    /// Render content of tab window. Returns false if tab was closed.
    virtual bool RenderWindowContent() = 0;
    /// Render toolbar buttons.
    virtual void RenderToolbarButtons() { }
    /// Render tab content.
    virtual bool RenderWindow();
    /// Called when window is focused.
    virtual void OnUpdateFocused() { };
    /// Save ui settings.
    virtual void OnSaveUISettings(ImGuiTextBuffer* buf);
    /// Load ui settings.
    virtual const char* OnLoadUISettings(const char* name, const char* line);
    /// Load a file from resource path.
    virtual bool LoadResource(const ea::string& resourcePath);
    /// Save tab contents to a resource file.
    virtual bool SaveResource();
    /// Set scene view tab title.
    void SetTitle(const ea::string& title);
    /// Get scene view tab title.
    ea::string GetTitle() const { return title_; }
    /// Returns title which uniquely identifies scene tab in imgui.
    ea::string GetUniqueTitle() const { return uniqueTitle_;}
    /// Returns title which uniquely identifies scene tab in imgui.
    ea::string GetUniqueName() const { return uniqueName_;}
    /// Return unique object id.
    ea::string GetID() const { return id_; }
    /// Set unique object id.
    void SetID(const ea::string& id);
    /// Returns true of tab is utility window.
    bool IsUtility() const { return isUtility_; }
    /// Position tab automatically to most appropriate place.
    void AutoPlace() { autoPlace_ = true; }
    /// Returns true when tab is open.
    bool IsOpen() const { return open_; }
    /// Open/close tab without permanently removing it.
    void SetOpen(bool open) { open_ = open; }
    /// Make tab active.
    void Activate() { activateTab_ = true; }
    /// Returns true when loaded resource was modified.
    bool IsModified() const { return modified_; }
    /// Closes current tab and unloads it's contents from memory.
    virtual void Close() { open_ = false; }
    /// Clear any user selection tracked by this tab.
    virtual void ClearSelection() { }
    /// Serialize or deserialize selection.
    virtual bool SerializeSelection(Archive& archive) { return false; }
    /// Serialize current user selection into a buffer and return it.
    ByteVector SerializeSelection();
    /// Deserialize selection from provided buffer and apply it to current tab.
    bool DeserializeSelection(const ByteVector& data);

    /// Sent during rendering of tab context menu.
    Signal<void()> onTabContextMenu_;

protected:
    /// Updates cached unique title when id or title changed.
    void UpdateUniqueTitle();

    /// Unique scene id.
    ea::string id_;
    /// Scene title. Should be unique.
    ea::string title_;
    /// Title with id appended to it. Used as unique window name.
    ea::string uniqueTitle_;
    /// TYpe name with id appended to it.
    ea::string uniqueName_;
    /// Returns true if tab is utility (non-content) window.
    bool isUtility_ = false;
    /// Flag indicating that tab is open and renders it's contents.
    bool open_ = true;
    /// Flag indicating tab should reactivate itself next time it is rendered.
    bool activateTab_ = false;
    /// Flag indicating that tab should auto-dock itself into most appropriate place.
    bool autoPlace_ = false;
    /// Flag indicating that tab was open at the start of this frame.
    bool wasOpen_ = false;
    /// Flag indicating that window should not render any padding for window content.
    bool noContentPadding_ = false;
    /// Current window flags.
    ImGuiWindowFlags windowFlags_ = 0;
    /// Global undo stack reference.
    WeakPtr<UndoStack> undo_;
    /// Flag indicating that tab is modified.
    bool modified_ = false;
};

}
