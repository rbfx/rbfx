//
// Copyright (c) 2017-2019 the rbfx project.
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
#include <Urho3D/Math/StringHash.h>
#include <Urho3D/Core/StringUtils.h>
#include <ImGui/imgui.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Scene/Node.h>


namespace Urho3D
{

class IHierarchyProvider
{
public:
    /// Render hierarchy window.
    virtual void RenderHierarchy() = 0;
};

class IInspectorProvider
{
public:
    /// Render inspector window.
    virtual void RenderInspector(const char* filter) = 0;
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
    /// Update window when it is active.
    virtual void OnActiveUpdate() { }
    /// Render tab content.
    virtual bool RenderWindow();
    /// Called right before ui::Begin() of tab.
    virtual void OnBeforeBegin() { }
    /// Called right after ui::Begin() of tab.
    virtual void OnAfterBegin() { }
    /// Called right before ui::End() of tab
    virtual void OnBeforeEnd() { }
    /// Called right after ui::End() of tab
    virtual void OnAfterEnd() { }
    /// Save ui settings.
    virtual void OnSaveUISettings(ImGuiTextBuffer* buf);
    /// Load ui settings.
    virtual void OnLoadUISettings(const char* name, const char* line);
    /// Load a file from resource path.
    virtual bool LoadResource(const ea::string& resourcePath);
    /// Save tab contents to a resource file.
    virtual bool SaveResource();
    /// Called when tab focused.
    virtual void OnFocused() { }
    /// Set scene view tab title.
    void SetTitle(const ea::string& title);
    /// Get scene view tab title.
    ea::string GetTitle() const { return title_; }
    /// Returns title which uniquely identifies scene tab in imgui.
    ea::string GetUniqueTitle() const { return uniqueTitle_;}
    /// Returns title which uniquely identifies scene tab in imgui.
    ea::string GetUniqueName() const { return uniqueName_;}
    /// Return true if scene tab is active and focused.
    bool IsActive() const { return isActive_; }
    /// Return true if scene view was rendered on this frame.
    bool IsRendered() const { return isRendered_; }
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
    virtual bool IsModified() const { return false; }
    /// Closes current tab and unloads it's contents from memory.
    virtual void Close() { open_ = false; }

protected:
    ///
    virtual IntRect UpdateViewRect();
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
    /// Scene dock is active and window is focused.
    bool isActive_ = false;
    /// Flag set to true when dock contents were visible. Used for tracking "appearing" effect.
    bool isRendered_ = false;
    /// Returns true if tab is utility (non-content) window.
    bool isUtility_ = false;
    /// Current window flags.
    ImGuiWindowFlags windowFlags_ = 0;
    /// Attribute inspector.
    AttributeInspector inspector_;
    /// Last known mouse position when it was visible.
    IntVector2 lastMousePosition_;
    /// Flag indicating that tab is open and renders it's contents.
    bool open_ = true;
    /// Flag indicating tab should reactivate itself next time it is rendered.
    bool activateTab_ = false;
    /// Flag indicating that tab should auto-dock itself into most appropriate place.
    bool autoPlace_ = false;
    ///
    bool wasOpen_ = false;
};

}
