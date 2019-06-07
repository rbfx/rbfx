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
#   define CR_ROLLBACK 0
#   define CR_HOST CR_DISABLE
#   include <cr/cr.h>
#endif
#if _WIN32
#   undef GetObject
#endif
#include <../Common/PluginUtils.h>
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


#if URHO3D_CSHARP
// Shared library build for execution by managed runtime.
extern "C" URHO3D_EXPORT_API void URHO3D_STDCALL ParseArgumentsC(int argc, char** argv) { Urho3D::ParseArguments(argc, argv); }
extern "C" URHO3D_EXPORT_API Urho3D::Application* URHO3D_STDCALL CreateApplication(Urho3D::Context* context) { return new Urho3D::Player(context); }
#elif !defined(URHO3D_STATIC)
// Native executable build for direct execution.
URHO3D_DEFINE_APPLICATION_MAIN(Urho3D::Player);
#endif

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
#if URHO3D_CSHARP
    if (Script* script = GetSubsystem<Script>())    // Graceful failure when managed runtime support is present but not in use.
        script->LoadRuntime();
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
    if (!projectRoot.Contains("plugins"))
    {
        ErrorExit("Project.json does not have 'plugins' section.");
        return;
    }

    const JSONValue& plugins = projectRoot["plugins"];
    if (!LoadPlugins(plugins))
        ErrorExit("Loading of required plugins failed.");

    for (auto& plugin : plugins_)
        plugin->Start();

    // Load main scene.
    {
        SceneManager* manager = GetSubsystem<SceneManager>();
        Scene* scene = manager->CreateScene();

        if (scene->LoadFile(projectRoot["default-scene"].GetString()))
            manager->SetActiveScene(scene);
        else
            ErrorExit("Invalid scene file.");
    }
}

void Player::Stop()
{
    for (auto& plugin : plugins_)
        plugin->Stop();

    for (auto& plugin : plugins_)
        plugin->Unload();

    if (SceneManager* manager = GetSubsystem<SceneManager>())
        manager->UnloadAll();
}

bool Player::LoadPlugins(const JSONValue& plugins)
{
    // Load plugins.
    bool failure = false;
#if URHO3D_PLUGINS || URHO3D_CSHARP
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

#if URHO3D_PLUGINS
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
#endif  // URHO3D_PLUGINS
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
bool Player::LoadAssembly(const ea::string& path, PluginType assumeType)
{
    if (assumeType == PLUGIN_INVALID)
        assumeType = GetPluginType(context_, path);

    if (assumeType == PLUGIN_NATIVE)
    {
        auto sharedLibrary = cr_so_load(path.c_str());
        if (sharedLibrary != nullptr)
        {
            cr_plugin_main_func pluginMain = cr_so_symbol(sharedLibrary);
            if (pluginMain)
            {
                cr_plugin dummy{};
                dummy.userdata = reinterpret_cast<void*>(context_);
                if (pluginMain(&dummy, CR_LOAD) == 0)
                {
                    plugins_.push_back(
                        SharedPtr<PluginApplication>(reinterpret_cast<PluginApplication*>(dummy.userdata)));
                    return true;
                }
            }
        }
    }
#if URHO3D_CSHARP
    else if (assumeType == PLUGIN_MANAGED)
    {
        if (Script* script = GetSubsystem<Script>())
        {
            if (PluginApplication* plugin = script->LoadAssembly(path))
            {
                plugins_.emplace_back(SharedPtr<PluginApplication>(plugin));
                return true;
            }
        }
    }
#endif
    return false;
}
#endif

BakedResourceRouter::BakedResourceRouter(Context* context)
    : ResourceRouter(context)
{
    SharedPtr<JSONFile> file(GetCache()->GetResource<JSONFile>("CacheInfo.json"));
    if (file)
    {
        const auto& info = file->GetRoot().GetObject();
        for (auto it = info.begin(); it != info.end(); it++)
        {
            const JSONArray& files = it->second["files"].GetArray();
            if (files.size() == 1)
                routes_[it->first] = files[0].GetString();
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
