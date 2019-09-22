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

#if URHO3D_PLUGINS

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

Plugin::Plugin(Context* context)
    : Object(context)
{
}

Plugin::~Plugin()
{
}

ea::string Plugin::NameToPath(const ea::string& name) const
{
    FileSystem* fs = GetFileSystem();
    ea::string result;

#if __linux__ || __APPLE__
    result = Format("{}lib{}{}", fs->GetProgramDir(), name, platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;
#endif

#if !_WIN32
    result = Format("{}{}{}", fs->GetProgramDir(), name, ".dll");
    if (fs->FileExists(result))
        return result;
#endif

    result = Format("{}{}{}", fs->GetProgramDir(), name, platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;

    return EMPTY_STRING;
}

ea::string Plugin::VersionModule(const ea::string& path)
{
    auto* fs = GetFileSystem();
    unsigned pdbOffset = 0, pdbSize = 0;
    ea::string dir, name, ext;

    ModuleType type = PluginModule::ReadModuleInformation(context_, path, &pdbOffset, &pdbSize);
    SplitPath(path, dir, name, ext);

    ea::string versionString, shortenedName;

    // Headless utilities do not reuqire reloading. They will load module directly.
    if (GetEngine()->IsHeadless())
    {
        versionString = "";
        shortenedName = name;
    }
    else
    {
        versionString = ea::to_string(version_ + 1);
        shortenedName = name.substr(0, name.length() - versionString.length());
    }

    if (shortenedName.length() < 3)
    {
        URHO3D_LOGERROR("Plugin file name '{}' is too short.", name);
        return EMPTY_STRING;
    }

    ea::string versionedPath = dir + shortenedName + versionString + ext;

    // Non-versioned modules do not need pdb/version patching.
    if (GetEngine()->IsHeadless())
        return versionedPath;

    if (!fs->Copy(path, versionedPath))
    {
        URHO3D_LOGERROR("Copying '{}' to '{}' failed.", path, versionedPath);
        return EMPTY_STRING;
    }

#if _MSC_VER || URHO3D_CSHARP
#if _MSC_VER
    bool hashPdb = true;
#else
    bool hashPdb = type == MODULE_MANAGED;
#endif
    if (hashPdb)
    {
        if (pdbOffset != 0)
        {
            File dll(context_);
            if (dll.Open(versionedPath, FILE_READWRITE))
            {
                VectorBuffer fileData(dll, dll.GetSize());
                char* pdbPointer = (char*)fileData.GetModifiableData() + pdbOffset;

                ea::string pdbPath;
                pdbPath.resize(pdbSize);
                strncpy(&pdbPath.front(), pdbPointer, pdbSize);
                SplitPath(pdbPath, dir, name, ext);

                ea::string versionedPdbPath = dir + shortenedName + versionString + ".pdb";
                assert(versionedPdbPath.length() == pdbPath.length());

                // We have to copy pdbs for native and managed dlls
                if (!fs->Copy(pdbPath, versionedPdbPath))
                {
                    URHO3D_LOGERROR("Copying '{}' to '{}' failed.", pdbPath, versionedPdbPath);
                    return EMPTY_STRING;
                }

                strcpy((char*)fileData.GetModifiableData() + pdbOffset, versionedPdbPath.c_str());
                dll.Seek(0);
                dll.Write(fileData.GetData(), fileData.GetSize());
            }
            else
                return EMPTY_STRING;
        }
    }
#if URHO3D_CSHARP
    if (type == MODULE_MANAGED)
    {
        // Managed runtime will modify file version in specified file.
        Script::GetRuntimeApi()->SetAssemblyVersion(versionedPath, version_ + 1);
    }
#endif
#endif
    return versionedPath;
}

bool Plugin::Load(const ea::string& name)
{
    ea::string path = NameToPath(name);
    ea::string pluginPath = VersionModule(path);

    if (pluginPath.empty())
        return false;

    if (module_.Load(pluginPath))
    {
        application_ = module_.InstantiatePlugin();
        if (application_.NotNull())
        {
            application_->InitializeReloadablePlugin();
            name_ = name;
            path_ = path;
            mtime_ = GetFileSystem()->GetLastModifiedTime(pluginPath);
            version_++;
            unloading_ = false;
            return true;
        }
    }
    return false;
}

bool Plugin::InternalUnload()
{
    if (application_.Null())
        return false;

    ModuleType moduleType = GetModuleType();
    // Disposing object requires managed reference to be the last one alive.
    WeakPtr<PluginApplication> application(application_);
    application_->UninitializeReloadablePlugin();
    application_ = nullptr;
    if (!module_.Unload())
        return false;
#if URHO3D_CSHARP
    if (moduleType == MODULE_MANAGED)
    {
        if (!application.Expired())
            Script::GetRuntimeApi()->Dispose(application);
    }
#endif
    return true;
}

PluginManager::PluginManager(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_ENDFRAMEPRIVATE, [this](StringHash, VariantMap&) { OnEndFrame(); });
    SubscribeToEvent(E_SIMULATIONSTART, [this](StringHash, VariantMap&) {
        for (Plugin* plugin : plugins_)
            plugin->application_->Start();
    });
    SubscribeToEvent(E_SIMULATIONSTOP, [this](StringHash, VariantMap&) {
        for (Plugin* plugin : plugins_)
            plugin->application_->Stop();
    });
}

Plugin* PluginManager::Load(const ea::string& name)
{
    Plugin* plugin = GetPlugin(name);

    if (plugin == nullptr)
        plugin = new Plugin(context_);
    else if (plugin->IsLoaded())
        return plugin;

    if (plugin->Load(name))
    {
        plugin->application_->Load();
        URHO3D_LOGINFO("Loaded plugin '{}' version {}.", name, plugin->version_);
        plugins_.push_back(SharedPtr(plugin));
        return plugin;
    }
    else
        URHO3D_LOGERROR("Failed loading plugin '{}'.", name);
    return nullptr;
}

void PluginManager::OnEndFrame()
{
    // TODO: Timeout probably should be configured, larger projects will have a pretty long linking time.
    const unsigned pluginLinkingTimeout = 10000;
    Timer wait;
    bool eventSent = false;

    bool checkOutOfDatePlugins = updateCheckTimer_.GetMSec(false) >= 1000;
    if (checkOutOfDatePlugins)
        updateCheckTimer_.Reset();

    for (Plugin* plugin : plugins_)
    {
        if (plugin->application_.Null())
            continue;

        bool pluginOutOfDate = false;

        // Plugin reloading is not used in headless executions.
        if (!GetEngine()->IsHeadless())
        {
            // Check for modified plugins once in a while, do not hammer syscalls on every frame.
            if (checkOutOfDatePlugins)
                pluginOutOfDate = plugin->mtime_ < GetFileSystem()->GetLastModifiedTime(plugin->GetPath());
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
            plugin->InternalUnload();
        }

        if (pluginOutOfDate)
        {
            // Plugin change is detected the moment compiler starts linking file. We should wait until linker is done.
            while (PluginModule::ReadModuleInformation(context_, plugin->GetPath()) != plugin->GetModuleType())
            {
                Time::Sleep(0);
                if (wait.GetMSec(false) >= pluginLinkingTimeout)
                {
                    plugin->unloading_ = true;
                    URHO3D_LOGERROR("Plugin module '{}' linking timeout. Plugin will be unloaded.");
                    break;
                }
            }
            // Above loop may fail if user swaps plugin file with something incompatible or linking takes too long. This did not happen yet.
            // Code below should gracefully fail.
        }

        if (!plugin->unloading_ && pluginOutOfDate)
        {
            if (plugin->Load())
                URHO3D_LOGINFO("Reloaded plugin '{}' version {}.", GetFileNameAndExtension(plugin->name_), plugin->version_);
            else
                URHO3D_LOGERROR("Reloading plugin '{}' failed and it was unloaded.", GetFileNameAndExtension(plugin->name_));
        }

        if (!plugin->unloading_)
        {
            if (plugin->application_.NotNull())
            {
                if (pluginOutOfDate)
                    plugin->application_->Load();
            }
            else
                plugin->unloading_ = true;
        }
    }

    if (eventSent)
        SendEvent(E_EDITORUSERCODERELOADEND);
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
#endif
    if (path.ends_with(".dll"))
        return GetFileName(path);

    return EMPTY_STRING;
}

const StringVector& PluginManager::GetPluginNames()
{
    auto* pluginNames = ui::GetUIState<StringVector>();

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
                info.pluginType_ = PluginModule::ReadModuleInformation(context_, fullPath);
            }

            if (info.pluginType_ == MODULE_INVALID)
                continue;

            pluginNames->push_back(baseName);
        }
    }
    return *pluginNames;
}

}

#endif // URHO3D_PLUGINS
