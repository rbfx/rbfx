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

#include "../Precompiled.h"

#include "../Plugins/PluginManager.h"

#include "../Core/CoreEvents.h"
#include "../Core/ProcessUtils.h"
#include "../Engine/Engine.h"
#include "../Engine/EngineDefs.h"
#include "../Engine/EngineEvents.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/BinaryArchive.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Plugins/ModulePlugin.h"
#include "../Plugins/ScriptBundlePlugin.h"

#include <EASTL/bonus/adaptors.h>
#include <EASTL/finally.h>

namespace Urho3D
{

namespace
{

auto& GetRegistry()
{
    static ea::unordered_map<ea::string, PluginApplicationFactory> registry;
    return registry;
}

#if !URHO3D_STATIC
ea::string PathToName(const ea::string& path)
{
#if !_WIN32
    if (path.ends_with(DYN_LIB_SUFFIX))
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
#endif

}

PluginStack::PluginStack(PluginManager* manager, const StringVector& plugins)
    : Object(manager->GetContext())
{
    for (const ea::string& name : plugins)
    {
        unsigned version{};
        if (const WeakPtr<PluginApplication> application{manager->GetPluginApplication(name, false, &version)})
        {
            const PluginInfo info{name, version, application};
            applications_.push_back(info);
            if (application->IsMain())
                mainApplications_.push_back(info);
        }
    }

    LoadPlugins();
}

PluginStack::~PluginStack()
{
    if (isStarted_)
        StopApplication();
    UnloadPlugins();
}

void PluginStack::LoadPlugins()
{
    for (const PluginInfo& info : applications_)
    {
        if (info.application_)
            info.application_->LoadPlugin();
    }
}

void PluginStack::UnloadPlugins()
{
    for (const PluginInfo& info : ea::reverse(applications_))
    {
        if (info.application_)
            info.application_->UnloadPlugin();
    }
}

PluginApplication* PluginStack::FindMainPlugin(const ea::string& mainPlugin) const
{
    if (!mainPlugin.empty())
    {
        for (const PluginInfo& info : mainApplications_)
        {
            if (info.name_ == mainPlugin)
                return info.application_.Get();
        }

        URHO3D_LOGWARNING("Cannot find main plugin '{}'", mainPlugin);
    }

    if (mainApplications_.size() > 1)
        URHO3D_LOGWARNING("Multiple main plugins found, using '{}'", mainApplications_.front().name_);

    if (!mainApplications_.empty())
        return mainApplications_.front().application_;

    return nullptr;
}

void PluginStack::StartApplication(const ea::string& mainPlugin)
{
    if (isStarted_)
    {
        URHO3D_ASSERT(0);
        return;
    }

    mainApplication_ = FindMainPlugin(mainPlugin);

    for (const PluginInfo& info : applications_)
    {
        if (info.application_)
            info.application_->StartApplication(info.application_ == mainApplication_);
    }
    isStarted_ = true;
}

SerializedPlugins PluginStack::SuspendApplication()
{
    SerializedPlugins data;
    if (!isStarted_)
    {
        URHO3D_ASSERT(0);
        return data;
    }

    isStarted_ = false;

    for (const PluginInfo& info : ea::reverse(applications_))
    {
        if (!info.application_)
            continue;

        BinaryOutputArchive archive{context_, data[info.name_]};
        info.application_->SuspendApplication(archive, info.version_);
    }
    return data;
}

void PluginStack::ResumeApplication(const SerializedPlugins& serializedPlugins)
{
    if (isStarted_)
    {
        URHO3D_ASSERT(0);
        return;
    }

    for (const PluginInfo& info : applications_)
    {
        if (!info.application_)
            continue;

        const auto iter = serializedPlugins.find(info.name_);
        if (iter == serializedPlugins.end())
            info.application_->ResumeApplication(nullptr, info.version_);
        else
        {
            const VectorBuffer& pluginData = iter->second;
            MemoryBuffer dataView{pluginData.GetBuffer()};
            BinaryInputArchive archive{context_, dataView};
            info.application_->ResumeApplication(&archive, info.version_);
        }
    }
    isStarted_ = true;
}

void PluginStack::StopApplication()
{
    if (!isStarted_)
    {
        URHO3D_ASSERT(0);
        return;
    }

    isStarted_ = false;

    for (const PluginInfo& info : ea::reverse(applications_))
    {
        if (info.application_)
            info.application_->StopApplication();
    }
}

PluginApplication* PluginStack::GetMainPlugin() const
{
    return mainApplication_;
}

void PluginManager::RegisterPluginApplication(const ea::string& name, PluginApplicationFactory factory)
{
    GetRegistry().emplace(name, factory);
}

PluginManager::PluginManager(Context* context)
    : Object(context)
    , enableAutoReload_(!context_->GetSubsystem<Engine>()->IsHeadless())
    , pluginStack_(MakeShared<PluginStack>(this, loadedPlugins_))
{
    for (const auto& [name, factory] : GetRegistry())
    {
        const auto application = factory(context_);
        application->SetPluginName(name);
        AddStaticPlugin(application);
    }

#if URHO3D_PLUGINS && URHO3D_CSHARP
    auto scriptBundlePlugin = MakeShared<ScriptBundlePlugin>(context_);
    scriptBundlePlugin->SetName("Automatic:Scripts");
    AddDynamicPlugin(scriptBundlePlugin);
#endif

    SubscribeToEvent(E_ENDFRAMEPRIVATE, [this](StringHash, VariantMap&) { Update(false); });
}

PluginManager::~PluginManager()
{
    for (const auto& [name, plugin] : dynamicPlugins_)
        plugin->Unload();

    // Could have called PerformUnload() above,
    // but this way we get a consistent log message informing that module was unloaded.
    Update(true);
}

void PluginManager::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "LoadedPlugins", loadedPlugins_);
    if (archive.IsInput())
        SetPluginsLoaded(loadedPlugins_);
}

