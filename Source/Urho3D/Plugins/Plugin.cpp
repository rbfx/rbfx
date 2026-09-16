// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Plugins/Plugin.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Plugins/PluginManager.h"

namespace Urho3D
{

void Plugin::RegisterPlugin(const ea::string& name, PluginFactory factory)
{
    PluginManager::RegisterPlugin(name, factory);
}

Plugin::Plugin(Context* context)
    : Object(context)
{
}

Plugin::~Plugin()
{
}

void Plugin::Dispose()
{
    if (Refs() != 1)
    {
        URHO3D_LOGERROR(
            "Plugin '{}' has more than one reference remaining. "
            "This may lead to memory leaks or crashes.",
            GetTypeName());
    }

    if (isStarted_)
        StopApplication();
    if (isLoaded_)
        UnloadPlugin();
}

void Plugin::LoadPlugin()
{
    if (isLoaded_)
    {
        URHO3D_ASSERT(0, "Plugin is already loaded");
        return;
    }

    isLoaded_ = true;
    Load();
}

void Plugin::UnloadPlugin()
{
    if (!isLoaded_)
    {
        URHO3D_ASSERT(0, "Plugin is not loaded");
        return;
    }

    Unload();
    isLoaded_ = false;

    for (const StringHash type : reflectedTypes_)
        context_->RemoveReflection(type);
    reflectedTypes_.clear();
}

void Plugin::StartApplication(bool isMain)
{
    if (isStarted_)
    {
        URHO3D_ASSERT(0, "Plugin is already started");
        return;
    }

    isStarted_ = true;
    Start(isMain);
}

void Plugin::StopApplication()
{
    if (!isStarted_)
    {
        URHO3D_ASSERT(0, "Plugin is not started");
        return;
    }

    Stop();
    isStarted_ = false;
}

void Plugin::SuspendApplication(Archive& output, unsigned version)
{
    URHO3D_ASSERT(!output.IsInput());

    if (!isStarted_)
    {
        URHO3D_ASSERT(0, "Plugin is not started");
        return;
    }

    isStarted_ = false;

    const auto block = output.OpenUnorderedBlock("Application");
    SerializeValue(output, "Version", version);
    Suspend(output);
}

void Plugin::ResumeApplication(Archive* input, unsigned version)
{
    URHO3D_ASSERT(!input || input->IsInput());

    if (isStarted_)
    {
        URHO3D_ASSERT(0, "Plugin is already started");
        return;
    }

    isStarted_ = true;

    if (!input)
        Resume(nullptr, true);
    else
    {
        const auto block = input->OpenUnorderedBlock("Application");
        unsigned oldVersion{};
        SerializeValue(*input, "Version", oldVersion);
        Resume(input, oldVersion != version);
    }
}

ExecutablePlugin::ExecutablePlugin(Context* context)
    : Plugin(context)
{
}

ExecutablePlugin::~ExecutablePlugin()
{
}

}
