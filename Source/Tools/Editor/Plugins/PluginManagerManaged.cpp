//
// Copyright (c) 2018 Rokas Kupstys
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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Script/ScriptSubsystem.h>
#include "EditorEventsPrivate.h"
#include "PluginManagerManaged.h"


namespace Urho3D
{

PluginManagerManaged::PluginManagerManaged(Context* context)
    : PluginManager(context)
{
#if URHO3D_PLUGINS_CSHARP
    // SubscribeToEvent(E_ENDFRAME, std::bind(&PluginManagerManaged::OnEndFrame, this));

    ScriptSubsystem::RuntimeSettings settings{};
    settings.domainName_ = "Editor";
    settings.jitOptions_ = {
        "--debugger-agent=transport=dt_socket,address=127.0.0.1:53630,server=y,suspend=n",
        "--optimize=float32"
    };
    GetScripts()->HostManagedRuntime(settings);
#endif
}

bool PluginManagerManaged::LoadPlugin(const String& path)
{
#if URHO3D_PLUGINS_CSHARP
    if (auto* assembly = GetScripts()->LoadAssembly(path, nullptr))
    {
        auto name = GetFileName(path);
        auto descBase = ToString("%s.%s:", name.CString(), name.CString());
        auto object = GetScripts()->CallMethod(assembly, descBase + "PluginMain", nullptr, {
            GetScripts()->ToManagedObject("Urho3DNet", "Urho3D.Context", reinterpret_cast<RefCounted*>(context_))});
        auto objectHandle = GetScripts()->Lock(object.GetVoidPtr(), false);
        GetScripts()->CallMethod(assembly, descBase + "OnLoad", object.GetVoidPtr());
        plugins_[path] = objectHandle;
    }
    else
        URHO3D_LOGWARNINGF("Failed loading managed plugin \"%s\".", GetFileNameAndExtension(path).CString());
#endif

    return false;
}

bool PluginManagerManaged::UnloadPlugin(const String& path)
{
#if URHO3D_PLUGINS_CSHARP
    // TODO: Experimental/untested.
    auto it = plugins_.Find(path);
    if (it == plugins_.End())
    {
        URHO3D_LOGERRORF("Plugin %s was never loaded.", path.CString());
        return false;
    }

    gchandle handle = it->second_;
    if (auto* assembly = GetScripts()->LoadAssembly(path, nullptr)) // TODO: is this correct?
    {
        SendEvent(E_EDITORUSERCODERELOADSTART);

        // Call OnUnload
        auto name = GetFileName(path);
        GetScripts()->CallMethod(assembly, ToString("%s.%s:OnUnload", name.CString(), name.CString()),
            GetScripts()->GetObject(handle));

        // Unreference plugin object
        GetScripts()->Unlock(handle);

        // Unload assembly
        //GetScripts()->UnloadAssembly(assembly);
        plugins_.Erase(it);

        SendEvent(E_EDITORUSERCODERELOADEND);
        return true;
    }
#endif
    return false;
}

void PluginManagerManaged::OnEndFrame()
{
#if URHO3D_PLUGINS_CSHARP
    // TODO: hot reload
#endif
}

bool PluginManagerManaged::IsPluginPath(const String& path)
{
#if URHO3D_PLUGINS_CSHARP
    const char* start = "epm";
    const char* end = ".dll";
    String fileName = GetFileNameAndExtension(path).ToLower();
    return fileName.StartsWith(start) && fileName.EndsWith(end);
#else
    return false;
#endif
}

}
