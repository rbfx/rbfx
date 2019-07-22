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

#if URHO3D_PLUGINS

#define CR_ROLLBACK 0
#define CR_HOST CR_DISABLE

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Script/Script.h>
#include <Toolbox/SystemUI/Widgets.h>
#include "EditorEvents.h"
#include "Editor.h"
#include "Plugins/PluginManager.h"
#include "PluginManager.h"


namespace Urho3D
{

URHO3D_EVENT(E_ENDFRAMEPRIVATE, EndFramePrivate)
{
}

#if __linux__
static const char* platformDynamicLibrarySuffix = ".so";
#elif _WIN32
static const char* platformDynamicLibrarySuffix = ".dll";
#elif __APPLE__
static const char* platformDynamicLibrarySuffix = ".dylib";
#else
#   error Unsupported platform.
#endif

#if URHO3D_CSHARP && URHO3D_PLUGINS
extern "C" URHO3D_EXPORT_API void URHO3D_STDCALL ParseArgumentsC(int argc, char** argv) { ParseArguments(argc, argv); }
extern "C" URHO3D_EXPORT_API Application* URHO3D_STDCALL CreateEditorApplication(Context* context) { return new Editor(context); }
#endif

Plugin::Plugin(Context* context)
    : Object(context)
{
    nativeContext_.userdata = context;
}

Plugin::~Plugin()
{
#if URHO3D_PLUGINS
    if (type_ == PLUGIN_NATIVE)
    {
        cr_plugin_close(nativeContext_);
        nativeContext_.userdata = nullptr;
        application_ = nullptr;
    }
#if URHO3D_CSHARP
    else if (type_ == PLUGIN_MANAGED)
    {
        // Disposing object requires managed reference to be the last one alive.
        WeakPtr<PluginApplication> application(application_);
        application_ = nullptr;
        if (!application.Expired())
            Script::GetRuntimeApi()->Dispose(application);
    }
#endif
#endif
}

PluginManager::PluginManager(Context* context)
    : Object(context)
{
#if URHO3D_PLUGINS
    SubscribeToEvent(E_ENDFRAMEPRIVATE, [this](StringHash, VariantMap&) { OnEndFrame(); });
    SubscribeToEvent(E_SIMULATIONSTART, [this](StringHash, VariantMap&) {
        for (Plugin* plugin : plugins_)
            plugin->application_->Start();
    });
    SubscribeToEvent(E_SIMULATIONSTOP, [this](StringHash, VariantMap&) {
        for (Plugin* plugin : plugins_)
            plugin->application_->Stop();
    });
#endif
}

Plugin* PluginManager::Load(const ea::string& name)
{
#if URHO3D_PLUGINS
    if (Plugin* loaded = GetPlugin(name))
        return loaded;

    ea::string pluginPath = NameToPath(name);
    if (pluginPath.empty())
        return nullptr;

    SharedPtr<Plugin> plugin(new Plugin(context_));
    plugin->type_ = GetPluginType(context_, pluginPath);

    if (plugin->type_ == PLUGIN_NATIVE)
    {
        if (cr_plugin_load(plugin->nativeContext_, pluginPath.c_str()) && cr_plugin_update(plugin->nativeContext_) == 0)    // Triggers CR_LOAD
        {
            plugin->name_ = name;
            plugin->path_ = pluginPath;
            plugin->mtime_ = GetFileSystem()->GetLastModifiedTime(pluginPath);
            plugin->version_ = plugin->nativeContext_.version;
            plugin->application_ = reinterpret_cast<PluginApplication*>(plugin->nativeContext_.userdata);
            plugins_.push_back(plugin);
        }
        else
            plugin = nullptr;
    }
#if URHO3D_CSHARP
    else if (plugin->type_ == PLUGIN_MANAGED)
    {
        if (PluginApplication* app = Script::GetRuntimeApi()->LoadAssembly(pluginPath, plugin->version_))
        {
            plugin->name_ = name;
            plugin->path_ = pluginPath;
            plugin->mtime_ = GetFileSystem()->GetLastModifiedTime(pluginPath);
            plugins_.push_back(plugin);
            plugin->application_ = app;
        }
        else
            plugin = nullptr;
    }
#endif
#endif

    if (plugin.NotNull())
    {
        plugin->application_->Load();
        URHO3D_LOGINFO("Loaded plugin '{}' version {}.", name, plugin->version_);
        return plugin.Get();
    }
    else
    {
        URHO3D_LOGERROR("Failed loading plugin '{}'.", name);
        return nullptr;
    }
}

void PluginManager::Unload(Plugin* plugin)
{
    if (plugin == nullptr)
        return;

    plugin->unloading_ = true;
}

void PluginManager::OnEndFrame()
{
#if URHO3D_PLUGINS
    // TODO: Timeout probably should be configured, larger projects will have a pretty long linking time.
    const unsigned pluginLinkingTimeout = 10000;
    Timer wait;
    bool eventSent = false;

    bool checkOutOfDatePlugins = updateCheckTimer_.GetMSec(false) >= 1000;
    if (checkOutOfDatePlugins)
        updateCheckTimer_.Reset();

    for (auto it = plugins_.begin(); it != plugins_.end();)
    {
        Plugin* plugin = it->Get();
        bool pluginOutOfDate = false;
        unsigned pluginVersion = plugin->version_;

        if (plugin->application_.NotNull())
        {
            // Check for modified plugins once in a while, do not hammer syscalls on every frame.
            if (checkOutOfDatePlugins)
                pluginOutOfDate = plugin->mtime_ < GetFileSystem()->GetLastModifiedTime(plugin->path_);
        }

        if (plugin->unloading_ || pluginOutOfDate)
        {
            if (!eventSent)
            {
                SendEvent(E_EDITORUSERCODERELOADSTART);
                eventSent = true;
            }

            plugin->application_->Unload();

            int allowedPluginRefs = 1;
#if URHO3D_CSHARP
            if (plugin->application_->HasScriptObject())
                allowedPluginRefs = 2;
#endif
            if (plugin->application_->Refs() != allowedPluginRefs)
            {
                URHO3D_LOGERROR("Plugin application '{}' has more than one reference remaining. "
                                 "This will lead to memory leaks or crashes.",
                                 plugin->application_->GetTypeName());
            }
        }

        if (pluginOutOfDate)
        {
            // Plugin change is detected the moment compiler starts linking file. We should wait until linker is done.
            while (GetPluginType(context_, plugin->path_) != plugin->type_ && wait.GetMSec(false) < pluginLinkingTimeout)
                Time::Sleep(0);

            // Above loop may fail if user swaps plugin file with something incompatible or linking takes too long. This
            // did not happen yet. Code below should gracefully fail.
        }

        if (!plugin->unloading_ && pluginOutOfDate)
        {
            if (plugin->type_ == PLUGIN_NATIVE)
            {
                int status = cr_plugin_update(plugin->nativeContext_, pluginOutOfDate);
                if (status == 0)
                {
                    plugin->application_ = reinterpret_cast<PluginApplication*>(plugin->nativeContext_.userdata);                   // Should free old version of the plugin
                    plugin->version_ = plugin->nativeContext_.version;
                }
                else
                {
                    cr_plugin_close(plugin->nativeContext_);
                    plugin->unloading_ = true;
                }
            }
#if URHO3D_CSHARP
            else if (plugin->type_ == PLUGIN_MANAGED)
            {
                pluginVersion += 1;
                plugin->application_ = Script::GetRuntimeApi()->LoadAssembly(plugin->path_, pluginVersion);         // Should free old version of the plugin
                if (plugin->application_.NotNull())
                    plugin->version_ = pluginVersion;
                else
                    plugin->unloading_ = true;
            }
#endif
            if (!plugin->unloading_)
            {
                URHO3D_LOGINFO("Reloaded plugin '{}' version {}.", GetFileNameAndExtension(plugin->name_), plugin->version_);
                plugin->mtime_ = GetFileSystem()->GetLastModifiedTime(plugin->path_);
            }
            else
                URHO3D_LOGERROR("Reloading plugin '{}' failed and it was unloaded.", GetFileNameAndExtension(plugin->name_));
        }

        if (plugin->unloading_ || plugin->application_.Null())
            it = plugins_.erase(it);
        else
        {
            if (pluginOutOfDate)
                plugin->application_->Load();
            ++it;
        }
    }

    if (eventSent)
        SendEvent(E_EDITORUSERCODERELOADEND);
#endif
}

Plugin* PluginManager::GetPlugin(const ea::string& name)
{
    for (Plugin* plugin : plugins_)
    {
        if (plugin->name_ == name)
            return plugin;
    }
    return nullptr;
}

ea::string PluginManager::NameToPath(const ea::string& name) const
{
    FileSystem* fs = GetFileSystem();
    ea::string result;

#if __linux__ || __APPLE__
    result = ToString("%slib%s%s", fs->GetProgramDir().c_str(), name.c_str(), platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;
#endif

#if !_WIN32
    result = ToString("%s%s%s", fs->GetProgramDir().c_str(), name.c_str(), ".dll");
    if (fs->FileExists(result))
        return result;
#endif

    result = ToString("%s%s%s", fs->GetProgramDir().c_str(), name.c_str(), platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;

    return EMPTY_STRING;
}

ea::string PluginManager::PathToName(const ea::string& path)
{
#if !_WIN32
    if (path.ends_with(platformDynamicLibrarySuffix))
    {
        ea::string name = GetFileName(path);
#if __linux__ || __APPLE__
        if (name.starts_with("lib"))
            name = name.substr(3);
#endif
        return name;
    }
    else
#endif
    if (path.ends_with(".dll"))
        return GetFileName(path);

    return EMPTY_STRING;
}

PluginManager::~PluginManager()
{
}

const StringVector& PluginManager::GetPluginNames()
{
#if URHO3D_PLUGINS
    StringVector* pluginNames = ui::GetUIState<StringVector>();

    if (pluginNames->empty())
    {
        FileSystem* fs = GetFileSystem();

        StringVector files;
        ea::unordered_map<ea::string, ea::string> nameToPath;
        fs->ScanDir(files, fs->GetProgramDir(), "*.*", SCAN_FILES, false);

        // Remove deleted plugin files.
        for (const ea::string& key : pluginInfoCache_.keys())
        {
            if (!files.contains(key))
                pluginInfoCache_.erase(key);
        }

        // Remove definitely not plugins.
        for (const ea::string& file : files)
        {
            ea::string baseName = PluginManager::PathToName(file);
            // Native plugins will rename main file and append version after base name.
            if (baseName.empty() || IsDigit(static_cast<unsigned int>(baseName.back())))
                continue;

            ea::string fullPath = fs->GetProgramDir() + file;
            DynamicLibraryInfo& info = pluginInfoCache_[file];
            unsigned currentModificationTime = fs->GetLastModifiedTime(fullPath);
            if (info.mtime_ != currentModificationTime)
            {
                // Parse file only if it is outdated or was not parsed already.
                info.mtime_ = currentModificationTime;
                info.pluginType_ = GetPluginType(context_, fullPath);
            }

            if (info.pluginType_ == PLUGIN_INVALID)
                continue;

            pluginNames->push_back(baseName);
        }
    }
#endif
    return *pluginNames;
}

}

#endif
