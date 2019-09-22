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
#if _WIN32
#   undef GetObject
#endif
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/SceneManager.h>
#include <Urho3D/Scene/SceneMetadata.h>
#if URHO3D_CSHARP
#   include <Urho3D/Script/Script.h>
#endif
#if URHO3D_SYSTEMUI
#   include <Urho3D/SystemUI/SystemUI.h>
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
#if DESKTOP
    FileSystem* fs = GetFileSystem();
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = fs->GetProgramDir() + ";" + fs->GetCurrentDir();
#endif
    engineParameters_[EP_RESOURCE_PATHS] = "Cache;Resources";

    JSONFile file(context_);
    if (!file.LoadFile(ToString("%s%s", APK, "Settings.json")))
        return;

    for (auto& pair : file.GetRoot().GetObject())
        engineParameters_[pair.first] = pair.second.GetVariant();
}

void Player::Start()
{
#if URHO3D_SYSTEMUI
    ui::GetIO().IniFilename = nullptr;              // Disable of imgui.ini creation,
#endif

    GetCache()->AddResourceRouter(new BakedResourceRouter(context_));

    context_->RegisterSubsystem(new SceneManager(context_));

    SharedPtr<JSONFile> projectFile(GetCache()->GetResource<JSONFile>("Project.json", false));
    if (!projectFile)
    {
        projectFile = new JSONFile(context_);
        if (!projectFile->LoadFile(ToString("%s%s", APK, "Project.json")))
        {
            ErrorExit("Project.json missing.");
            return;
        }
    }

    const JSONValue& projectRoot = projectFile->GetRoot();
#if URHO3D_STATIC
    RegisterPlugins();
#else
    if (!projectRoot.Contains("plugins"))
    {
        ErrorExit("Project.json does not have 'plugins' section.");
        return;
    }

    const JSONValue& plugins = projectRoot["plugins"];
    if (!LoadPlugins(plugins))
        ErrorExit("Loading of required plugins failed.");
#endif

    for (LoadedModule& plugin : plugins_)
        plugin.application_->Start();

    // Load main scene.
    {
        auto* manager = GetSubsystem<SceneManager>();
        Scene* scene = manager->CreateScene();

        if (scene->LoadFile(projectRoot["default-scene"].GetString()))
            manager->SetActiveScene(scene);
        else
            ErrorExit("Invalid scene file.");
    }
}

void Player::Stop()
{
    for (LoadedModule& plugin : plugins_)
        plugin.application_->Stop();

    for (LoadedModule& plugin : plugins_)
        plugin.application_->Unload();

#if URHO3D_CSHARP
    for (LoadedModule& plugin : plugins_)
        Script::GetRuntimeApi()->Dispose(plugin.application_);
#endif

    if (auto* manager = GetSubsystem<SceneManager>())
        manager->UnloadAll();
}

bool Player::LoadPlugins(const JSONValue& plugins)
{
    // Load plugins.
#if URHO3D_PLUGINS
    for (auto i = 0; i < plugins.Size(); i++)
    {
        if (plugins[i]["private"].GetBool())
            continue;

        ea::string pluginName = plugins[i]["name"].GetString();
        ea::string pluginFileName;
        bool loaded = false;
#if !_WIN32
        // Native plugins on unixes
#if __linux__
        pluginFileName = "lib" + pluginName + ".so";
#elif APPLE
        pluginFileName = "lib" + pluginName + ".dylib";
#endif

#if MOBILE
        // On mobile libraries are loaded already so it is ok to not check for existence, TODO: iOS
        loaded = LoadAssembly(pluginFileName, PLUGIN_NATIVE);
#else
        // On desktop we can access file system as usual
        if (GetFileSystem()->Exists(pluginFileName))
            loaded = LoadAssembly(pluginFileName);
        else
        {
            pluginFileName = GetFileSystem()->GetProgramDir() + pluginFileName;
            if (GetFileSystem()->Exists(pluginFileName))
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
            if (GetFileSystem()->Exists(pluginFileName))
                loaded = LoadAssembly(pluginFileName);
#if DESKTOP
            else
            {
                pluginFileName = GetFileSystem()->GetProgramDir() + pluginFileName;
                if (GetFileSystem()->Exists(pluginFileName))
                    loaded = LoadAssembly(pluginFileName);
            }
#endif
        }
#endif

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
            return true;
        }
    }
    return false;
}
#if URHO3D_STATIC
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
    plugin->Load();
    return true;
}
#endif
#endif

BakedResourceRouter::BakedResourceRouter(Context* context)
    : ResourceRouter(context)
{
    SharedPtr<JSONFile> file(GetCache()->GetResource<JSONFile>("CacheInfo.json"));
    if (file)
    {
        const auto& info = file->GetRoot().GetObject();
        for (const auto & it : info)
        {
            const JSONArray& files = it.second["files"].GetArray();
            if (files.size() == 1)
                routes_[it.first] = files[0].GetString();
        }
    }
}

void BakedResourceRouter::Route(ea::string& name, ResourceRequest requestType)
{
    auto it = routes_.find(name);
    if (it != routes_.end())
        name = it->second;
}

}
