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

#include <atomic>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Thread.h>
#include <Urho3D/Engine/PluginApplication.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <EditorEvents.h>
#include "Editor.h"
#include "Plugins/PluginManager.h"
#if !_WIN32
#   include "Plugins/PE.h"
#endif

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

#if URHO3D_CSHARP && URHO3D_PLUGINS

struct DomainManagerInterface
{
    void* handle;
    bool(*LoadPlugin)(void* handle, const char* path);
    void(*SetReloading)(void* handle, bool reloading);
    bool(*GetReloading)(void* handle);
    bool(*IsPlugin)(void* handle, const char* path);
};

static std::atomic_bool managedInterfaceSet{false};
static DomainManagerInterface managedInterface_;

///
extern "C" URHO3D_EXPORT_API void ParseArgumentsC(int argc, char** argv) { ParseArguments(argc, argv); }

///
extern "C" URHO3D_EXPORT_API Application* CreateEditorApplication(Context* context) { return new Editor(context); }

/// Update a pointer to a struct that facilitates interop between native code and a .net runtime manager object. Will be called every time .net code is reloaded.
extern "C" URHO3D_EXPORT_API void SetManagedRuntimeInterface(DomainManagerInterface* managedInterface)
{
    managedInterface_ = *managedInterface;
    managedInterfaceSet = true;
}

#endif

Plugin::Plugin(Context* context)
    : Object(context)
{
}

bool Plugin::Unload()
{
    if (type_ == PLUGIN_NATIVE)
    {
        cr_plugin_close(nativeContext_);
        nativeContext_.userdata = nullptr;
        return true;
    }
#if URHO3D_CSHARP
    else if (type_ == PLUGIN_MANAGED)
    {
        // Managed plugin unloading requires to tear down entire AppDomain and recreate it. Instruct main .net thread to
        // do that and wait.
        managedInterfaceSet = false;
        managedInterface_.SetReloading(managedInterface_.handle, true);
        managedInterface_.handle = nullptr;

        // Wait for AppDomain reload.
        while (!managedInterfaceSet)
            Time::Sleep(0);

        return true;
    }
#endif
    return false;
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
    // This function implements a naive check for plugin validity. Proper check would parse executable headers and look
    // for relevant exported function names.
    Context* context = reinterpret_cast<SystemUI*>(ui::GetIO().UserData)->GetContext();
#if __linux__
    // ELF magic
    if (path.EndsWith(".so"))
    {
        File file(context);
        if (!file.Open(path, FILE_READ))
            return PLUGIN_INVALID;

        if (file.ReadUInt() == 0x464C457F)
        {
            file.Seek(0);
            String buf{ };
            buf.Resize(file.GetSize());
            file.Read(&buf[0], file.GetSize());
            auto pos = buf.Find("cr_main");
            // Function names are preceeded with 0 in elf files.
            // TODO: Proper analysis of elf file format.
            if (pos != String::NPOS && buf[pos - 1] == 0)
                return PLUGIN_NATIVE;
        }
    }
#endif
    if (path.EndsWith(".dll"))
    {
        File file(context);
        if (!file.Open(path, FILE_READ))
            return PLUGIN_INVALID;

        if (file.ReadShort() == IMAGE_DOS_SIGNATURE)
        {
            String buf{};
            buf.Resize(file.GetSize());
            file.Seek(0);
            file.Read(&buf[0], file.GetSize());
            file.Close();

            const auto base = reinterpret_cast<const char*>(buf.CString());
            const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);

            if (nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC)
                return PLUGIN_INVALID;

            const auto& eatDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
            const auto& netDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR];
            if (netDir.VirtualAddress != 0)
            {
                // Verify that plugin has a class that inherits from PluginApplication.
                if (managedInterface_.IsPlugin(managedInterface_.handle, path.CString()))
                    return PLUGIN_MANAGED;
            }
            else if (eatDir.VirtualAddress > 0)
            {
                // Verify that plugin has exported function named cr_main.
                // Find section that contains EAT.
                const auto* section = IMAGE_FIRST_SECTION(nt);
                uint32_t eatModifier = 0;
                for (auto i = 0; i < nt->FileHeader.NumberOfSections; i++, section++)
                {
                    if (eatDir.VirtualAddress >= section->VirtualAddress && eatDir.VirtualAddress < (section->VirtualAddress + section->SizeOfRawData))
                    {
                        eatModifier = section->VirtualAddress - section->PointerToRawData;
                        break;
                    }
                }

                const auto eat = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + eatDir.VirtualAddress - eatModifier);
                const auto names = reinterpret_cast<const uint32_t*>(base + eat->AddressOfNames - eatModifier);
                for (auto i = 0; i < eat->NumberOfFunctions; i++)
                {
                    if (strcmp(base + names[i] - eatModifier, "cr_main") == 0)
                        return PLUGIN_NATIVE;
                }
            }
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
#if URHO3D_PLUGINS
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
#if URHO3D_CSHARP
    else if (plugin->type_ == PLUGIN_MANAGED)
    {
        if (managedInterface_.LoadPlugin(managedInterface_.handle, pluginPath.CString()))
        {
            plugin->name_ = name;
            plugin->path_ = pluginPath;
            plugins_.Push(plugin);
            return plugin.Get();
        }
    }
