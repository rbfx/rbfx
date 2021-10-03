//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "KeyBindings.h"
#include "Project.h"
#include "Pipeline/Commands/SubCommand.h"

using namespace std::placeholders;

namespace Urho3D
{

class Tab;
class SceneTab;
struct InspectArgs;

static const unsigned EDITOR_VIEW_LAYER = 1u << 31u;

class Editor : public Application
{
    URHO3D_OBJECT(Editor, Application);
public:
    /// Construct.
    explicit Editor(Context* context);
    /// Destruct.
    ~Editor() override = default;
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
#if URHO3D_STATIC && URHO3D_PLUGINS
    /// Register static plugin.
    bool RegisterPlugin(PluginApplication* plugin);
#endif
    /// Serialize editor user-specific settings.
    bool Serialize(Archive& archive) override;

    /// Key bindings manager.
    KeyBindings keyBindings_{context_};
    /// Signal is fired when settings tabs are rendered. Various subsystems can register their tabs.
    Signal<void()> settingsTabs_{};
    /// Signal is fired when something wants to inspect a certain object.
    Signal<void(InspectArgs&)> onInspect_;

protected:
    /// Process console commands.
    void OnConsoleCommand(VariantMap& args);
    /// Housekeeping tasks.
    void OnEndFrame();
    /// Handle user closing editor window.
    void OnExitRequested();
    /// Handle user closing editor with a hotkey.
    void OnExitHotkeyPressed();
    /// Renders a project plugins submenu.
    void RenderProjectMenu();
    ///
    void RenderSettingsWindow();
    ///
    void SetupSystemUI();
    ///
    template<typename T> void RegisterSubcommand();
    /// Opens a file dialog for folder selection. Project is opened or created in selected folder.
    void OpenOrCreateProject();
    ///
    void OnConsoleUriClick(VariantMap& args);
    /// Handle selection changes.
    void OnSelectionChanged(StringHash, VariantMap& args);

    /// List of active scene tabs.
    ea::vector<SharedPtr<Tab>> tabs_;
    /// Last focused scene tab.
    WeakPtr<Tab> activeTab_;
    /// Tab containing current user selection.
    WeakPtr<Tab> selectionTab_;
    /// Current selection serialized.
    ByteVector selectionBuffer_;
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
    /// Monospace font.
    ImFont* monoFont_ = nullptr;
    /// Flag indicating editor is exiting.
    bool exiting_ = false;
    /// Flag indicating that settings window is open.
    bool settingsOpen_ = false;
    /// Project path passed on command line.
    ea::string defaultProjectPath_;
    /// Registered subcommands.
    ea::vector<SharedPtr<SubCommand>> subCommands_;
    /// A list of of recently opened projects. First one is alaways last project that was opened.
    StringVector recentProjects_{};
    /// Window position which is saved between sessions.
    IntVector2 windowPos_{0, 0};
    /// Window size which is saved between sessions.
    IntVector2 windowSize_{1920, 1080};
    /// All instances of type-specific inspectors.
    ea::vector<SharedPtr<RefCounted>> inspectors_;
    /// Show imgui metrics window.
    bool showMetricsWindow_ = false;
};

}
