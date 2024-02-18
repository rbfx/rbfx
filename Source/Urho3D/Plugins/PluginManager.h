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
    PluginStack(PluginManager* manager, const StringVector& plugins);
    ~PluginStack() override;

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
        unsigned version_{};
        WeakPtr<PluginApplication> application_;
    };

    void LoadPlugins();
    void UnloadPlugins();
    PluginApplication* FindMainPlugin(const ea::string& mainPlugin) const;

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
    /// Register plugin application class to be visible in all future instances of PluginManager.
    static void RegisterPluginApplication(const ea::string& name, PluginApplicationFactory factory);

    explicit PluginManager(Context* context);
    ~PluginManager() override;
    void SerializeInBlock(Archive& archive) override;

    /// Reload all dynamic modules.
    void Reload();

    /// Start plugin application for all loaded plugins.
    void StartApplication();
    /// Stop plugin application for all loaded plugins.
    void StopApplication();
    /// Return whether the application is started now.
    bool IsStarted() const { return pluginStack_ && pluginStack_->IsStarted(); }

    /// Return whether the plugins are renamed on load.
    bool ArePluginsRenamed() const { return renamePluginBinaries_; }
    /// Set loaded plugins. Order is preserved.
    void SetPluginsLoaded(const StringVector& plugins);
    /// Return whether the plugin is loaded.
    bool IsPluginLoaded(const ea::string& name);
    /// Return loaded plugins.
    const StringVector& GetLoadedPlugins() const { return loadedPlugins_; }
    /// Return revision of loaded plugins.
    unsigned GetRevision() const { return revision_; }
    /// Return whether the load is pending at the end of the frame.
    bool IsReloadPending() const { return stackReloadPending_; }

    /// Manually add new plugin with dynamic reloading.
    bool AddDynamicPlugin(Plugin* plugin);
    /// Manually add plugin that stays loaded forever.
    bool AddStaticPlugin(PluginApplication* pluginApplication);
    /// Find or load dynamic plugin by name.
    Plugin* GetDynamicPlugin(const ea::string& name, bool ignoreUnloaded);
    /// Find or load plugin application by name.
    PluginApplication* GetPluginApplication(const ea::string& name, bool ignoreUnloaded, unsigned* version = nullptr);
    /// Return main plugin. The result is valid after plugin application started.
    PluginApplication* GetMainPlugin() const;
    /// Enumerate dynamic modules available to load.
    StringVector ScanAvailableModules();
    /// Enumerate already loaded dynamic modules and static plugins.
    StringVector EnumerateLoadedModules();

private:
    void DisposeStack();
    void RestoreStack();

    void Update(bool exiting);
    void UpdatePlugin(Plugin* plugin, bool checkOutOfDate);

    void PerformPluginUnload(Plugin* plugin);
    void TryReloadPlugin(Plugin* plugin);
    bool CheckAndRemoveUnloadedPlugin(Plugin* plugin);

    template <class T>
    void ForEachPluginApplication(const T& callback)
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
    const bool renamePluginBinaries_{};
    unsigned reloadIntervalMs_{1000};
    unsigned reloadTimeoutMs_{10000};
    /// @}

    bool startPending_{};
    bool stopPending_{};
    bool stackReloadPending_{};
    StringVector loadedPlugins_;
    unsigned revision_{};
    SharedPtr<PluginStack> pluginStack_;

    SerializedPlugins restoreBuffer_;
    bool wasStarted_{};

    /// Currently loaded modules
    /// @{
    ea::unordered_map<ea::string, SharedPtr<Plugin>> dynamicPlugins_;
    ea::unordered_map<ea::string, SharedPtr<PluginApplication>> staticPlugins_;
    /// @}

    /// Auto-reloading of dynamic plugins
    /// @{
    bool forceReload_{};
    bool enableAutoReload_{};
    Timer reloadTimer_;
    /// @}

    /// Cached info about dynamic library on the disk. It may or may not be loaded.
    struct DynamicLibraryInfo
    {
        /// Last modification time.
        unsigned mtime_{};
        /// Type of plugin.
        ModuleType pluginType_{};
    };
    ea::unordered_map<ea::string, DynamicLibraryInfo> pluginInfoCache_;
};

}
