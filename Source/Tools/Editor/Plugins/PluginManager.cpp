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
extern "C" URHO3D_EXPORT_API void ParseArgumentsC(int argc, char** argv) { ParseArguments(argc, argv); }
extern "C" URHO3D_EXPORT_API Application* CreateEditorApplication(Context* context) { return new Editor(context); }
#endif

Plugin::Plugin(Context* context)
    : Object(context)
{
}

bool Plugin::Unload()
{
#if URHO3D_PLUGINS
    if (type_ == PLUGIN_NATIVE)
    {
        cr_plugin_close(nativeContext_);
        nativeContext_.userdata = nullptr;
        return true;
    }
#if URHO3D_CSHARP
    else if (type_ == PLUGIN_MANAGED)
    {
        Script* script = GetSubsystem<Script>();
        script->UnloadRuntime();        // Destroy plugin AppDomain.
        script->LoadRuntime();          // Create new empty plugin AppDomain. Caller is responsible for plugin reinitialization.
        return true;
    }
#endif
#endif
    return false;
}

PluginManager::PluginManager(Context* context)
    : Object(context)
{
#if URHO3D_PLUGINS
    SubscribeToEvent(E_ENDFRAMEPRIVATE, [this](StringHash, VariantMap&) { OnEndFrame(); });
    SubscribeToEvent(E_SIMULATIONSTART, [this](StringHash, VariantMap&) {
        for (auto& plugin : plugins_)
        {
            if (plugin->nativeContext_.userdata != nullptr && plugin->nativeContext_.userdata != context_)
                reinterpret_cast<PluginApplication*>(plugin->nativeContext_.userdata)->Start();
        }
    });
    SubscribeToEvent(E_SIMULATIONSTOP, [this](StringHash, VariantMap&) {
        for (auto& plugin : plugins_)
        {
            if (plugin->nativeContext_.userdata != nullptr && plugin->nativeContext_.userdata != context_)
                reinterpret_cast<PluginApplication*>(plugin->nativeContext_.userdata)->Stop();
        }
    });
#if URHO3D_CSHARP
    // Initialize AppDomain for managed plugins.
    GetSubsystem<Script>()->LoadRuntime();
#endif
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
        if (cr_plugin_load(plugin->nativeContext_, pluginPath.c_str()))
        {
            plugin->nativeContext_.userdata = context_;

            ea::string pluginTemp = GetTemporaryPluginPath();
            GetFileSystem()->CreateDirsRecursive(pluginTemp);
            cr_set_temporary_path(plugin->nativeContext_, pluginTemp.c_str());

            plugin->name_ = name;
            plugin->path_ = pluginPath;
            plugin->mtime_ = GetFileSystem()->GetLastModifiedTime(pluginPath);
            plugins_.push_back(plugin);
            return plugin.Get();
        }
        else
            URHO3D_LOGWARNINGF("Failed loading native plugin \"%s\".", name.c_str());
    }
#if URHO3D_CSHARP
    else if (plugin->type_ == PLUGIN_MANAGED)
    {
        if (GetSubsystem<Script>()->LoadAssembly(pluginPath))
        {
            plugin->name_ = name;
            plugin->path_ = pluginPath;
            plugin->mtime_ = GetFileSystem()->GetLastModifiedTime(pluginPath);
            plugins_.push_back(plugin);
            return plugin.Get();
        }
    }
#endif
#endif
    return nullptr;
}

void PluginManager::Unload(Plugin* plugin)
{
    if (plugin == nullptr)
        return;

    auto it = plugins_.find(SharedPtr<Plugin>(plugin));
    if (it == plugins_.end())
    {
        URHO3D_LOGERRORF("Plugin %s was never loaded.", plugin->name_.c_str());
        return;
    }

    plugin->unloading_ = true;
}

