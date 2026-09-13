// Copyright (c) 2008-2022 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"

namespace Urho3D
{

/// A command has been entered on the console.
URHO3D_EVENT(E_CONSOLECOMMAND, ConsoleCommand)
{
    URHO3D_PARAM(P_COMMAND, Command);              // String
    URHO3D_PARAM(P_ID, Id);                        // String
}

/// A command has been entered on the console.
URHO3D_EVENT(E_CONSOLEURICLICK, ConsoleUriClick)
{
    URHO3D_PARAM(P_ADDRESS, Address);              // String
    URHO3D_PARAM(P_PROTOCOL, Protocol);            // String
}

/// Engine finished initialization, but Application::Start() was not called yet.
URHO3D_EVENT(E_ENGINEINITIALIZED, EngineInitialized)
{
}

/// Application started, but first frame was not rendered yet.
URHO3D_EVENT(E_APPLICATIONSTARTED, ApplicationStarted)
{
}

/// Application stopped and no frames will be rendered any more.
URHO3D_EVENT(E_APPLICATIONSTOPPED, ApplicationStopped)
{
}

/// Begin plugin reloading.
URHO3D_EVENT(E_BEGINPLUGINRELOAD, BeginPluginReload)
{
}

/// End plugin reloading.
URHO3D_EVENT(E_ENDPLUGINRELOAD, EndPluginReload)
{
}

}
