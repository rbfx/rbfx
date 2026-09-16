// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Plugins/ScriptBundlePluginInstance.h"

#include "Urho3D/Plugins/PluginManager.h"
#include "Urho3D/Resource/ResourceEvents.h"
#include "Urho3D/Script/Script.h"

namespace Urho3D
{

ScriptBundlePluginInstance::ScriptBundlePluginInstance(Context* context)
    : PluginInstance(context)
{
    SubscribeToEvent(E_FILECHANGED, [this](VariantMap& args)
    {
        using namespace FileChanged;
        const ea::string& name = args[P_RESOURCENAME].GetString();

        OnFileChanged(name);
    });
}

bool ScriptBundlePluginInstance::Load()
{
    ScriptRuntimeApi* runtime = Script::GetRuntimeApi();
    if (!runtime)
        return false;

    plugin_ = runtime->CompileResourceScriptPlugin();
    if (!plugin_)
        return false;

    plugin_->SetPluginName(name_);

    unloading_ = false;
    outOfDate_ = false;
    ++version_;
    return true;
}

bool ScriptBundlePluginInstance::PerformUnload()
{
    ScriptRuntimeApi* runtime = Script::GetRuntimeApi();
    if (!runtime || !plugin_)
        return false;

    plugin_->Dispose();
    runtime->Dispose(plugin_.Detach());
    return true;
}

void ScriptBundlePluginInstance::OnFileChanged(const ea::string& name)
{
    outOfDate_ |= name.ends_with(".cs");
}

}
