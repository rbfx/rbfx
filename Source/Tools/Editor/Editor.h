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
#include "IDPool.h"

using namespace std::placeholders;

namespace Urho3D
{

class Tab;
class SceneTab;

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
    /// Create sample scene. Specify xml or json file with serialized scene contents to load them.
    /// \param project is xml element containing serialized scene information. This is same parameter that would be
    /// passed to SceneTab::LoadProject(scene).
    SceneTab* CreateNewScene(XMLElement project=XMLElement());
    /// Return active scene tab.
    Tab* GetActiveTab() { return activeTab_; }
    /// Return currently open scene tabs.
    const Vector<SharedPtr<Tab>>& GetSceneViews() const { return tabs_; }

protected:
    /// Pool tracking availability of unique IDs used by editor.
    IDPool idPool_;
    /// List of active scene tabs.
    Vector<SharedPtr<Tab>> tabs_;
    /// Last focused scene tab.
    WeakPtr<Tab> activeTab_;
    /// Path to a project file.
    String projectFilePath_;
    /// Flag which opens resource browser window.
    bool resourceBrowserWindowOpen_ = true;
};

}