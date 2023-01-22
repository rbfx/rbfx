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

#include "../Project/CloseDialog.h"
#include "../Project/EditorTab.h"
#include "../Project/LaunchManager.h"
#include "../Project/ProjectRequest.h"
#include "../Project/ToolManager.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Plugins/PluginManager.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/SystemUI/DragDropPayload.h>

#include <EASTL/functional.h>
#include <EASTL/set.h>

#include <future>
#include <regex>

struct ImFont;

namespace Urho3D
{

class AssetManager;
class HotkeyManager;
class SettingsManager;
class UndoManager;

/// Result of the graceful project close.
enum class CloseProjectResult
{
    Undefined,
    Closed,
    Canceled
};

/// Helper class to keep and restore state of ResourceCache.
class ResourceCacheGuard
{
public:
    explicit ResourceCacheGuard(Context* context);
    ~ResourceCacheGuard();

    const ea::string& GetCoreData() const { return oldCoreData_; }
    const ea::string& GetEditorData() const { return oldEditorData_; }

private:
    Context* context_{};
    StringVector oldResourceDirs_;
    ea::string oldCoreData_;
    ea::string oldEditorData_;
};

/// File type analysis context.
struct AnalyzeFileContext
{
    Context* context_{};

    AbstractFilePtr binaryFile_;
    SharedPtr<XMLFile> xmlFile_;
    SharedPtr<JSONFile> jsonFile_;

    bool HasXMLRoot(ea::string_view root) const;
    bool HasXMLRoot(std::initializer_list<ea::string_view> roots) const;
};

/// Main class for all Editor logic related to the project folder.
class Project : public Object
{
    URHO3D_OBJECT(Project, Object);

public:
    using AnalyzeFileCallback = ea::function<void(ResourceFileDescriptor& desc, const AnalyzeFileContext& ctx)>;
    using CommandExecutedCallback = ea::function<void(bool success, const ea::string& output)>;
    using FileSavedCallback =
        ea::function<void(const ea::string& fileName, const ea::string& resourceName, bool& needReload)>;

    Signal<void()> OnInitialized;
    Signal<void()> OnShallowSaved;
    Signal<void()> OnRenderProjectMenu;
    Signal<void()> OnRenderProjectToolbar;
    Signal<void(ProjectRequest*)> OnRequest;
    Signal<void(const ea::string& command, const ea::string& args, bool& processed)> OnCommand;

    Project(Context* context, const ea::string& projectPath, const ea::string& settingsJsonPath, bool isReadOnly);
    ~Project() override;
    void SerializeInBlock(Archive& archive) override;

    /// Execute the command line in the context of the project.
    /// Execution will be postponed until the project is initialized.
    void ExecuteCommand(const ea::string& command, bool exitOnCompletion = false);
    /// Execute the command line in another process.
    bool ExecuteRemoteCommand(const ea::string& command, ea::string* output = nullptr);
    /// Execute the command line in another process asynchronously.
    void ExecuteRemoteCommandAsync(const ea::string& command, CommandExecutedCallback callback);

    /// Called right before destructor.
    /// Perform all complicated stuff here because Project is still available for plugins as subsystem.
    void Destroy();

    /// Request graceful close of the project. Called multiple times during close sequence.
    CloseProjectResult CloseGracefully();
    /// Request graceful close of the resource.
    void CloseResourceGracefully(const CloseResourceRequest& request);
    /// Process global request.
    void ProcessRequest(ProjectRequest* request, EditorTab* sender = nullptr);
    /// Add callback for file analysis.
    void AddAnalyzeFileCallback(const AnalyzeFileCallback& callback);
    /// Return file descriptor for specified file.
    ResourceFileDescriptor GetResourceDescriptor(const ea::string& resourceName, const ea::string& fileName = EMPTY_STRING) const;

    /// Update and render main window with tabs.
    void Render();
    /// Update and render toolbar.
    void RenderToolbar();
    /// Update and render project menu.
    void RenderProjectMenu();
    /// Update and render the rest of menu bar.
    void RenderMainMenu();

    /// Save file after delay and ignore reload event.
    void SaveFileDelayed(const ea::string& fileName, const ea::string& resourceName, const SharedByteVector& bytes,
        const FileSavedCallback& onSaved = {});
    void SaveFileDelayed(Resource* resource, const FileSavedCallback& onSaved = {});
    /// Mark files with specified name pattern as internal and ignore them in UI.
    void IgnoreFileNamePattern(const ea::string& pattern);
    /// Return whether the file name is ignored.
    bool IsFileNameIgnored(const ea::string& fileName) const;
    /// Add new tab. Avoid calling it in realtime.
    void AddTab(SharedPtr<EditorTab> tab);
    /// Find first tab of matching type.
    template <class T> T* FindTab() const;
    /// Set whether the global hotkeys are enabled.
    void SetGlobalHotkeysEnabled(bool enabled) { areGlobalHotkeysEnabled_ = enabled; }
    /// Set whether the UI highlight is enabled.
    void SetHighlightEnabled(bool enabled) { isHighlightEnabled_ = enabled; }
    /// Set current launch configuration name.
    void SetLaunchConfigurationName(const ea::string& name) { currentLaunchConfiguration_ = name; }
    /// Return current launch configuration name.
    const ea::string& GetLaunchConfigurationName() const { return currentLaunchConfiguration_; }
    /// Return current launch configuration.
    const LaunchConfiguration* GetLaunchConfiguration() const { return launchManager_->FindConfiguration(currentLaunchConfiguration_); }
    /// Return name of random temporary directory.
    ea::string GetRandomTemporaryPath() const;
    /// Create temporary directory that will be automatically deleted when the handler is destroyed.
    TemporaryDir CreateTemporaryDir();
    /// Mark project itself as having unsaved changes.
    void MarkUnsaved() { hasUnsavedChanges_ = true; }
    /// Return whether the project itself has unsaved changes.
    bool HasUnsavedChanges() const { return hasUnsavedChanges_; }

