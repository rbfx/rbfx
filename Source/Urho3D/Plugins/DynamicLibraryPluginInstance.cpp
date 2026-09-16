// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Plugins/DynamicLibraryPluginInstance.h"

#include "Urho3D/Core/ProcessUtils.h"
#include "Urho3D/Core/StringUtils.h"
#include "Urho3D/Engine/Engine.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Plugins/PluginManager.h"
#include "Urho3D/Script/Script.h"

namespace Urho3D
{

bool DynamicLibraryPluginInstance::Load()
{
    // Locate binaries
    originalFileName_ = GetAbsoluteFileName(name_, true);
    temporaryFileName_ = GetAbsoluteFileName(name_, false);
    if (originalFileName_.empty() || temporaryFileName_.empty())
    {
        URHO3D_LOGERROR("Plugin '{}' is not found", name_);
        return false;
    }

    URHO3D_PROFILE("LoadModule");

    // If paths are different, patch pdb name in temporary file
    if (temporaryFileName_ != originalFileName_)
        PatchTemporaryBinary(temporaryFileName_);

    // Try to load temporary or original file
    lastModuleType_ = MODULE_INVALID;
    if (!module_.Load(temporaryFileName_))
        return false;

    plugin_ = module_.InstantiatePlugin();
    if (!plugin_)
        return false;

    const ea::string& oldName = plugin_->GetPluginName();
    if (!oldName.empty() && plugin_->GetPluginName() != name_)
        URHO3D_LOGWARNING("Plugin name mismatch: file {} contains plugin {}. This plugin may be incompatible in static build.", name_, oldName);
    plugin_->SetPluginName(name_);

    lastModificationTime_ = context_->GetSubsystem<FileSystem>()->GetLastModifiedTime(originalFileName_);
    ++version_;
    unloading_ = false;
    lastModuleType_ = module_.GetModuleType();

    URHO3D_LOGDEBUG("Plugin {} version {} is loaded from {}", name_, version_, temporaryFileName_);

    return true;
}

bool DynamicLibraryPluginInstance::IsLoaded() const
{
    return module_.GetModuleType() != MODULE_INVALID && !unloading_ && plugin_ != nullptr;
}

bool DynamicLibraryPluginInstance::PerformUnload()
{
    if (!plugin_)
        return false;

    URHO3D_PROFILE("UnloadModule");

    // Disposing object requires managed reference to be the last one alive.
    WeakPtr<Plugin> application(plugin_);
    plugin_->Dispose();

#if URHO3D_CSHARP
    if (module_.GetModuleType() == MODULE_MANAGED)
        Script::GetRuntimeApi()->Dispose(plugin_.Detach());
#endif

    plugin_ = nullptr;
    if (!module_.Unload())
        return false;

    return true;
}

ea::string DynamicLibraryPluginInstance::GetTemporaryPdbName(const ea::string& fileName)
{
    ea::string path, file, extension;
    SplitPath(fileName, path, file, extension);
    file.back() = '_';
    return path + file + extension;
}

ea::string DynamicLibraryPluginInstance::GetAbsoluteFileName(const ea::string& name, bool original) const
{
#if __linux__ || __APPLE__
    static const ea::string_view prefix = "lib";
#else
    static const ea::string_view prefix = "";
#endif

    auto fs = context_->GetSubsystem<FileSystem>();
    auto pluginManager = GetSubsystem<PluginManager>();

    const ea::string& originalDirectory = pluginManager->GetOriginalBinaryDirectory();
    const ea::string& temporaryDirectory = pluginManager->GetTemporaryBinaryDirectory();
    const ea::string& directory = original || temporaryDirectory.empty() ? originalDirectory : temporaryDirectory;

    const ea::string fileName = Format("{}{}{}{}", directory, prefix, name, DYN_LIB_SUFFIX);
    if (fs->FileExists(fileName))
        return fileName;

    return EMPTY_STRING;
}

void DynamicLibraryPluginInstance::PatchTemporaryBinary(const ea::string& fileName)
{
#if _MSC_VER || URHO3D_CSHARP
    unsigned pdbOffset = 0, pdbSize = 0;
    const ModuleType type = DynamicLibrary::ReadModuleInformation(context_, fileName, &pdbOffset, &pdbSize);
#if _MSC_VER
    bool hashPdb = true;
#else
    bool hashPdb = type == MODULE_MANAGED;
#endif
    if (hashPdb)
    {
        if (pdbOffset != 0)
        {
            File dll(context_);
            if (dll.Open(fileName, FILE_READWRITE))
            {
                dll.Seek(pdbOffset);

                ea::string pdbName(pdbSize + 1, '\0');
                dll.Read(pdbName.data(), pdbSize);
                pdbName = GetTemporaryPdbName(GetFileNameAndExtension(pdbName));
                pdbName.resize(pdbSize + 1);

                dll.Seek(pdbOffset);
                dll.Write(pdbName.data(), pdbSize);
            }
        }
    }
#if URHO3D_CSHARP
    if (type == MODULE_MANAGED)
    {
        // Managed runtime will modify file version in specified file.
        Script::GetRuntimeApi()->SetAssemblyVersion(fileName, version_ + 1);
    }
#endif
#endif
}

bool DynamicLibraryPluginInstance::IsOutOfDate() const
{
    return lastModificationTime_ < context_->GetSubsystem<FileSystem>()->GetLastModifiedTime(originalFileName_);
}

bool DynamicLibraryPluginInstance::IsReadyToReload() const
{
    URHO3D_PROFILE("IsModuleReadyToReload");
    return DynamicLibrary::ReadModuleInformation(context_, originalFileName_) == lastModuleType_;
}

}
