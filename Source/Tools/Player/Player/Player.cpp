//
// Copyright (c) 2018 Rokas Kupstys
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
#define CR_HOST CR_DISABLE
#include <cr/cr.h>
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
#if __linux__
#   include <dlfcn.h>
#endif
#include "Player.h"


#if URHO3D_CSHARP
// Shared library build for execution by managed runtime.
extern "C" URHO3D_EXPORT_API void ParseArgumentsC(int argc, char** argv) { Urho3D::ParseArguments(argc, argv); }
extern "C" URHO3D_EXPORT_API Urho3D::Application* CreateApplication(Urho3D::Context* context) { return new Urho3D::Player(context); }
#else
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
    FileSystem* fs = GetFileSystem();
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = fs->GetProgramDir() + ";" + fs->GetCurrentDir();
    engineParameters_[EP_RESOURCE_PATHS] = "Cache;Resources";

    JSONFile file(context_);
    if (!file.LoadFile("Settings.json"))
        return;

    for (auto& pair : file.GetRoot().GetObject())
        engineParameters_[pair.first_] = pair.second_.GetVariant();
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

    context_->RegisterSubsystem(new SceneManager(context_));

    SharedPtr<JSONFile> projectFile(GetCache()->GetResource<JSONFile>("Project.json"));
    if (projectFile.Null())
    {
        ErrorExit("Project.json missing.");
        return;
    }

    // Load plugins.
    bool failure = false;
    const JSONValue& projectRoot = projectFile->GetRoot();
    const JSONValue& plugins = projectRoot["plugins"];
    for (auto i = 0; i < plugins.Size(); i++)
    {
        if (plugins[i]["private"].GetBool())
            continue;

        String pluginName = plugins[i]["name"].GetString();
        String pluginFileName;
        bool loaded = false;
#if !_WIN32
        // Native plugins on unixes
#if __linux__
        pluginFileName = "lib" + pluginName + ".so";
#elif APPLE
        pluginFileName = "lib" + pluginName + ".dylib";
#endif
        if (GetFileSystem()->Exists(pluginFileName))
            loaded = LoadAssembly(pluginFileName);
        else
        {
            pluginFileName = GetFileSystem()->GetProgramDir() + pluginFileName;
            if (GetFileSystem()->Exists(pluginFileName))
                loaded = LoadAssembly(pluginFileName);
        }
#endif
        // Native plugins on windows or managed plugins on all platforms
        if (!loaded)
        {
            pluginFileName = pluginName + ".dll";
            if (GetFileSystem()->Exists(pluginFileName))
                loaded = LoadAssembly(pluginFileName);
            else
            {
                pluginFileName = GetFileSystem()->GetProgramDir() + pluginFileName;
                if (GetFileSystem()->Exists(pluginFileName))
                    loaded = LoadAssembly(pluginFileName);
            }
        }

        if (!loaded)
        {
            URHO3D_LOGERRORF("Loading of '%s' assembly failed.", pluginName.CString());
            failure = true;
        }
    }

    if (failure)
        ErrorExit("Loading of required plugins failed.");

    for (auto& plugin : plugins_)
        plugin->Start();

    // Load main scene.
    {
        SceneManager* manager = GetSubsystem<SceneManager>();
        Scene* scene = manager->CreateScene();

        SharedPtr<XMLFile> sceneFile(GetCache()->GetResource<XMLFile>(projectRoot["default-scene"].GetString()));
        if (scene->LoadXML(sceneFile->GetRoot()))
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
}

bool Player::LoadAssembly(const String& path)
{
    PluginType type = GetPluginType(context_, path);
    if (type == PLUGIN_NATIVE)
    {
        cr_plugin dummy{};
        dummy.userdata = reinterpret_cast<void*>(context_);
        auto sharedLibrary = cr_so_load(dummy, path.CString());
        if (sharedLibrary != nullptr)
        {
            cr_plugin_main_func pluginMain = cr_so_symbol(sharedLibrary);
            if (pluginMain)
            {
                if (pluginMain(&dummy, CR_LOAD) == 0)
                {
                    plugins_.Push(SharedPtr<PluginApplication>(reinterpret_cast<PluginApplication*>(dummy.userdata)));
                    return true;
                }
            }
        }
    }
#if URHO3D_CSHARP
    else if (type == PLUGIN_MANAGED)
    {
        if (Script* script = GetSubsystem<Script>())
        {
            if (PluginApplication* plugin = script->LoadAssembly(path))
            {
                plugins_.Push(SharedPtr<PluginApplication>(plugin));
                return true;
            }
        }
    }
#endif
    return false;
}

}
