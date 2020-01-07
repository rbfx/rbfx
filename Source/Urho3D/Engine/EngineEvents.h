//
// Copyright (c) 2008-2020 the Urho3D project.
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

/// Engine finished initialization, but Application::Start() was not claled yet.
URHO3D_EVENT(E_ENGINEINITIALIZED, EngineInitialized)
{
}

/// Application started, but first frame was not executed yet.
URHO3D_EVENT(E_APPLICATIONSTARTED, ApplicationStarted)
{
}

/// Plugin::Load() is about to get called.
URHO3D_EVENT(E_PLUGINLOAD, PluginLoad)
{
}

/// Plugin::Unload() is about to get called.
URHO3D_EVENT(E_PLUGINUNLOAD, PluginUnload)
{
}

/// Plugin::Start() is about to get called.
URHO3D_EVENT(E_PLUGINSTART, PluginStart)
{
}

/// Plugin::Stop() is about to get called.
URHO3D_EVENT(E_PLUGINSTOP, PluginStop)
{
}

/// A request for user to manually register static plugins.
URHO3D_EVENT(E_REGISTERSTATICPLUGINS, RegisterStaticPlugins)
{
}

}
