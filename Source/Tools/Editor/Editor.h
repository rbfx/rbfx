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


#include <Urho3D/Engine/Application.h>
#include <Toolbox/SystemUI/AttributeInspector.h>
#include "Project.h"

using namespace std::placeholders;

namespace Urho3D
{

class Tab;
class SceneTab;
class AssetConverter;
class SubCommand;

static const unsigned EDITOR_VIEW_LAYER = 1U << 31;

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

    ///
    void ExecuteSubcommand(SubCommand* cmd);

    /// Renders UI elements.
    void OnUpdate(VariantMap& args);
    /// Renders menu bar at the top of the screen.
    void RenderMenuBar();
    /// Create a new tab of specified type.
    template<typename T> T* CreateTab() { return (T*)CreateTab(T::GetTypeStatic()); }
    /// Create a new tab of specified type.
    Tab* CreateTab(StringHash type);
    ///
    Tab* GetTabByName(const ea::string& uniqueName);
    ///
    Tab* GetTabByResource(const ea::string& resourceName);
    /// Returns first tab of specified type.
    Tab* GetTab(StringHash type);
    /// Returns first tab of specified type.
    template<typename T> T* GetTab() { return (T*)GetTab(T::GetTypeStatic()); }

    /// Return active scene tab.
    Tab* GetActiveTab() { return activeTab_; }
    /// Return currently open scene tabs.
    const ea::vector<SharedPtr<Tab>>& GetSceneViews() const { return tabs_; }
    /// Return a map of names and type hashes from specified category.
    StringVector GetObjectsByCategory(const ea::string& category);
    /// Returns a list of open content tabs/docks/windows. This list does not include utility docks/tabs/windows.
    const ea::vector<SharedPtr<Tab>>& GetContentTabs() const { return tabs_; }
    /// Opens project or creates new one.
    void OpenProject(const ea::string& projectPath);
    /// Close current project.
    void CloseProject();
    /// Return path containing data directories of engine.
    const ea::string& GetCoreResourcePrefixPath() const { return coreResourcePrefixPath_; }
    /// Create tabs that are open by default and persist through entire lifetime of editor.
    void CreateDefaultTabs();
    /// Load default tab layout.
    void LoadDefaultLayout();
    /// Returns ID of root dockspace.
    ImGuiID GetDockspaceID() const { return dockspaceId_; }
    ///
    ImFont* GetMonoSpaceFont() const { return monoFont_; }
    ///
    void UpdateWindowTitle(const ea::string& resourcePath=EMPTY_STRING);
    ///
    VariantMap& GetEngineParameters() { return engineParameters_; }

protected:
    /// Process console commands.
    void OnConsoleCommand(VariantMap& args);
    /// Process any global hotkeys.
    void HandleHotkeys();
    /// Renders a project plugins submenu.
    void RenderProjectMenu();
    ///
    void SetupSystemUI();
    ///
    template<typename T> void RegisterSubcommand();

    /// List of active scene tabs.
    ea::vector<SharedPtr<Tab>> tabs_;
    /// Last focused scene tab.
    WeakPtr<Tab> activeTab_;
    /// Prefix path of CoreData and EditorData.
    ea::string coreResourcePrefixPath_;
    /// Currently loaded project.
    SharedPtr<Project> project_;
    /// ID of dockspace root.
    ImGuiID dockspaceId_;
    /// Path to a project that editor should open on the end of the frame.
    ea::string pendingOpenProject_;
    /// Flag indicating that editor should create and load default layout.
    bool loadDefaultLayout_ = false;
    ///
    ImFont* monoFont_ = nullptr;
    ///
    bool exiting_ = false;
    ///
    ea::string defaultProjectPath_;
    ///
    ea::vector<SharedPtr<SubCommand>> subCommands_;
};

}
