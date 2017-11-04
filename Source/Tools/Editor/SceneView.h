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


#include <Urho3D/Urho3DAll.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include <Toolbox/SystemUI/Gizmo.h>
#include <Toolbox/SystemUI/ImGuiDock.h>
#include "IDPool.h"


namespace Urho3D
{

class SceneSettings;
class SceneEffects;

class SceneView : public Object
{
    URHO3D_OBJECT(SceneView, Object);
public:
    /// Construct.
    explicit SceneView(Context* context, StringHash id, const String& afterDockName, ui::DockSlot_ position);
    /// Destruct.
    ~SceneView() override;
    /// Set screen rectangle where scene is being rendered.
    void SetScreenRect(const IntRect& rect);
    /// Return scene debug camera component.
    Camera* GetCamera() { return camera_->GetComponent<Camera>(); }
    /// Set dummy node which helps to get scene rendered into texture.
    Node* GetRendererNode();
    /// Render scene window.
    bool RenderWindow();
    /// Render inspector window.
    void RenderInspector();
    /// Render scene hierarchy window.
    void RenderSceneNodeTree(Node* node=nullptr);
    /// Load scene from xml or json file.
    void LoadScene(const String& filePath);
    /// Save scene to a resource file.
    bool SaveScene(const String& filePath = "");

    /// Add a node to selection.
    void Select(Node* node);
    /// Remove a node from selection.
    void Unselect(Node* node);
    /// Select if node was not selected or unselect if node was selected.
    void ToggleSelection(Node* node);
    /// Unselect all nodes.
    void UnselectAll();
    /// Return true if node is selected by gizmo.
    bool IsSelected(Node* node) const;
    /// Return list of selected nodes.
    const Vector<WeakPtr<Node>>& GetSelection() const;
    /// Render buttons which customize gizmo behavior.
    void RenderGizmoButtons();
    /// Save project data to xml.
    void SaveProject(XMLElement scene) const;
    /// Load project data from xml.
    void LoadProject(XMLElement scene);
    /// Set scene view tab title.
    void SetTitle(const String& title);
    /// Get scene view tab title.
    String GetTitle() const { return title_; }
    /// Returns title which uniquely identifies scene tab in imgui.
    String GetUniqueTitle() const { return uniqueTitle_;}
    /// Return true if scene tab is active and focused.
    bool IsActive() const { return isActive_; }
    /// Return scene rendered in this tab.
    Scene* GetScene() const { return scene_; }
    /// Return inuque object id.
    StringHash GetID() const { return id_; }
    /// Clearing cached paths forces choosing a file name next time scene is saved.
    void ClearCachedPaths();
    /// Return scene viewport instance.
    Viewport* GetViewport() const { return viewport_; }

protected:
    /// Called when node selection changes.
    void OnNodeSelectionChanged();
    /// Creates scene camera and other objects required by editor.
    void CreateEditorObjects();

    /// Unique scene id.
    StringHash id_;
    /// Scene title. Should be unique.
    String title_ = "Scene";
    /// Title with id appended to it. Used as unique window name.
    String uniqueTitle_;
    /// Last resource path scene was loaded from or saved to.
    String path_;
    /// Scene which is being edited.
    SharedPtr<Scene> scene_;
    /// Debug camera node.
    SharedPtr<Node> camera_;
    /// Texture into which scene is rendered.
    SharedPtr<Texture2D> view_;
    /// Viewport which renders into texture.
    SharedPtr<Viewport> viewport_;
    /// Node in a main scene which has material with a texture this scene is being rendered to.
    SharedPtr<Node> renderer_;
    /// Current screen rectangle at which scene texture is being rendered.
    IntRect screenRect_;
    /// Scene dock is active and window is focused.
    bool isActive_ = false;
    /// Gizmo used for manipulating scene elements.
    Gizmo gizmo_;
    /// Current window flags.
    ImGuiWindowFlags windowFlags_ = 0;
    /// Attribute inspector.
    AttributeInspector inspector_;
    /// Current selected component displayed in inspector.
    WeakPtr<Component> selectedComponent_;
    /// Name of sibling dock for initial placement.
    String placeAfter_;
    /// Position where this scene view should be docked initially.
    ui::DockSlot_ placePosition_;
    /// Last known mouse position when it was visible.
    IntVector2 lastMousePosition_;
    /// Flag set to true when dock contents were visible. Used for tracking "appearing" effect.
    bool wasRendered_ = false;
    /// Flag which controls visibility of scene settings window.
    bool settingsOpen_ = false;
    /// Serializable which handles scene settings.
    SharedPtr<SceneSettings> settings_;
    /// Serializable which handles scene postprocess effect settings.
    SharedPtr<SceneEffects> effectSettings_;
};

};
