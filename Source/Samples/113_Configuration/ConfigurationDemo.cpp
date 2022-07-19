//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "ConfigurationDemo.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/ConfigManager.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/SystemUI/Console.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <Urho3D/DebugNew.h>

SampleConfigurationFile::SampleConfigurationFile(Context* context)
    : BaseClassName(context)
{
}

void SampleConfigurationFile::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "checkbox", checkbox_);
}

ConfigurationDemo::ConfigurationDemo(Context* context)
    : BaseClassName(context)
{
    if (!context_->IsReflected<SampleConfigurationFile>())
        context_->RegisterFactory<SampleConfigurationFile>();
}

void ConfigurationDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    ui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    ui::GetIO().BackendFlags |= ImGuiBackendFlags_HasMouseCursors;

    // Finally subscribe to the update event. Note that by subscribing events at this point we have already missed some
    // events like the ScreenMode event sent by the Graphics subsystem when opening the application window. To catch
    // those as well we could subscribe in the constructor instead.
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    SetMouseMode(MM_FREE);
    SetMouseVisible(true);

    // Pass console commands to file system.
    GetSubsystem<FileSystem>()->SetExecuteConsoleCommands(true);
    GetSubsystem<Console>()->RefreshInterpreters();

    configurationFile_ = context_->GetSubsystem<ConfigManager>()->Get<SampleConfigurationFile>();
}

void ConfigurationDemo::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ConfigurationDemo, RenderUi));
}

void ConfigurationDemo::RenderUi(StringHash eventType, VariantMap& eventData)
{
    ui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_FirstUseEver);
    ui::SetNextWindowPos(ImVec2(200, 300), ImGuiCond_FirstUseEver);

    if (ui::Begin("Configuration", 0, ImGuiWindowFlags_NoSavedSettings))
    {
        if (ui::Button("Load"))
        {
            configurationFile_->Load();
        }
        if (ui::Button("Save"))
        {
            configurationFile_->Save();
        }
        ui::Checkbox("Checkbox", &configurationFile_->checkbox_);
    }
    ui::End();
}

