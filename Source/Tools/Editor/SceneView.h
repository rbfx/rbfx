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
#include <Toolbox/AttributeInspector.h>
#include <Toolbox/Gizmo.h>

namespace Urho3D
{

class SceneView : public Object
{
    URHO3D_OBJECT(SceneView, Object);
public:
    /// Construct.
    explicit SceneView(Context* context);
    /// Destruct.
    ~SceneView() override;
    /// Set screen rectangle where scene is being rendered.
    void SetScreenRect(const IntRect& rect);
    /// Return scene debug camera component.
    Camera* GetCamera() { return camera_->GetComponent<Camera>(); }
    /// Set dummy node which helps to get scene rendered into texture.
    void SetRendererNode(Node* node) { renderer_ = node; }
    /// Render scene window.
    bool RenderWindow();
    /// Load scene from xml or json file.
    void LoadScene(const String& filePath);

    /// Add a node to selection.
    void Select(Node* node);
    /// Select any serializable object.
    void Select(Serializable* serializable);
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
    /// Return selected serializable object. It usually is a component selected for inspection.
    Serializable* GetSelectedSerializable() { return selectedSerializable_; }

protected:
    /// Called when node selection changes.
    void OnNodeSelectionChanged();
    /// Creates scene camera and other objects required by editor.
    void CreateEditorObjects();

public:
    /// Scene title. Should be unique.
    String title_ = "Scene";
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
    /// Scene is active when scene window is focused and mouse hovers that window.
    bool isActive_ = false;
    /// Gizmo used for manipulating scene elements.
    Gizmo gizmo_;
    /// Current window flags.
    ImGuiWindowFlags windowFlags_ = 0;
    /// A selected serializable object. This usually is a component that is inspeced by inspector.
    WeakPtr<Serializable> selectedSerializable_;
};

};