void PluginManager::OnEndFrame()
{
#if URHO3D_PLUGINS
    // TODO: Timeout probably should be configured, larger projects will have a pretty long linking time.
    const unsigned pluginLinkingTimeout = 10000;
    Timer wait;
    bool eventSent = false;
#if URHO3D_CSHARP
    Script* script = GetSubsystem<Script>();
    // C# plugin auto-reloading.
    ea::vector<Plugin*> reloadingPlugins;
    for (auto it = plugins_.begin(); it != plugins_.end(); it++)
    {
        Plugin* plugin = it->Get();
        if (plugin->type_ == PLUGIN_MANAGED && plugin->mtime_ < GetFileSystem()->GetLastModifiedTime(plugin->path_))
            reloadingPlugins.push_back(plugin);
    }

    if (!reloadingPlugins.empty())
    {
        if (!eventSent)
        {
            SendEvent(E_EDITORUSERCODERELOADSTART);
            eventSent = true;
        }
        script->UnloadRuntime();
        script->LoadRuntime();
        for (auto& plugin : plugins_)
        {
            if (plugin->type_ == PLUGIN_MANAGED)
            {
                if (reloadingPlugins.contains(plugin.Get()))
                {
                    // This plugin was modified and triggered a reload. Good idea before loading it would be waiting for
                    // build to be done (should it still be in progress).
                    while (GetPluginType(context_, plugin->path_) != plugin->type_ && wait.GetMSec(false) < pluginLinkingTimeout)
                        Time::Sleep(0);
                }
                plugin->mtime_ = GetFileSystem()->GetLastModifiedTime(plugin->path_);
                script->LoadAssembly(plugin->path_);
            }
        }
        URHO3D_LOGINFO("Managed plugins were reloaded.");
    }
#endif

    for (auto it = plugins_.begin(); it != plugins_.end();)
    {
        Plugin* plugin = it->Get();

        if (plugin->unloading_)
        {
            if (!eventSent)
            {
                SendEvent(E_EDITORUSERCODERELOADSTART);
                eventSent = true;
            }
            // Actual unload
            plugin->Unload();
#if URHO3D_CSHARP
            if (plugin->type_ == PLUGIN_MANAGED)
            {
                // Now load back all managed plugins except this one.
                for (auto& plug : plugins_)
                {
                    if (plug == plugin || plug->type_ == PLUGIN_NATIVE)
                        continue;
                    script->LoadAssembly(plug->path_);
                }
            }
#endif
            URHO3D_LOGINFOF("Plugin %s was unloaded.", plugin->name_.c_str());
            it = plugins_.erase(it);
        }
        else if (plugin->type_ == PLUGIN_NATIVE && plugin->nativeContext_.userdata)
        {
            bool reloading = cr_plugin_changed(plugin->nativeContext_);
            if (reloading)
            {
                if (!eventSent)
                {
                    SendEvent(E_EDITORUSERCODERELOADSTART);
                    eventSent = true;
                }

                // Plugin change is detected the moment compiler starts linking file. We should wait until linker is done.
                while (GetPluginType(context_, plugin->path_) != plugin->type_ && wait.GetMSec(false) < pluginLinkingTimeout)
                    Time::Sleep(0);
            }

            bool checkUpdatedFile = reloading || updateCheckTimer_.GetMSec(false) >= 1000;
            if (checkUpdatedFile)
                updateCheckTimer_.Reset();

            auto status = cr_plugin_update(plugin->nativeContext_, checkUpdatedFile);
            if (status != 0)
            {
                URHO3D_LOGERRORF("Processing plugin \"%s\" failed and it was unloaded.",
                    GetFileNameAndExtension(plugin->name_).c_str());
                cr_plugin_close(plugin->nativeContext_);
                plugin->nativeContext_.userdata = nullptr;
                it = plugins_.erase(it);
            }
            else if (reloading && plugin->nativeContext_.userdata != nullptr)
            {
                URHO3D_LOGINFOF("Loaded plugin \"%s\" version %d.", GetFileNameAndExtension(plugin->name_).c_str(),
                    plugin->nativeContext_.version);
                it++;
            }
            else
                it++;
        }
        else
            it++;
    }

    if (eventSent)
        SendEvent(E_EDITORUSERCODERELOADEND);
#endif
}

Plugin* PluginManager::GetPlugin(const ea::string& name)
{
    for (auto it = plugins_.begin(); it != plugins_.end(); it++)
    {
        if (it->Get()->name_ == name)
            return it->Get();
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

ea::string PluginManager::GetTemporaryPluginPath() const
{
    return GetFileSystem()->GetTemporaryDir() + ToString("Urho3D-Editor-Plugins-%d/", GetCurrentProcessID());
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
    for (auto& plugin : plugins_)
    {
        if (plugin->type_ == PLUGIN_NATIVE)
            // Native plugins can be unloaded one by one.
            plugin->Unload();
    }

#if URHO3D_CSHARP
    // Managed plugins can not be unloaded one at a time. Entire plugin AppDomain must be dropped.
    GetSubsystem<Script>()->UnloadRuntime();
#endif

    GetFileSystem()->RemoveDir(GetTemporaryPluginPath(), true);
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
