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

#include "../Precompiled.h"

#include "../Plugins/PluginApplication.h"

#include "../Core/Context.h"
#include "../IO/Archive.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"
#include "../Plugins/PluginManager.h"

namespace Urho3D
{

void PluginApplication::RegisterPluginApplication(const ea::string& name, PluginApplicationFactory factory)
{
    PluginManager::RegisterPluginApplication(name, factory);
}

PluginApplication::PluginApplication(Context* context)
    : Object(context)
{
}

PluginApplication::~PluginApplication()
{
}

void PluginApplication::Dispose()
{
    if (Refs() != 1)
    {
        URHO3D_LOGERROR(
            "Plugin application '{}' has more than one reference remaining. "
            "This may lead to memory leaks or crashes.",
            GetTypeName());
    }

    if (isStarted_)
        StopApplication();
    if (isLoaded_)
        UnloadPlugin();
}

void PluginApplication::LoadPlugin()
{
    if (isLoaded_)
    {
        URHO3D_ASSERT(0, "PluginApplication is already loaded");
        return;
    }

    isLoaded_ = true;
    Load();
}

void PluginApplication::UnloadPlugin()
{
    if (!isLoaded_)
    {
        URHO3D_ASSERT(0, "PluginApplication is not loaded");
        return;
    }

    Unload();
    isLoaded_ = false;

    for (const StringHash type : reflectedTypes_)
        context_->RemoveReflection(type);
    reflectedTypes_.clear();
}

void PluginApplication::StartApplication(bool isMain)
{
    if (isStarted_)
    {
        URHO3D_ASSERT(0, "PluginApplication is already started");
        return;
    }

    isStarted_ = true;
    Start(isMain);
}

void PluginApplication::StopApplication()
{
    if (!isStarted_)
    {
        URHO3D_ASSERT(0, "PluginApplication is not started");
        return;
    }

    Stop();
    isStarted_ = false;
}

void PluginApplication::SuspendApplication(Archive& output, unsigned version)
{
    URHO3D_ASSERT(!output.IsInput());

    if (!isStarted_)
    {
        URHO3D_ASSERT(0, "PluginApplication is not started");
        return;
    }

    isStarted_ = false;

    const auto block = output.OpenUnorderedBlock("Application");
    SerializeValue(output, "Version", version);
    Suspend(output);
}

void PluginApplication::ResumeApplication(Archive* input, unsigned version)
{
    URHO3D_ASSERT(!input || input->IsInput());

    if (isStarted_)
    {
        URHO3D_ASSERT(0, "PluginApplication is already started");
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

MainPluginApplication::MainPluginApplication(Context* context)
    : PluginApplication(context)
{
}

MainPluginApplication::~MainPluginApplication()
{
}

}
