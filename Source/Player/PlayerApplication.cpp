// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "PlayerApplication.h"

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Engine/StateManager.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Plugins/PluginApplication.h>
#include <Urho3D/Plugins/PluginManager.h>
#include <Urho3D/Resource/ResourceCache.h>
#if URHO3D_SYSTEMUI
#   include <Urho3D/SystemUI/SystemUI.h>
#endif
#if URHO3D_RMLUI
#   include <Urho3D/RmlUI/RmlUI.h>
#endif

namespace Urho3D
{

PlayerApplication::PlayerApplication(Context* context)
    : Application(context)
{
}

void PlayerApplication::Setup()
{
    FileSystem* fs = context_->GetSubsystem<FileSystem>();

#if MOBILE
    engineParameters_[EP_RESOURCE_PATHS] = "";
#else
    engineParameters_[EP_RESOURCE_PREFIX_PATHS] = fs->GetProgramDir() + ";" + fs->GetCurrentDir();
#endif
}

void PlayerApplication::Start()
{
    auto engine = GetSubsystem<Engine>();
    if (!engine->IsHeadless())
    {
#if URHO3D_SYSTEMUI
        ui::GetIO().IniFilename = nullptr; // Disable of imgui.ini creation,
#endif

        // TODO(editor): Support resource routing
    }

    const StringVector loadedPlugins = engine->GetParameter(EP_PLUGINS).GetString().split(';');

    auto pluginManager = GetSubsystem<PluginManager>();
    pluginManager->SetPluginsLoaded(loadedPlugins);
    pluginManager->StartApplication();
}

void PlayerApplication::Stop()
{
    auto pluginManager = GetSubsystem<PluginManager>();
    pluginManager->StopApplication();
    pluginManager->Commit();

    auto stateManager = GetSubsystem<StateManager>();
    stateManager->Reset();
}

}