void PluginManager::Reload()
{
    forceReload_ = true;
}

void PluginManager::StartApplication()
{
    // If StopApplication was called during this frame, it's okay to start again
    if ((pluginStack_->IsStarted() || startPending_) && !stopPending_)
    {
        // Already started
        URHO3D_ASSERT(0);
        return;
    }

    startPending_ = true;
}

void PluginManager::StopApplication()
{
    // If StartApplication was called during this frame, just cancel it
    if (startPending_)
    {
        startPending_ = false;
        return;
    }

    if (!pluginStack_->IsStarted() || stopPending_)
    {
        // Already stopped
        URHO3D_ASSERT(0);
        return;
    }

    stopPending_ = true;
}

void PluginManager::SetPluginsLoaded(const StringVector& plugins)
{
    loadedPlugins_ = plugins;
    stackReloadPending_ = true;
    revision_ = ea::max(1u, revision_ + 1);
}

bool PluginManager::IsPluginLoaded(const ea::string& name)
{
    PluginApplication* application = GetPluginApplication(name, true);
    return application && application->IsLoaded();
}

bool PluginManager::AddDynamicPlugin(Plugin* plugin)
{
#if URHO3D_PLUGINS && !URHO3D_STATIC
    const ea::string& name = plugin->GetName();
    if (dynamicPlugins_.contains(name) || staticPlugins_.contains(name))
    {
        URHO3D_ASSERTLOG(0, "Plugin name '{}' is already used", name);
        return false;
    }

    if (!plugin->Load())
    {
        URHO3D_LOGERROR("Failed to load plugin '{}'", name);
        return false;
    }

    if (!plugin->GetApplication())
    {
        URHO3D_ASSERTLOG(0, "Unexpected behavior of Plugin::Load");
        return false;
    }

    const ea::string& internalName = plugin->GetApplication()->GetPluginName();
    if (name != internalName)
        URHO3D_LOGWARNING("Plugin file name {} doesn't match plugin class name", name, internalName);

    dynamicPlugins_.emplace(name, SharedPtr<Plugin>(plugin));

    URHO3D_LOGINFO("Loaded plugin '{}' version {}", name, plugin->GetVersion());
    return true;
#else
    return false;
#endif
}

