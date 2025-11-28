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

#include "../Container/ConstString.h"
#include "../IO/FileWatcher.h"
#include "../IO/VectorBuffer.h"
#include "../Plugins/DynamicModule.h"
#include "../Plugins/Plugin.h"

#include <EASTL/functional.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class PluginManager;

using SerializedPlugins = ea::unordered_map<ea::string, VectorBuffer>;

/// Stack of loaded plugins that are automatically unloaded on destruction.
class URHO3D_API PluginStack : public Object
{
    URHO3D_OBJECT(PluginStack, Object);

public:
    PluginStack(PluginManager* manager, const StringVector& plugins, const ea::string& binaryDirectory,
        const ea::string& temporaryDirectory, unsigned version);
    ~PluginStack() override;

    /// Return whether suspend/resume is supported.
    bool IsSuspendSupported() const;
    /// Start application for all plugins in the stack.
    void StartApplication(const ea::string& mainPlugin);
    /// Suspend all plugins in the stack and stop application.
    SerializedPlugins SuspendApplication();
    /// Resume all plugins in the stack and start application.
    void ResumeApplication(const SerializedPlugins& serializedPlugins);
    /// Stop plugin application for all loaded plugins.
    void StopApplication();

    /// Return whether the application is started now.
    bool IsStarted() const { return isStarted_; }
    /// Return number of loaded plugins.
    unsigned GetNumPlugins() const { return applications_.size(); }

    /// Return main plugin. The result is valid after StartApplication.
    PluginApplication* GetMainPlugin() const;

private:
    struct PluginInfo
    {
        ea::string name_;
        WeakPtr<PluginApplication> application_;
    };

    void CopyBinariesToTemporaryDirectory();

    void LoadPlugins();
    void UnloadPlugins();
    PluginApplication* FindMainPlugin(const ea::string& mainPlugin) const;

private:
    const ea::string binaryDirectory_;
    const ea::string temporaryDirectory_;
    const unsigned version_;

    ea::vector<PluginInfo> applications_;
    ea::vector<PluginInfo> mainApplications_;
    WeakPtr<PluginApplication> mainApplication_;
    bool isStarted_{};
};

/// Manages engine plugins.
/// Note that module being loaded and plugin being loaded are two different things.
class URHO3D_API PluginManager : public Object
{
    URHO3D_OBJECT(PluginManager, Object);

public:
    /// Quit application callback.
    using QuitApplicationCallback = ea::function<void()>;
    /// Register plugin application class to be visible in all future instances of PluginManager.
    static void RegisterPluginApplication(const ea::string& name, PluginApplicationFactory factory);

    explicit PluginManager(Context* context);
    ~PluginManager() override;
    void SerializeInBlock(Archive& archive) override;

    /// Reload all dynamic modules.
    void Reload();
    /// Commit updates to the list of loaded plugins and to application status.
    /// This may be unsafe to call inside of the frame. Called automatically between frames.
    void Commit();

    /// Start plugin application for all loaded plugins.
    void StartApplication();
    /// Stop plugin application for all loaded plugins.
    void StopApplication();
    /// Quit application on user request.
    /// Engine is shut down by default. External tooling like Editor may override this behavior.
    void QuitApplication();
    /// Set callback for QuitApplication().
    void SetQuitApplicationCallback(const QuitApplicationCallback& callback) { quitApplication_ = callback; }
    /// Return whether the application is started now.
    bool IsStarted() const { return pluginStack_ && pluginStack_->IsStarted(); }

    /// Return original binary directory.
    const ea::string& GetOriginalBinaryDirectory() const { return binaryDirectory_; }
    /// Return temporary binary directory, or main binary directory if temporary copies are disabled.
    const ea::string& GetTemporaryBinaryDirectory() const { return temporaryDirectory_; }
    /// Set loaded plugins. Order is preserved.
    void SetPluginsLoaded(const StringVector& plugins);
    /// Return whether the plugin is loaded.
    bool IsPluginLoaded(const ea::string& name);
    /// Return loaded plugins.
    const StringVector& GetLoadedPlugins() const { return loadedPlugins_; }
    /// Return revision of loaded plugins.
    unsigned GetPluginListRevision() const { return listRevision_; }
    /// Return whether the load is pending at the end of the frame.
    bool IsReloadPending() const { return reloadPending_; }
    /// Return whether loaded plugins are out of date.
    bool AreLoadedPluginsOutOfDate() const { return pluginsOutOfDate_; }

    /// Manually add new plugin with dynamic reloading.
    bool AddDynamicPlugin(Plugin* plugin);
    /// Manually add plugin that stays loaded forever.
    bool AddStaticPlugin(PluginApplication* pluginApplication);
    /// Find or load dynamic plugin by name.
    Plugin* GetDynamicPlugin(const ea::string& name, bool ignoreUnloaded);
    /// Find or load plugin application by name.
    PluginApplication* GetPluginApplication(const ea::string& name, bool ignoreUnloaded);
    /// Return main plugin. The result is valid after plugin application started.
    PluginApplication* GetMainPlugin() const;
    /// Enumerate dynamic modules available to load.
    StringVector ScanAvailableModules();
    /// Enumerate already loaded dynamic modules and static plugins.
    StringVector EnumerateLoadedModules();

    /// Return whether reload is blocked for external reasons.
    bool IsReloadBlocked(ea::string* reason = nullptr) const;

private:
    void DisposeStack();
    void RestoreStack();

    void Update(bool exiting);
    void CheckOutOfDatePlugins();

    void PerformPluginUnload(Plugin* plugin);
    bool CheckAndRemoveUnloadedPlugin(Plugin* plugin);

    template <class T> void ForEachPluginApplication(const T& callback)
    {
        for (const auto& [name, plugin] : dynamicPlugins_)
        {
            if (PluginApplication* application = plugin->GetApplication())
                callback(application, name, plugin->GetVersion());
        }

        for (const auto& [name, application] : staticPlugins_)
            callback(application, name, 0);
    }

    /// Parameters
    /// @{
    const bool enableAutoReload_{};
    const ea::string binaryDirectory_;
    ea::string temporaryDirectoryBase_;
    unsigned reloadIntervalMs_{1000};
    unsigned reloadTimeoutMs_{10000};
    /// @}

    bool startPending_{};
    bool stopPending_{};
    bool reloadPending_{};

    StringVector loadedPlugins_;
    unsigned listRevision_{};
    ea::string temporaryDirectory_;
    SharedPtr<PluginStack> pluginStack_;
    QuitApplicationCallback quitApplication_;

    bool pluginsOutOfDate_{};

    SerializedPlugins restoreBuffer_;
    bool wasStarted_{};

    /// Currently loaded modules
    /// @{
    ea::unordered_map<ea::string, SharedPtr<Plugin>> dynamicPlugins_;
    ea::unordered_map<ea::string, SharedPtr<PluginApplication>> staticPlugins_;
    /// @}

    /// Auto-reloading of dynamic plugins
    /// @{
    Timer reloadTimer_;
    /// @}

    /// Cached info about dynamic library on the disk. It may or may not be loaded.
    struct DynamicLibraryInfo
    {
        /// Last modification time.
        unsigned lastModificationTime_{};
        /// Type of plugin.
        ModuleType pluginType_{};
    };
    ea::unordered_map<ea::string, DynamicLibraryInfo> pluginInfoCache_;
};

} // namespace Urho3D
