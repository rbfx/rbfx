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
    if (!runtime)
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
