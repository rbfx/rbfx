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
#if _WIN32
#   undef GetObject
#endif
#include <EASTL/sort.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Scene/SceneManager.h>
#if URHO3D_CSHARP
#   include <Urho3D/Script/Script.h>
#endif
#if URHO3D_SYSTEMUI
#   include <Urho3D/SystemUI/SystemUI.h>
#endif
#if URHO3D_RMLUI
#   include <Urho3D/RmlUI/RmlUI.h>
#endif
#include "Player.h"

namespace Urho3D
{

Player::Player(Context* context)
    : Application(context)
{
}

void Player::Setup()
{
    FileSystem* fs = context_->GetSubsystem<FileSystem>();

#if MOBILE
    engineParameters_[EP_RESOURCE_PATHS] = "";
#else
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = fs->GetProgramDir() + ";" + fs->GetCurrentDir();
#endif

#if DESKTOP && URHO3D_DEBUG
    // Developer builds. Try loading from filesystem first.
    const ea::string settingsFilePath = "Cache/Settings.json";
    if (context_->GetSubsystem<FileSystem>()->Exists(settingsFilePath))
    {
        JSONFile jsonFile(context_);
        if (jsonFile.LoadFile(settingsFilePath))
        {
            JSONInputArchive archive(&jsonFile);
            if (settings_.Serialize(archive))
            {
                for (const auto& pair : settings_.engineParameters_)
                    engineParameters_[pair.first] = pair.second;
                engineParameters_[EP_RESOURCE_PATHS] = "Cache;Resources";
                // Unpacked application is executing. Do not attempt to load paks.
                URHO3D_LOGINFO("This is a developer build executing unpacked application.");
                return;
            }
        }
    }
#endif

    const ea::string defaultPak("Resources-default.pak");

    // Enumerate pak files
    StringVector pakFiles;
#if ANDROID
    const ea::string scanDir = ea::string(APK);
#else
    const ea::string scanDir = fs->GetCurrentDir();
#endif
    fs->ScanDir(pakFiles, scanDir, "*.pak", SCAN_FILES, false);

    // Sort paks. If something funny happens at least we will have a deterministic behavior.
    ea::quick_sort(pakFiles.begin(), pakFiles.end());

    // Default pak file goes to the front of the list. It is available on all platforms, but we may desire that platform-specific pak
    // would override settings defined in main pak.
    auto it = pakFiles.find(defaultPak);
    if (it != pakFiles.end())
    {
        pakFiles.erase(it);
        pakFiles.push_front(defaultPak);
    }

    // Find pak file for current platform
    for (const ea::string& pakFile : pakFiles)
    {
        SharedPtr<PackageFile> package(new PackageFile(context_));
        if (!package->Open(APK + pakFile))
        {
            URHO3D_LOGERROR("Failed to open {}", pakFile);
            continue;
        }

        if (!package->Exists("Settings.json"))
            // Likely a custom pak file from user. User is responsible for loading it.
            continue;

        SharedPtr<File> file(new File(context_));
        if (!file->Open(package, "Settings.json"))
        {
            URHO3D_LOGERROR("Unable to open Settings.json in {}", pakFile);
            continue;
        }

        JSONFile jsonFile(context_);
        if (!jsonFile.BeginLoad(*file))
        {
            URHO3D_LOGERROR("Unable to load Settings.json in {}", pakFile);
            continue;
        }

        JSONInputArchive archive(&jsonFile);
        if (!settings_.Serialize(archive))
        {
            URHO3D_LOGERROR("Unable to deserialize Settings.json in {}", pakFile);
            continue;
        }

        // Empty platforms list means pak is meant for all platforms.
        if (settings_.platforms_.empty() || settings_.platforms_.contains(GetPlatform()))
        {
            // Paks are manually added here in order to avoid modifying EP_RESOURCE_PACKAGES value. User may specify this configuration
            // parameter to load any custom paks if desired. Engine will add them later. Besides we already had to to open and parse package
            // in order to find Settings.json. By adding paks now we avoid engine doing all the loading same file twice.
            context_->GetSubsystem<ResourceCache>()->AddPackageFile(package);
            for (const auto& pair : settings_.engineParameters_)
                engineParameters_[pair.first] = pair.second;
        }
    }
}

void Player::Start()
{
#if URHO3D_SYSTEMUI
    ui::GetIO().IniFilename = nullptr;              // Disable of imgui.ini creation,
#endif

    // Add resource router that maps cooked resources to their original resource names.
    auto* router = new CacheRouter(context_);
    for (PackageFile* package : context_->GetSubsystem<ResourceCache>()->GetPackageFiles())
    {
        if (package->Exists("CacheInfo.json"))
            router->AddPackage(package);
    }
    context_->GetSubsystem<ResourceCache>()->AddResourceRouter(router);

    context_->RegisterSubsystem(new SceneManager(context_));

#if URHO3D_RMLUI
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<RmlUI>();
    ea::vector<ea::string> fonts;
    cache->Scan(fonts, "Fonts/", "*.ttf", SCAN_FILES, true);
    cache->Scan(fonts, "Fonts/", "*.otf", SCAN_FILES, true);
    for (const ea::string& font : fonts)
        ui->LoadFont(Format("Fonts/{}", font));
#endif

#if URHO3D_STATIC
    // Static builds require user to manually register plugins by subclassing Player class.
    SendEvent(E_REGISTERSTATICPLUGINS);
#else
    // Shared builds load plugin .so/.dll specified in config file.
    if (!LoadPlugins(settings_.plugins_))
        ErrorExit("Loading of required plugins failed.");
#endif
#if URHO3D_CSHARP && URHO3D_PLUGINS
    // Internal plugin which handles complication of *.cs source code in resource path.
    RegisterPlugin(Script::GetRuntimeApi()->CompileResourceScriptPlugin());
#endif

    for (LoadedModule& plugin : plugins_)
    {
        plugin.application_->SendEvent(E_PLUGINSTART);
        plugin.application_->Start();
    }

    // Load main scene.
    {
        auto* manager = GetSubsystem<SceneManager>();
        Scene* scene = manager->CreateScene();

        if (scene->LoadFile(settings_.defaultScene_))
            manager->SetActiveScene(scene);
        else
            ErrorExit("Invalid scene file.");
    }
}

void Player::Stop()
{
    for (LoadedModule& plugin : plugins_)
    {
        plugin.application_->SendEvent(E_PLUGINSTOP);
        plugin.application_->Stop();
    }

    if (auto* manager = GetSubsystem<SceneManager>())
        manager->UnloadAll();

    for (LoadedModule& plugin : plugins_)
    {
        plugin.application_->SendEvent(E_PLUGINUNLOAD);
        plugin.application_->Unload();
    }

#if URHO3D_CSHARP
    for (LoadedModule& plugin : plugins_)
        Script::GetRuntimeApi()->DereferenceAndDispose(plugin.application_.Detach());
#endif
}

bool Player::LoadPlugins(const StringVector& plugins)
{
    // Load plugins.
#if URHO3D_PLUGINS
    for (const ea::string& pluginName : plugins)
    {
        ea::string pluginFileName;
        bool loaded = false;
#if !_WIN32
        // Native plugins on unixes
#if LINUX
        pluginFileName = "lib" + pluginName + ".so";
#elif APPLE
        pluginFileName = "lib" + pluginName + ".dylib";
#endif

#if MOBILE
        // On mobile libraries are loaded already so it is ok to not check for existence, TODO: iOS
        loaded = LoadAssembly(pluginFileName);
#else
        // On desktop we can access file system as usual
        if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
            loaded = LoadAssembly(pluginFileName);
        else
        {
            pluginFileName = context_->GetSubsystem<FileSystem>()->GetProgramDir() + pluginFileName;
            if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
                loaded = LoadAssembly(pluginFileName);
        }
#endif  // MOBILE
#endif  // !_WIN32

#if _WIN32 || URHO3D_CSHARP
        // Native plugins on windows or managed plugins on all platforms
        if (!loaded)
        {
            pluginFileName = pluginName + ".dll";
#if ANDROID
            pluginFileName = ea::string(APK) + "assets/.net/" + pluginFileName;
#endif
            if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
                loaded = LoadAssembly(pluginFileName);
#if DESKTOP
            else
            {
                pluginFileName = context_->GetSubsystem<FileSystem>()->GetProgramDir() + pluginFileName;
                if (context_->GetSubsystem<FileSystem>()->Exists(pluginFileName))
                    loaded = LoadAssembly(pluginFileName);
            }
#endif  // DESKTOP
        }
#endif  // URHO3D_PLUGINS

        if (!loaded)
        {
            URHO3D_LOGERRORF("Loading of '%s' assembly failed.", pluginName.c_str());
            return false;
        }
    }
#endif  // URHO3D_PLUGINS
    return true;
}
#if URHO3D_PLUGINS
bool Player::LoadAssembly(const ea::string& path)
{
    LoadedModule moduleInfo;

    moduleInfo.module_ = new PluginModule(context_);
    if (moduleInfo.module_->Load(path))
    {
        moduleInfo.application_ = moduleInfo.module_->InstantiatePlugin();
        if (moduleInfo.application_.NotNull())
        {
            plugins_.emplace_back(moduleInfo);
            moduleInfo.application_->SendEvent(E_PLUGINLOAD);
            moduleInfo.application_->Load();
            return true;
        }
    }
    return false;
}

bool Player::RegisterPlugin(PluginApplication* plugin)
{
    if (plugin == nullptr)
    {
        URHO3D_LOGERROR("PluginApplication may not be null.");
        return false;
    }
    LoadedModule moduleInfo;
    moduleInfo.application_ = plugin;
    plugins_.emplace_back(moduleInfo);
    plugin->SendEvent(E_PLUGINLOAD);
    plugin->Load();
    return true;
}
#endif

}