#endif
#endif
    return nullptr;
}

void PluginManager::Unload(Plugin* plugin)
{
    if (plugin == nullptr)
        return;

    auto it = plugins_.Find(SharedPtr<Plugin>(plugin));
    if (it == plugins_.End())
    {
        URHO3D_LOGERRORF("Plugin %s was never loaded.", plugin->name_.CString());
        return;
    }

    plugin->unloading_ = true;
}

void PluginManager::OnEndFrame()
{
#if URHO3D_PLUGINS
    for (auto it = plugins_.Begin(); it != plugins_.End();)
    {
        Plugin* plugin = it->Get();

        if (plugin->unloading_)
        {
            SendEvent(E_EDITORUSERCODERELOADSTART);
            // Actual unload
            plugin->Unload();
            if (plugin->type_ == PLUGIN_MANAGED)
            {
                // Now load back all managed plugins except this one.
                for (auto& plug : plugins_)
                {
                    if (plug == plugin || plug->type_ == PLUGIN_NATIVE)
                        continue;
                    managedInterface_.LoadPlugin(managedInterface_.handle, plug->path_.CString());
                }
            }

            SendEvent(E_EDITORUSERCODERELOADEND);
            URHO3D_LOGINFOF("Plugin %s was unloaded.", plugin->name_.CString());
            it = plugins_.Erase(it);
        }
        else if (plugin->type_ == PLUGIN_NATIVE && plugin->nativeContext_.userdata)
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

            it++;
        }
        else
            it++;
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
    result = ToString("%slib%s%s", fs->GetProgramDir().CString(), name.CString(), platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;
#endif

#if !_WIN32
    result = ToString("%s%s%s", fs->GetProgramDir().CString(), name.CString(), ".dll");
    if (fs->FileExists(result))
        return result;
#endif

    result = ToString("%s%s%s", fs->GetProgramDir().CString(), name.CString(), platformDynamicLibrarySuffix);
    if (fs->FileExists(result))
        return result;

    return String::EMPTY;
}

String PluginManager::PathToName(const String& path)
{
#if !_WIN32
    if (path.EndsWith(platformDynamicLibrarySuffix))
    {
        String name = GetFileName(path);
#if __linux__ || __APPLE__
        if (name.StartsWith("lib"))
            name = name.Substring(3);
#endif
        return name;
    }
    else
#endif
    if (path.EndsWith(".dll"))
        return GetFileName(path);
    return String::EMPTY;
}

PluginManager::~PluginManager()
{
    for (auto& plugin : plugins_)
        plugin->Unload();
}

}

#endif