bool PluginManager::AddStaticPlugin(PluginApplication* pluginApplication)
{
    const ea::string name = pluginApplication->GetPluginName();
    if (dynamicPlugins_.contains(name) || staticPlugins_.contains(name))
    {
        URHO3D_ASSERTLOG(0, "Plugin name '{}' is already used", name);
        return false;
    }

    staticPlugins_.emplace(name, SharedPtr<PluginApplication>(pluginApplication));

    URHO3D_LOGINFO("Loaded static plugin '{}'", name);
    return true;
}

Plugin* PluginManager::GetDynamicPlugin(const ea::string& name, bool ignoreUnloaded)
{
#if URHO3D_PLUGINS && !URHO3D_STATIC
    const auto iter = dynamicPlugins_.find(name);
    if (iter != dynamicPlugins_.end())
        return iter->second;
    if (ignoreUnloaded)
        return nullptr;

    auto plugin = MakeShared<ModulePlugin>(context_);
    plugin->SetName(name);
    if (!AddDynamicPlugin(plugin))
        return nullptr;

    return plugin;
#else
    return nullptr;
#endif
}

PluginApplication* PluginManager::GetPluginApplication(const ea::string& name, bool ignoreUnloaded, unsigned* version)
{
    if (version)
        *version = 0;

    const auto iter = staticPlugins_.find(name);
    if (iter != staticPlugins_.end())
        return iter->second;

    if (Plugin* dynamicPlugin = GetDynamicPlugin(name, ignoreUnloaded))
    {
        if (version)
            *version = dynamicPlugin->GetVersion();

        if (PluginApplication* application = dynamicPlugin->GetApplication())
            return application;
    }

    return nullptr;
}

PluginApplication* PluginManager::GetMainPlugin() const
{
    if (pluginStack_)
        return pluginStack_->GetMainPlugin();
    return nullptr;
}

void PluginManager::DisposeStack()
{
    URHO3D_ASSERT(pluginStack_ != nullptr);

    SendEvent(E_BEGINPLUGINRELOAD);

    wasStarted_ = pluginStack_->IsStarted();
    if (wasStarted_)
        restoreBuffer_ = pluginStack_->SuspendApplication();
    pluginStack_ = nullptr;
}

void PluginManager::RestoreStack()
{
    URHO3D_ASSERT(pluginStack_ == nullptr);

    pluginStack_ = MakeShared<PluginStack>(this, loadedPlugins_);
    if (wasStarted_)
        pluginStack_->ResumeApplication(restoreBuffer_);
    restoreBuffer_.clear();

    SendEvent(E_ENDPLUGINRELOAD);
}

void PluginManager::Update(bool exiting)
{
    if (stopPending_)
    {
        pluginStack_->StopApplication();
        URHO3D_LOGINFO("Application is stopped with {} plugins", pluginStack_->GetNumPlugins());

        stopPending_ = false;
    }

    if (stackReloadPending_)
    {
        stackReloadPending_ = false;
        DisposeStack();
    }

    const bool checkOutOfDate = !exiting && enableAutoReload_ && (reloadTimer_.GetMSec(false) >= reloadIntervalMs_);
    if (checkOutOfDate)
        reloadTimer_.Reset();

    for (const auto& [name, plugin] : dynamicPlugins_)
        UpdatePlugin(plugin, checkOutOfDate);
    forceReload_ = false;

    ea::erase_if(dynamicPlugins_, [&](const auto& item) { return CheckAndRemoveUnloadedPlugin(item.second); });

    if (exiting)
        return;

    if (!pluginStack_)
        RestoreStack();

    if (startPending_)
    {
        auto engine = GetSubsystem<Engine>();
        pluginStack_->StartApplication(engine->GetParameter(EP_MAIN_PLUGIN).GetString());
        URHO3D_LOGINFO("Application is started with {} plugins", pluginStack_->GetNumPlugins());

        startPending_ = false;
    }
}

