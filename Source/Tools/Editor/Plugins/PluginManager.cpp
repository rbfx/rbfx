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

#if URHO3D_PLUGINS

#define CR_HOST

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <EditorEvents.h>
#include "PluginManager.h"
#include "EditorEventsPrivate.h"


namespace Urho3D
{

#if __linux__
static const char* platformDynamicLibrarySuffix = ".so";
#elif _WIN32
static const char* platformDynamicLibrarySuffix = ".dll";
#elif __APPLE__
static const char* platformDynamicLibrarySuffix = ".dylib";
#else
#   error Unsupported platform.
#endif

Plugin::Plugin(Context* context)
    : Object(context)
{
}

PluginManager::PluginManager(Context* context)
    : Object(context)
{
    CleanUp();
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) { OnEndFrame(); });
    SubscribeToEvent(E_SIMULATIONSTART, [this](StringHash, VariantMap&) {
        for (auto& plugin : plugins_)
        {
            if (plugin->nativeContext_.userdata != nullptr && plugin->nativeContext_.userdata != context_)
                reinterpret_cast<PluginApplication*>(plugin->nativeContext_.userdata)->Start();
        }
    });
    SubscribeToEvent(E_SIMULATIONSTOP, [this](StringHash, VariantMap&) {
        for (auto& plugin : plugins_)
        {
            if (plugin->nativeContext_.userdata != nullptr && plugin->nativeContext_.userdata != context_)
                reinterpret_cast<PluginApplication*>(plugin->nativeContext_.userdata)->Stop();
        }
    });
}

PluginType PluginManager::GetPluginType(const String& path)
{
    File file(context_);
    if (!file.Open(path, FILE_READ))
        return PLUGIN_INVALID;

    // This function implements a naive check for plugin validity. Proper check would parse executable headers and look
    // for relevant exported function names.

#if __linux__
    // ELF magic
    if (path.EndsWith(".so"))
    {
        if (file.ReadUInt() == 0x464C457F)
        {
            file.Seek(0);
            String buf{ };
            buf.Resize(file.GetSize());
            file.Read(&buf[0], file.GetSize());
            auto pos = buf.Find("cr_main");
            // Function names are preceeded with 0 in elf files.
            if (pos != String::NPOS && buf[pos - 1] == 0)
                return PLUGIN_NATIVE;
        }
    }
#endif
    file.Seek(0);
    if (path.EndsWith(".dll"))
    {
        if (file.ReadShort() == 0x5A4D)
        {
#if _WIN32
            // But only on windows we check if PE file is a native plugin
            file.Seek(0);
            String buf{};
            buf.Resize(file.GetSize());
            file.Read(&buf[0], file.GetSize());
            auto pos = buf.Find("cr_main");
            // Function names are preceeded with 2 byte hint which is preceeded with 0 in PE files.
            if (pos != String::NPOS && buf[pos - 3] == 0)
                return PLUGIN_NATIVE;
#endif
            // PE files are handled on all platforms because managed executables are PE files.
            file.Seek(0x3C);
            auto e_lfanew = file.ReadUInt();
#if URHO3D_64BIT
            const auto netMetadataRvaOffset = 0xF8;
#else
            const auto netMetadataRvaOffset = 0xE8;
#endif
            file.Seek(e_lfanew + netMetadataRvaOffset);  // Seek to .net metadata directory rva

            if (file.ReadUInt() != 0)
                return PLUGIN_MANAGED;
        }
    }

    if (path.EndsWith(".dylib"))
    {
        // TODO: MachO file support.
    }

    return PLUGIN_INVALID;
}

Plugin* PluginManager::Load(const String& name)
{
    if (Plugin* loaded = GetPlugin(name))
        return loaded;

    CleanUp();

    String pluginPath = NameToPath(name);
    if (pluginPath.Empty())
        return nullptr;

    SharedPtr<Plugin> plugin(new Plugin(context_));
    plugin->type_ = GetPluginType(pluginPath);

    if (plugin->type_ == PLUGIN_NATIVE)
    {
        if (cr_plugin_load(plugin->nativeContext_, pluginPath.CString()))
        {
            plugin->nativeContext_.userdata = context_;
            plugin->name_ = name;
            plugin->path_ = pluginPath;
            plugins_.Push(plugin);
            return plugin.Get();
        }
        else
            URHO3D_LOGWARNINGF("Failed loading native plugin \"%s\".", name.CString());
    }
    else if (plugin->type_ == PLUGIN_MANAGED)
    {
        // TODO
    }

    return nullptr;
}

