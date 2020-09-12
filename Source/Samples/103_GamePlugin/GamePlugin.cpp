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

#include <Urho3D/Scene/LogicComponent.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Scene/Node.h>
#include "FPSCameraController.h"
#include "RotateObject.h"
#include "GamePlugin.h"


URHO3D_DEFINE_PLUGIN_MAIN(Urho3D::GamePlugin);


namespace Urho3D
{

GamePlugin::GamePlugin(Context* context)
    : PluginApplication(context)
{
}

void GamePlugin::Load()
{
    // Register custom components/subsystems/events when plugin is loaded.
    RotateObject::RegisterObject(context_, this);
    FPSCameraController::RegisterObject(context_, this);
}

void GamePlugin::Unload()
{
    // Finalize plugin, ensure that no objects provided by the plugin are alive. Some of that work is automated by
    // parent class. Objects that had factories registered through PluginApplication::RegisterFactory<> have their
    // attributes automatically unregistered, factories/subsystems removed.
}

void GamePlugin::Start()
{
    // Set up any game state here. Configure input. Create objects. Add UI. Game application assumes control of the
    // input.
    context_->GetSubsystem<Input>()->SetMouseVisible(false);
    context_->GetSubsystem<Input>()->SetMouseMode(MM_WRAP);
}

void GamePlugin::Stop()
{
    // Tear down any game state here. Unregister events. Remove objects. Editor takes back the control.
}

}