void PluginManager::UpdatePlugin(Plugin* plugin, bool checkOutOfDate)
{
    if (!plugin->GetApplication())
        return;

    const bool wasApplicationLoaded = plugin->GetApplication()->IsLoaded();
    const bool pluginOutOfDate = forceReload_ || (checkOutOfDate && plugin->IsOutOfDate());

    if (plugin->IsUnloading() || pluginOutOfDate)
        PerformPluginUnload(plugin);

    if (pluginOutOfDate)
        TryReloadPlugin(plugin);
}

void PluginManager::PerformPluginUnload(Plugin* plugin)
{
    if (pluginStack_)
        DisposeStack();

    plugin->PerformUnload();
}

void PluginManager::TryReloadPlugin(Plugin* plugin)
{
    auto guard = ea::make_finally([&]{ plugin->Unload(); });

    if (!plugin->WaitForCompleteFile(reloadTimeoutMs_))
    {
        URHO3D_LOGERROR("Reloading plugin '{}' timed out and it was unloaded",
            GetFileNameAndExtension(plugin->GetName()));
        return;
    }

    if (!plugin->Load())
    {
        URHO3D_LOGERROR("Reloading plugin '{}' failed and it was unloaded",
            GetFileNameAndExtension(plugin->GetName()));
        return;
    }

    if (!plugin->GetApplication())
    {
        URHO3D_ASSERTLOG(0, "Unexpected behavior of Plugin::Load");
        return;
    }

    // Keep plugin loaded if everything is good
    guard.dismiss();

    URHO3D_LOGINFO("Reloaded plugin '{}' version {}",
        GetFileNameAndExtension(plugin->GetName()), plugin->GetVersion());
}

bool PluginManager::CheckAndRemoveUnloadedPlugin(Plugin* plugin)
{
    if (!plugin->IsUnloading())
        return false;

    URHO3D_LOGINFO("Unloaded plugin '{}'", GetFileNameAndExtension(plugin->GetName()));
    return true;
}

StringVector PluginManager::ScanAvailableModules()
{
    StringVector result;

#if !URHO3D_STATIC
    auto fs = context_->GetSubsystem<FileSystem>();

    StringVector files;
    fs->ScanDir(files, fs->GetProgramDir(), "*.*", SCAN_FILES, false);

    // Remove deleted plugin files.
    for (const ea::string& key : pluginInfoCache_.keys())
    {
        if (!files.contains(key))
            pluginInfoCache_.erase(key);
    }

    for (const ea::string& file : files)
    {
        // Native plugins will rename main file and append version after base name.
        const ea::string baseName = PathToName(file);
        if (baseName.empty() || IsDigit(static_cast<unsigned int>(baseName.back())))
            continue;

        DynamicLibraryInfo& info = pluginInfoCache_[file];

        const ea::string fullPath = fs->GetProgramDir() + file;
        const unsigned currentModificationTime = fs->GetLastModifiedTime(fullPath);

        if (info.mtime_ != currentModificationTime)
        {
            // Parse file only if it is outdated or was not parsed already.
            info.mtime_ = currentModificationTime;
            info.pluginType_ = DynamicModule::ReadModuleInformation(context_, fullPath);
        }

        if (info.pluginType_ == MODULE_INVALID)
            continue;

        result.push_back(baseName);
    }
#endif

    return result;
}

StringVector PluginManager::EnumerateLoadedModules()
{
    StringVector result;
    ForEachPluginApplication([&](PluginApplication* application, const ea::string& name, unsigned version)
    {
        result.push_back(name);
    });
    return result;
}

}
