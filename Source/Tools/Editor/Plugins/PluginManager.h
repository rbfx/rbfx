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
#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/IO/Log.h>
#include <Player/Common/PluginUtils.h>
#if !defined(NDEBUG) && defined(URHO3D_LOGGING)
#   define CR_DEBUG 1
#   define CR_ERROR(format, ...) URHO3D_LOGERRORF(format, ##__VA_ARGS__)
#   define CR_LOG(format, ...)   URHO3D_LOGINFOF(format, ##__VA_ARGS__)
#   define CR_TRACE
#endif
#include <cr/cr.h>

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
    explicit Plugin(Context* context);

    /// Returns type of the plugin.
    PluginType GetPluginType() const { return type_; }
    /// Returns file name of plugin.
    ea::string GetName() const { return name_; }
    ///
    PluginFlags GetFlags() const { return flags_; }
    ///
    void SetFlags(PluginFlags flags) { flags_ = flags; }

protected:
    /// Unload plugin.
    bool Unload();

    /// Base plugin file name.
    ea::string name_;
    /// Path to plugin dynamic library file.
    ea::string path_;
    /// Type of plugin (invalid/native/managed).
    PluginType type_ = PLUGIN_INVALID;
    /// Context of native plugin. Not initialized for managed plugins.
    cr_plugin nativeContext_{};
    /// Flag indicating that plugin should unload on the end of the frame.
    bool unloading_ = false;
    /// Last modification time.
    unsigned mtime_;
    ///
    PluginFlags flags_ = PLUGIN_DEFAULT;

    friend class PluginManager;
};

class PluginManager : public Object
{
    URHO3D_OBJECT(PluginManager, Object);
public:
    /// Construct.
    explicit PluginManager(Context* context);
    /// Unload all plugins an destruct.
    ~PluginManager();
    /// Load a plugin and return true if succeeded.
    virtual Plugin* Load(const ea::string& name);
    /// Unload a plugin and return true if succeeded.
    virtual void Unload(Plugin* plugin);
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
    /// Converts name to a full plugin file path. Returns empty string on error.
    ea::string NameToPath(const ea::string& name) const;
    /// Returns path to folder where temporary copies of plugin files are stored.
    ea::string GetTemporaryPluginPath() const;

    struct DynamicLibraryInfo
    {
        /// Last modification time.
        unsigned mtime_ = 0;
        /// Type of plugin.
        PluginType pluginType_ = PLUGIN_INVALID;
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
