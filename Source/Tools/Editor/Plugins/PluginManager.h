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


#if URHO3D_PLUGINS

#include <atomic>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/PluginModule.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/Container/FlagSet.h>
#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/IO/Log.h>

namespace Urho3D
{

enum PluginFlag
{
    PLUGIN_DEFAULT = 0,
    PLUGIN_PRIVATE = 1,
};
URHO3D_FLAGSET(PluginFlag, PluginFlags);

class Plugin : public Object
{
    URHO3D_OBJECT(Plugin, Object);
public:
    ///
    explicit Plugin(Context* context);
    ///
    ~Plugin() override;

    /// Returns type of the plugin.
    ModuleType GetModuleType() const { return module_.GetModuleType(); }
    /// Returns file name of plugin.
    const ea::string& GetName() const { return name_; }
    ///
    PluginFlags GetFlags() const { return flags_; }
    ///
    void SetFlags(PluginFlags flags) { flags_ = flags; }
    ///
    PluginModule& GetModule() { return module_; }
    ///
    bool Load(const ea::string& name);
    ///
    bool Load() { return Load(name_); }
    ///
    void Unload() { unloading_ = true; }
    ///
    bool IsLoaded() const { return module_.GetModuleType() != MODULE_INVALID && !unloading_ && application_.NotNull(); }
    ///
    const ea::string& GetPath() const { return path_; }

protected:
    /// Converts name to a full plugin file path. Returns empty string on error.
    ea::string NameToPath(const ea::string& name) const;
    ///
    ea::string VersionModule(const ea::string& path);
    ///
    bool InternalUnload();

    /// Base plugin file name.
    ea::string name_;
    /// Unversioned plugin module path.
    ea::string path_;
    ///
    PluginModule module_{context_};
    /// Flag indicating that plugin should unload on the end of the frame.
    bool unloading_ = false;
    /// Last modification time.
    unsigned mtime_ = 0;
    /// Current plugin version.
    unsigned version_ = 0;
    ///
    PluginFlags flags_ = PLUGIN_DEFAULT;
    /// Instance to the plugin application. This should be a single owning reference to the plugin. Managed plugins are
    /// an exception as managed object holds reference to native object and must be disposed in order to free this object.
    SharedPtr<PluginApplication> application_;

    friend class PluginManager;
};

class PluginManager : public Object
{
    URHO3D_OBJECT(PluginManager, Object);
public:
    /// Construct.
    explicit PluginManager(Context* context);
    /// Unload all plugins an destruct.
    ~PluginManager() override = default;
    /// Load a plugin and return true if succeeded.
    virtual Plugin* Load(const ea::string& name);
    /// Returns a loaded plugin with specified name.
    Plugin* GetPlugin(const ea::string& name);
    /// Returns a vector containing all loaded plugins.
    const ea::vector<SharedPtr<Plugin>>& GetPlugins() const { return plugins_; }
    /// Tick native plugins.
    void OnEndFrame();
    /// Returns list of sorted plugin names that exist in editor directory.
    const StringVector& GetPluginNames();
    /// Converts relative or absolute plugin path to universal plugin name. Returns empty string on failure.
    static ea::string PathToName(const ea::string& path);

protected:
    /// Entry about dynamic library on the disk. It may or may not be loaded.
    struct DynamicLibraryInfo
    {
        /// Last modification time.
        unsigned mtime_ = 0;
        /// Type of plugin.
        ModuleType pluginType_ = MODULE_INVALID;
    };

    /// Loaded plugins.
    ea::vector<SharedPtr<Plugin>> plugins_;
    /// Plugin update check timer.
    Timer updateCheckTimer_;
    /// Cached plugin information.
    ea::unordered_map<ea::string, DynamicLibraryInfo> pluginInfoCache_;

    friend class Plugin;
};

}
#endif