bool PluginManager::Unload(Plugin* plugin)
{
    if (plugin == nullptr)
        return false;

    auto it = plugins_.Find(SharedPtr<Plugin>(plugin));
    if (it == plugins_.End())
    {
        URHO3D_LOGERRORF("Plugin %s was never loaded.", plugin->name_.CString());
        return false;
    }

    SendEvent(E_EDITORUSERCODERELOADSTART);
    cr_plugin_close(plugin->nativeContext_);
    SendEvent(E_EDITORUSERCODERELOADEND);
    URHO3D_LOGINFOF("Plugin %s was unloaded.", plugin->name_.CString());
    plugins_.Erase(it);

    CleanUp();

    return true;
}

void PluginManager::OnEndFrame()
{
#if URHO3D_PLUGINS
    for (auto it = plugins_.Begin(); it != plugins_.End(); it++)
    {
        Plugin* plugin = it->Get();
        if (plugin->type_ == PLUGIN_NATIVE && plugin->nativeContext_.userdata)
        {
            bool reloading = cr_plugin_changed(plugin->nativeContext_);
            if (reloading)
                SendEvent(E_EDITORUSERCODERELOADSTART);

            if (cr_plugin_update(plugin->nativeContext_) != 0)
            {
                URHO3D_LOGERRORF("Processing plugin \"%s\" failed and it was unloaded.",
                    GetFileNameAndExtension(plugin->name_).CString());
                cr_plugin_close(plugin->nativeContext_);
                plugin->nativeContext_.userdata = nullptr;
                continue;
            }

            if (reloading)
            {
                SendEvent(E_EDITORUSERCODERELOADEND);
                if (plugin->nativeContext_.userdata != nullptr)
                {
                    URHO3D_LOGINFOF("Loaded plugin \"%s\" version %d.",
                        GetFileNameAndExtension(plugin->name_).CString(), plugin->nativeContext_.version);
                }
            }
        }
    }
#endif
}

void PluginManager::CleanUp(String directory)
{
    if (directory.Empty())
        directory = GetFileSystem()->GetProgramDir();

    if (!GetFileSystem()->DirExists(directory))
        return;

    StringVector files;
    GetFileSystem()->ScanDir(files, directory, "*.*", SCAN_FILES, false);

    for (const String& file : files)
    {
        bool possiblyPlugin = false;
#if __linux__
        possiblyPlugin |= file.EndsWith(".so");
#endif
#if __APPLE__
        possiblyPlugin |= file.EndsWith(".dylib");
#endif
        possiblyPlugin |= file.EndsWith(".dll");

        if (possiblyPlugin)
        {
            String name = GetFileName(file);
            if (IsDigit(static_cast<unsigned int>(name.Back())))
                GetFileSystem()->Delete(ToString("%s/%s", directory.CString(), file.CString()));
        }
    }
}

Plugin* PluginManager::GetPlugin(const String& name)
{
    for (auto it = plugins_.Begin(); it != plugins_.End(); it++)
    {
        if (it->Get()->name_ == name)
            return it->Get();
    }
    return nullptr;
}

String PluginManager::NameToPath(const String& name) const
{
    FileSystem* fs = GetFileSystem();
    String result;

#if __linux__ || __APPLE__
    result = ToString("%s/lib%s%s", fs->GetProgramDir().CString(), name.CString(), platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;
#endif
    result = ToString("%s/%s%s", fs->GetProgramDir().CString(), name.CString(), platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;

    return String::EMPTY;
}

String PluginManager::PathToName(const String& path)
{
    if (path.EndsWith(platformDynamicLibrarySuffix))
    {
        String name = GetFileName(path);
#if __linux__ || __APPLE__
        if (name.StartsWith("lib"))
            name = name.Substring(3);
        return name;
#endif
    }
    return String::EMPTY;
}

}

#endif
