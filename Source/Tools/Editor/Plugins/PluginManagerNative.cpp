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

#if URHO3D_PLUGINS_NATIVE
#   define CR_HOST
#endif

#include <EditorEventsPrivate.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include "PluginManagerNative.h"


namespace Urho3D
{

PluginManagerNative::PluginManagerNative(Context* context)
    : PluginManager(context)
{
#if URHO3D_PLUGINS_NATIVE
    SubscribeToEvent(E_ENDFRAME, std::bind(&PluginManagerNative::OnEndFrame, this));
#endif
}

bool PluginManagerNative::LoadPlugin(const String& path)
{
#if URHO3D_PLUGINS_NATIVE
    cr_plugin plugin{};
    if (cr_plugin_load(plugin, path.CString()))
    {
        plugin.userdata = context_;
        plugins_[path] = plugin;
        return true;
    }
    else
        URHO3D_LOGWARNINGF("Failed loading native plugin \"%s\".", GetFileNameAndExtension(path).CString());
#endif

    return false;
}

bool PluginManagerNative::UnloadPlugin(const String& path)
{
#if URHO3D_PLUGINS_NATIVE
    auto it = plugins_.Find(path);
    if (it == plugins_.End())
    {
        URHO3D_LOGERRORF("Plugin %s was never loaded.", path.CString());
        return false;
    }

    SendEvent(E_EDITORUSERCODERELOADSTART);

    cr_plugin_close(it->second_);
    plugins_.Erase(it);

    SendEvent(E_EDITORUSERCODERELOADEND);
    return true;
#else
    return false;
#endif
}

void PluginManagerNative::OnEndFrame()
{
#if URHO3D_PLUGINS_NATIVE
    for (auto it = plugins_.Begin(); it != plugins_.End(); it++)
    {
        cr_plugin& plugin = it->second_;
        if (plugin.userdata)
        {
            bool reloading = cr_plugin_changed(plugin);
            if (reloading)
                SendEvent(E_EDITORUSERCODERELOADSTART);

            if (cr_plugin_update(plugin) != 0)
            {
                const String& path = it->first_;
                URHO3D_LOGERRORF("Processing plugin \"%s\" failed and it was unloaded.",
                    GetFileNameAndExtension(path).CString());
                cr_plugin_close(plugin);
                plugin.userdata = nullptr;
            }

            if (reloading)
            {
                SendEvent(E_EDITORUSERCODERELOADEND);
                if (plugin.userdata != nullptr)
                {
                    const String& path = it->first_;
                    URHO3D_LOGINFOF("Loaded plugin \"%s\" version %d.",
                        GetFileNameAndExtension(path).CString(), plugin.version);
                }
            }
        }
    }
#endif
}

PluginManager::PluginPathType PluginManagerNative::IsPluginPath(const String& path)
{
#if URHO3D_PLUGINS_NATIVE
#if WIN32
    const char* start = "epn";
    const char* end = ".dll";
#elif APPLE
    const char* start = "libepn";
    const char* end = ".dylib";
#else
    const char* start = "libepn";
    const char* end = ".so";
#endif
    String fileName = GetFileNameAndExtension(path).ToLower();
    auto lastCharacterPos = path.Length() - strlen(end) - 1;
    auto lastCharacter = static_cast<unsigned int>(fileName[lastCharacterPos]);

    bool isLib = fileName.EndsWith(end);
#if WIN32
    bool isPdb = fileName.EndsWith(".pdb");
#else
    bool isPdb = false;
#endif
    if (fileName.StartsWith(start))
    {
        if (IsDigit(lastCharacter) && (isLib || isPdb))
            // Last file name character before extension can not be digit. cr appends a number to file name for versioning of
            // assemblies. We must not load these versions as plugins as it is done internally by cr.
            return PPT_TEMPORARY;
        else if (isLib)
            return PPT_VALID;
    }
#endif
    return PPT_INVALID;
}

}
