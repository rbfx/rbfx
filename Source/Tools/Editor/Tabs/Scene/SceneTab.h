//
// Copyright (c) 2017-2019 the rbfx project.
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


#include <Urho3D/Scene/SceneManager.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include <Toolbox/SystemUI/Gizmo.h>
#include <Toolbox/Graphics/SceneView.h>
#include "Tabs/BaseResourceTab.h"
#include "Tabs/Scene/SceneClipboard.h"
#include "Tabs/UI/RootUIElement.h"


namespace Urho3D
{

class EditorSceneSettings;
class SceneEffects;

struct SceneState
{
    void Save(Scene* scene, UIElement* root)
    {
        sceneState_.Clear();
        uiState_.Clear();
        startScene_ = scene;
        scene->SaveXML(sceneState_);
        root->GetUI()->SaveLayout(uiState_, root);
        defaultStyle_ = root->GetDefaultStyle();
    }

    void Load(UIElement* root)
    {
        sceneState_.Seek(0);
        startScene_->LoadXML(sceneState_);
        startScene_->GetUI()->Clear();
        root->SetDefaultStyle(defaultStyle_);
        root->LoadXML(uiState_);
        defaultStyle_ = nullptr;
        sceneState_.Clear();
        uiState_.Clear();
        startScene_->GetSubsystem<SceneManager>()->SetActiveScene(startScene_);
    }

    ///
    WeakPtr<Scene> startScene_;
    ///
    VectorBuffer sceneState_;
    ///
    VectorBuffer uiState_;
    ///
    SharedPtr<XMLFile> defaultStyle_;
};

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
    /// Load scene from xml or json file.
    bool LoadResource(const ea::string& resourcePath) override;
    /// Save scene to a resource file.
    bool SaveResource() override;
    ///
    StringHash GetResourceType() override { return XMLFile::GetTypeStatic(); };

    /// Called when tab focused.
    void OnFocused() override;
    /// Add a node to selection.
    void Select(Node* node);
    /// Add a component to selection.
    void Select(Component* component);
    /// Add multiple nodes to selection.
    void Select(ea::vector<Node*> nodes);
    /// Remove a node from selection.
    void Unselect(Node* node);
    /// Remove a component from selection.
    void Unselect(Component* component);
    /// Select if node was not selected or unselect if node was selected.
    void ToggleSelection(Node* node);
    /// Select if component was not selected or unselect if component was selected.
    void ToggleSelection(Component* component);
    /// Unselect all nodes.
    void UnselectAll();
    /// Return true if node is selected.
    bool IsSelected(Node* node) const;
    /// Return true if component is selected.
    bool IsSelected(Component* component) const;
    /// Return list of selected nodes.
    const ea::vector<WeakPtr<Node>>& GetSelection() const;
    /// Removes component if it was selected in inspector, otherwise removes selected scene nodes.
    void RemoveSelection();
    /// Return scene displayed in the tab viewport.
    Scene* GetScene();
    /// Returns editor viewport.
    Viewport* GetViewport() { return viewport_; }
    /// Returns undo state manager.
    Undo::Manager& GetUndo() { return undo_; }
    /// Serialize scene to binary buffer.
    void SaveState(SceneState& destination);
    /// Unserialize scene from binary buffer.
    void RestoreState(SceneState& source);
    /// Returns editor viewport camera.
    Camera* GetCamera();
    /// Closes current tab and unloads it's contents from memory.
    void Close() override;

protected:
    /// Render scene hierarchy window starting from specified node.
    void RenderNodeTree(Node* node);
    /// Called when node selection changes.
    void OnNodeSelectionChanged();
    /// Render content of the tab window.
    bool RenderWindowContent() override;
    /// Called right before ui::Begin() of tab.
    void OnBeforeBegin() override;
    /// Called right after ui::Begin() of tab.
    void OnAfterBegin() override;
    /// Called right before ui::End() of tab
    void OnBeforeEnd() override;
    /// Called right after ui::End() of tab
    void OnAfterEnd() override;
    /// Update objects with current tab view rect size.
    IntRect UpdateViewRect() override;
    /// Manually updates scene.
    void OnUpdate(VariantMap& args);
    /// Render context menu of a scene node.
    void RenderNodeContextMenu();
    /// Inserts extra editor objects for representing some components.
    void OnComponentAdded(VariantMap& args);
    /// Removes extra editor objects that were used for representing some components.
    void OnComponentRemoved(VariantMap& args);
    ///
    void OnTemporaryChanged(VariantMap& args);
    ///
    void AddComponentIcon(Component* component);
    ///
    void RemoveComponentIcon(Component* component);
    /// Add or remove camera preview.
    void UpdateCameras();
    ///
    void CopySelection();
    /// Paste clipboard contents into a parent of first selected node.
    void PasteNextToSelection();
    /// Paste clipboard contents into a selected node.
    void PasteIntoSelection();
    /// Paste components into selection or nodes into parent if first selected node.
    void PasteIntuitive();
    ///
    void ResizeMainViewport(const IntRect& rect);
    ///
    void RenderDebugInfo();

    /// Rectangle dimensions that are rendered by this view.
    IntRect rect_;
    /// Texture to which scene is rendered.
    SharedPtr<Texture2D> texture_;
    /// Viewport which defines rendering area.
    SharedPtr<Viewport> viewport_;
    /// Gizmo used for manipulating scene elements.
    Gizmo gizmo_;
    /// Current selected component displayed in inspector.
    ea::hash_set<WeakPtr<Component>> selectedComponents_;
    /// Flag indicating that mouse is hovering scene viewport.
    bool mouseHoversViewport_ = false;
    /// Nodes whose entries in hierarchy tree should be opened on next frame.
    ea::vector<Node*> openHierarchyNodes_;
    /// Node to scroll to on next frame.
    WeakPtr<Node> scrollTo_;
    /// Selected camera preview texture.
    SharedPtr<Texture2D> cameraPreviewtexture_;
    /// Selected camera preview viewport.
    SharedPtr<Viewport> cameraPreviewViewport_;
    /// Utility for copying and pasting scene nodes.
    SceneClipboard clipboard_;
    /// Original window padding that was overwritten before creating window. This padding will be restored right after window started.
    ImVec2 windowPadding_;
    /// List of node IDs that are saved when scene state is saved. Node selection will be restored using these.
    ea::vector<unsigned> savedNodeSelection_;
    /// List of component IDs that are saved when scene state is saved. Component selection will be restored using these.
    ea::vector<unsigned> savedComponentSelection_;
    ///
    SharedPtr<UI> offScreenUI_;
    /// Root element which contains edited UI.
    SharedPtr<RootUIElement> rootElement_;
    ///
    SharedPtr<XMLFile> defaultStyle_;
};

};
