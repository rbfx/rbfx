//
// Copyright (c) 2008-2022 the Urho3D project.
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
