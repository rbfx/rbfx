// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Plugins/ScriptBundlePlugin.h"

#include "../Plugins/PluginManager.h"
#include "../Resource/ResourceEvents.h"
#include "../Script/Script.h"

namespace Urho3D
{

ScriptBundlePlugin::ScriptBundlePlugin(Context* context)
    : Plugin(context)
{
    SubscribeToEvent(E_FILECHANGED, [this](VariantMap& args)
    {
        using namespace FileChanged;
        const ea::string& name = args[P_RESOURCENAME].GetString();

        OnFileChanged(name);
    });
}

bool ScriptBundlePlugin::Load()
{
    ScriptRuntimeApi* runtime = Script::GetRuntimeApi();
    if (!runtime)
        return false;

    application_ = runtime->CompileResourceScriptPlugin();
    if (!application_)
        return false;

    application_->SetPluginName(name_);

    unloading_ = false;
    outOfDate_ = false;
    ++version_;
    return true;
}

bool ScriptBundlePlugin::PerformUnload()
{
    ScriptRuntimeApi* runtime = Script::GetRuntimeApi();
    if (!runtime || !application_)
        return false;

    application_->Dispose();
    runtime->Dispose(application_.Detach());
    return true;
}

void ScriptBundlePlugin::OnFileChanged(const ea::string& name)
{
    outOfDate_ |= name.ends_with(".cs");
}

}
