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

#include "../Plugins/ModulePlugin.h"

#include "../Core/ProcessUtils.h"
#include "../Core/StringUtils.h"
#include "../Engine/Engine.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/VectorBuffer.h"
#include "../Plugins/PluginManager.h"
#include "../Script/Script.h"

namespace Urho3D
{

bool ModulePlugin::Load()
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

    application_ = module_.InstantiatePlugin();
    if (!application_)
        return false;

    const ea::string& oldName = application_->GetPluginName();
    if (!oldName.empty() && application_->GetPluginName() != name_)
        URHO3D_LOGWARNING("Plugin name mismatch: file {} contains plugin {}. This plugin may be incompatible in static build.", name_, oldName);
    application_->SetPluginName(name_);

    lastModificationTime_ = context_->GetSubsystem<FileSystem>()->GetLastModifiedTime(originalFileName_);
    ++version_;
    unloading_ = false;
    lastModuleType_ = module_.GetModuleType();

    URHO3D_LOGDEBUG("Plugin {} version {} is loaded from {}", name_, version_, temporaryFileName_);

    return true;
}

bool ModulePlugin::IsLoaded() const
{
    return module_.GetModuleType() != MODULE_INVALID && !unloading_ && application_ != nullptr;
}

bool ModulePlugin::PerformUnload()
{
    if (!application_)
        return false;

    URHO3D_PROFILE("UnloadModule");

    // Disposing object requires managed reference to be the last one alive.
    WeakPtr<PluginApplication> application(application_);
    application_->Dispose();

#if URHO3D_CSHARP
    if (module_.GetModuleType() == MODULE_MANAGED)
        Script::GetRuntimeApi()->Dispose(application_.Detach());
#endif

    application_ = nullptr;
    if (!module_.Unload())
        return false;

    return true;
}

ea::string ModulePlugin::GetTemporaryPdbName(const ea::string& fileName)
{
    ea::string path, file, extension;
    SplitPath(fileName, path, file, extension);
    file.back() = '_';
    return path + file + extension;
}

ea::string ModulePlugin::GetAbsoluteFileName(const ea::string& name, bool original) const
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

void ModulePlugin::PatchTemporaryBinary(const ea::string& fileName)
{
#if _MSC_VER || URHO3D_CSHARP
    unsigned pdbOffset = 0, pdbSize = 0;
    const ModuleType type = DynamicModule::ReadModuleInformation(context_, fileName, &pdbOffset, &pdbSize);
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

bool ModulePlugin::IsOutOfDate() const
{
    return lastModificationTime_ < context_->GetSubsystem<FileSystem>()->GetLastModifiedTime(originalFileName_);
}

bool ModulePlugin::IsReadyToReload() const
{
    URHO3D_PROFILE("IsModuleReadyToReload");
    return DynamicModule::ReadModuleInformation(context_, originalFileName_) == lastModuleType_;
}

}
