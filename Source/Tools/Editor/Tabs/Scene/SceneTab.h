//
// Copyright (c) 2018 Rokas Kupstys
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
#include <Toolbox/Graphics/SceneView.h>
#include <Toolbox/Common/UndoManager.h>
#include "Tabs/BaseResourceTab.h"


namespace Urho3D
{

class SceneSettings;
class SceneEffects;

class SceneTab : public BaseResourceTab, public IHierarchyProvider, public IInspectorProvider
{
    URHO3D_OBJECT(SceneTab, BaseResourceTab);
public:
    /// Construct.
    explicit SceneTab(Context* context);
    /// Destruct.
    ~SceneTab() override;
    /// Render inspector window.
    void RenderInspector(const char* filter) override;
    /// Render scene hierarchy window starting from the root node (scene).
    void RenderHierarchy() override;
    /// Render buttons which customize gizmo behavior.
    void RenderToolbarButtons() override;
    /// Called on every frame when tab is active.
    void OnActiveUpdate() override;
    /// Save project data to xml.
    void OnSaveProject(JSONValue& tab) override;
    /// Load project data from xml.
    void OnLoadProject(const JSONValue& tab) override;
    /// Load scene from xml or json file.
    bool LoadResource(const String& resourcePath) override;
    /// Save scene to a resource file.
    bool SaveResource() override;
    /// Called when tab focused.
    void OnFocused() override;
    /// Add a node to selection.
    void Select(Node* node);
    /// Add multiple nodes to selection.
    void Select(PODVector<Node*> nodes);
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
    /// Removes component if it was selected in inspector, otherwise removes selected scene nodes.
    void RemoveSelection();
    /// Return scene view.
    SceneView* GetSceneView() { return &view_; }
    /// Return scene displayed in the tab viewport.
    Scene* GetScene() { return view_.GetScene(); }
    /// Start scene simulation.
    void Play();
    /// Stop scene simulation.
    void Pause();

protected:
    /// Render scene hierarchy window starting from specified node.
    void RenderNodeTree(Node* node);
    /// Called when node selection changes.
    void OnNodeSelectionChanged();
    /// Creates scene camera and other objects required by editor.
    void CreateObjects();
    /// Render content of the tab window.
    bool RenderWindowContent() override;
    /// Update objects with current tab view rect size.
    IntRect UpdateViewRect() override;
    /// Serialize scene to xml file.
    void SceneStateSave(XMLFile& destination);
    /// Unserialize scene from xml file.
    void SceneStateRestore(XMLFile& source);
    /// Manually updates scene.
    void OnUpdate(VariantMap& args);
    /// Render context menu of a scene node.
    void RenderNodeContextMenu();
    /// Inserts extra editor objects for representing some components.
    void OnComponentAdded(VariantMap& args);
    /// Removes extra editor objects that were used for representing some components.
    void OnComponentRemoved(VariantMap& args);
    /// Add or remove camera preview.
    void UpdateCameraPreview();

    /// Scene renderer.
    SceneView view_;
    /// Gizmo used for manipulating scene elements.
    Gizmo gizmo_;
    /// Current selected component displayed in inspector.
    WeakPtr<Component> selectedComponent_;
    /// State change tracker.
    Undo::Manager undo_;
    /// Flag controlling scene updates in the viewport.
    bool scenePlaying_ = false;
    /// Temporary storage of scene data used in play/pause functionality.
    XMLFile sceneState_;
    /// Temporary storage of scene data used when plugins are being reloaded.
    XMLFile sceneReloadState_;
    /// Flag indicating that mouse is hovering scene viewport.
    bool mouseHoversViewport_ = false;
    /// Nodes whose entries in hierarchy tree should be opened on next frame.
    PODVector<Node*> openHierarchyNodes_;
    /// Node to scroll to on next frame.
    WeakPtr<Node> scrollTo_;
    /// Selected camera preview texture.
    SharedPtr<Texture2D> cameraPreviewtexture_;
    /// Selected camera preview viewport.
    SharedPtr<Viewport> cameraPreviewViewport_;
    /// Time since ESC was last pressed. Used for double-press ESC to exit scene simulation.
    unsigned lastEscPressTime_ = 0;
};

};