    /// Commands
    /// @{
    void SaveShallowOnly();
    void SaveProjectOnly();
    void SaveResourcesOnly();
    void Save();
    /// @}

    void ReadIniSettings(const char* entry, const char* line);
    void WriteIniSettings(ImGuiTextBuffer& output);

    /// Return global properties
    /// @{
    const ea::string& GetProjectPath() const { return projectPath_; }
    const ea::string& GetCoreDataPath() const { return coreDataPath_; }
    const ea::string& GetDataPath() const { return dataPath_; }
    const ea::string& GetCachePath() const { return cachePath_; }
    const ea::string& GetPreviewPngPath() const { return previewPngPath_; }
    /// @}

    /// Return singletons
    /// @{
    AssetManager* GetAssetManager() const { return assetManager_; }
    HotkeyManager* GetHotkeyManager() const { return hotkeyManager_; }
    SettingsManager* GetSettingsManager() const { return settingsManager_; }
    UndoManager* GetUndoManager() const { return undoManager_; }
    PluginManager* GetPluginManager() const { return pluginManager_; }
    LaunchManager* GetLaunchManager() const { return launchManager_; }
    ToolManager* GetToolManager() const { return toolManager_; }
    /// @}

    /// Internal
    /// @{
    void SetFocusedTab(EditorTab* tab);
    EditorTab* GetRootFocusedTab() { return focusedRootTab_; }
    /// @}

    /// System UI helpers
    /// @{
    static void SetMonoFont(ImFont* font);
    static ImFont* GetMonoFont();
    /// @}

private:
    struct PendingRequest
    {
        SharedPtr<ProjectRequest> request_;
        WeakPtr<EditorTab> sender_;
    };
    struct PendingFileSave
    {
        ea::string fileName_;
        SharedByteVector bytes_;
        FileSavedCallback onSaved_;
        SharedPtr<Resource> resource_;
        Timer timer_;

        void Clear() { bytes_ = nullptr; resource_ = nullptr; }
        bool IsEmpty() const { return !bytes_ && !resource_; }
    };
    struct PendingRemoteCommand
    {
        CommandExecutedCallback callback_;
        std::future<ea::pair<bool, ea::string>> result_;
    };

    void InitializeHotkeys();
    void EnsureDirectoryInitialized();
    void InitializeDefaultProject();
    void InitializeResourceCache();
    void ResetLayout();
    void ApplyPlugins();
    void SaveGitIgnore();
    void ProcessPendingRequests();
    void ProcessDelayedSaves(bool forceSave = false);
    void ProcessCommand(const ea::string& command, bool exitOnCompletion);
    void ProcessPendingRemoteCommands();
    void RenderAssetsToolbar();

    /// Project properties
    /// @{
    const bool isHeadless_{};
    const bool isReadOnly_{};
    const unsigned saveDelayMs_{3000};

    const ea::string projectPath_;

    const ea::string coreDataPath_;
    const ea::string cachePath_;
    const ea::string tempPath_;

    const ea::string projectJsonPath_;
    const ea::string settingsJsonPath_;
    const ea::string cacheJsonPath_;
    const ea::string uiIniPath_;
    const ea::string gitIgnorePath_;
    const ea::string previewPngPath_;

    ea::string dataPath_;

    const ResourceCacheGuard oldCacheState_;
    /// @}

    SharedPtr<AssetManager> assetManager_;
    SharedPtr<HotkeyManager> hotkeyManager_;
    SharedPtr<SettingsManager> settingsManager_;
    SharedPtr<UndoManager> undoManager_;
    SharedPtr<PluginManager> pluginManager_;
    SharedPtr<LaunchManager> launchManager_;
    SharedPtr<ToolManager> toolManager_;

    bool assetManagerInitialized_{};
    ea::weak_ptr<void> initializationGuard_;
    bool firstInitialization_{};
    bool initialized_{};
    bool hasUnsavedChanges_{};
    ea::vector<SharedPtr<EditorTab>> tabs_;
    ea::map<ea::string, SharedPtr<EditorTab>> sortedTabs_;
    ea::set<ea::string> ignoredFileNames_;
    ea::vector<std::regex> ignoredFileNameRegexes_;
    ea::vector<AnalyzeFileCallback> analyzeFileCallbacks_;

    /// Commands to be executed after initialization.
    ea::vector<ea::pair<ea::string, bool>> pendingCommands_;

    /// Global requests to be processed.
    ea::vector<PendingRequest> pendingRequests_;

    /// File save requests to be processed.
    ea::unordered_map<ea::string, PendingFileSave> delayedFileSaves_;

    /// Ongoing remote commands.
    ea::vector<PendingRemoteCommand> pendingRemoteCommands_;

    /// Close popup handling
    /// @{
    SharedPtr<CloseDialog> closeDialog_;
    CloseProjectResult closeProjectResult_{};
    /// @}

    /// UI state
    /// @{
    bool pendingResetLayout_{};
    ImGuiID dockspaceId_{};
    WeakPtr<EditorTab> focusedTab_;
    WeakPtr<EditorTab> focusedRootTab_;
    bool areGlobalHotkeysEnabled_{true};
    bool isHighlightEnabled_{};
    ea::string currentLaunchConfiguration_;
    /// @}
};

template <class T>
T* Project::FindTab() const
{
    for (EditorTab* tab : tabs_)
    {
        if (auto derivedTab = dynamic_cast<T*>(tab))
            return derivedTab;
    }
    return nullptr;
}

}
