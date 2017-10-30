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

using namespace std::placeholders;

namespace Urho3D
{

class SceneView;

class Editor : public Application
{
    URHO3D_OBJECT(Editor, Application);
public:
    /// Construct.
    explicit Editor(Context* context);
    /// Set up editor application.
    void Setup() override;
    /// Initialize editor application.
    void Start() override;
    /// Tear down editor application.
    void Stop() override;

    /// Save editor configuration.
    void SaveProject(const String& filePath);
    /// Load saved editor configuration.
    void LoadProject(const String& filePath);
    /// Renders UI elements.
    void OnUpdate(VariantMap& args);
    /// Renders menu bar at the top of the screen.
    void RenderMenuBar();
    /// Render scene node tree. Should be called with Scene as parameter in order to render entire scene node tree.
    void RenderSceneNodeTree(Node* node);
    /// Create sample scene. Specify xml or json file with serialized scene contents to load them.
    SceneView* CreateNewScene(const String& path = "");
    /// Return true if specified scene tab is focused and mouse hovers it.
    bool IsActive(Scene* scene);
    /// Return scene view based on it's label, or null if no such scene view exists.
    SceneView* GetSceneView(const String& title);

protected:
    /// Flag indicating that dock UI should be initialized to default locations.
    bool initializeDocks_ = true;
    /// List of active scene views
    Vector<SharedPtr<SceneView>> sceneViews_;
    /// Dummy scene required for making scene rendering to textures work.
    SharedPtr<Scene> scene_;
    /// Focused scene view tab.
    WeakPtr<SceneView> activeView_;
    /// Last focused scene view tab.
    WeakPtr<SceneView> lastActiveView_;
    /// Attribute inspector dock.
    AttributeInspectorDockWindow inspector_;
    /// Path to a project file.
    String projectFilePath_;
};

}