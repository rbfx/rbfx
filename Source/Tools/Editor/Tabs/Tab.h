//
// Copyright (c) 2008-2017 the Urho3D project.
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
#include <Toolbox/SystemUI/ImGuiDock.h>
#include <Urho3D/Scene/Node.h>


namespace Urho3D
{

class Tab : public Object
{
    URHO3D_OBJECT(Tab, Object);
public:
    /// Construct.
    explicit Tab(Context* context, StringHash id, const String& afterDockName, ui::DockSlot_ position);
    /// Render scene hierarchy window.
    virtual void RenderNodeTree() = 0;
    /// Render inspector window.
    virtual void RenderInspector() = 0;
    /// Render content of tab window.
    virtual bool RenderWindowContent() = 0;
    /// Render toolbar buttons.
    virtual void RenderToolbarButtons() { }
    /// Update window when it is active.
    virtual void OnActiveUpdate() { }
    /// Render scene window.
    virtual bool RenderWindow();
    /// Save project data to xml.
    virtual void SaveProject(XMLElement& tab) { }
    /// Load project data from xml.
    virtual void LoadProject(XMLElement& tab) { }
    /// Load a file from resource path.
    virtual void LoadResource(const String& resourcePath) { }
    /// Save tab contents to a resource file.
    virtual bool SaveResource(const String& resourcePath) { return false; }
    /// Save tab contents to a previously loaded resource file.
    bool SaveResource() { return SaveResource(String::EMPTY); }
    /// Set scene view tab title.
    void SetTitle(const String& title);
    /// Set screen rectangle where scene is being rendered.
    virtual void SetSize(const IntRect& rect) { tabRect_ = rect; }
    /// Get scene view tab title.
    String GetTitle() const { return title_; }
    /// Returns title which uniquely identifies scene tab in imgui.
    String GetUniqueTitle() const { return uniqueTitle_;}
    /// Return true if scene tab is active and focused.
    bool IsActive() const { return isActive_; }
    /// Return true if scene view was rendered on this frame.
    bool IsRendered() const { return isRendered_; }
    /// Return unuque object id.
    StringHash GetID() const { return id_; }
    /// Return rect of tab content area.
    IntRect GetContentRect() const { return tabRect_; }

protected:
    /// Unique scene id.
    StringHash id_;
    /// Scene title. Should be unique.
    String title_;
    /// Title with id appended to it. Used as unique window name.
    String uniqueTitle_;
    /// Scene dock is active and window is focused.
    bool isActive_ = false;
    /// Flag set to true when dock contents were visible. Used for tracking "appearing" effect.
    bool isRendered_ = false;
    /// Rectangle dimensions that are rendered by this view.
    IntRect tabRect_;
    /// Current window flags.
    ImGuiWindowFlags windowFlags_ = 0;
    /// Attribute inspector.
    AttributeInspector inspector_;
    /// Name of sibling dock for initial placement.
    String placeAfter_;
    /// Position where this scene view should be docked initially.
    ui::DockSlot_ placePosition_;
    /// Last known mouse position when it was visible.
    IntVector2 lastMousePosition_;
};

}
