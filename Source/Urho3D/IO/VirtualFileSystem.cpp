//
// Copyright (c) 2022-2022 the Urho3D project.
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

#include "../IO/VirtualFileSystem.h"

#include "PackageFile.h"
#include "../IO/Log.h"
#include "../IO/FileSystem.h"
#include "../IO/MountedDirectory.h"
#include "../IO/MountPoint.h"
#include "../Core/Context.h"
#include "../Engine/EngineDefs.h"
#include "../Engine/Engine.h"

namespace Urho3D
{

VirtualFileSystem::VirtualFileSystem(Context* context)
    : Object(context)
{
}

VirtualFileSystem::~VirtualFileSystem() = default;

void VirtualFileSystem::Mount(MountPoint* mountPoint)
{
    MutexLock lock(mountMutex_);

    const SharedPtr<MountPoint> pointPtr{mountPoint};
    if (mountPoints_.find(pointPtr) != mountPoints_.end())
    {
        return;
    }
    mountPoints_.push_back(pointPtr);
}

void VirtualFileSystem::MountDefault()
{
    const auto engine = context_->GetSubsystem<Engine>();
    const auto fileSystem = context_->GetSubsystem<FileSystem>();

    const ea::string& prefixPaths = engine->GetParameter(EP_RESOURCE_PREFIX_PATHS).GetString();
    const ea::string& packages = engine->GetParameter(EP_RESOURCE_PACKAGES).GetString();
    const ea::string& paths = engine->GetParameter(EP_RESOURCE_PATHS).GetString();
    const ea::string& autoloadPaths = engine->GetParameter(EP_AUTOLOAD_PATHS).GetString();
        

    // Converts paths to absolute
    ea::vector<ea::string> resourcePrefixPaths = prefixPaths.split(';', true);
    const auto& programDir = fileSystem->GetProgramDir();
    bool hasProgramDir = false;
    for (unsigned i = 0; i < resourcePrefixPaths.size(); ++i)
    {
        hasProgramDir |= resourcePrefixPaths[i].empty() || resourcePrefixPaths[i] == programDir;
        resourcePrefixPaths[i] = AddTrailingSlash(IsAbsolutePath(resourcePrefixPaths[i])
                ? resourcePrefixPaths[i] : programDir + resourcePrefixPaths[i]);
    }

    // Append program dir as default fall-back option
    if (!hasProgramDir)
        resourcePrefixPaths.push_back(programDir);

    // Combine resource paths and autoload paths. Autoload appended last to have most priority.
    ea::vector<ea::string> resourcePaths = paths.split(';');
    resourcePaths.append(autoloadPaths.split(';'));

    // Split package names.
    ea::vector<ea::string> resourcePackages = packages.split(';');

    ea::vector<SharedPtr<MountedDirectory>> directories;
    for (auto& resourcePrefixPath : resourcePrefixPaths)
    {
        for (auto& resourcePackage: resourcePackages)
        {
            auto packagePath = resourcePrefixPath + resourcePackage;
            if (fileSystem->FileExists(packagePath))
            {
                Mount(MakeShared<PackageFile>(context_, packagePath, 0));
            }
        }
        for (auto& resourcePath : resourcePaths)
        {
            auto dirPath = resourcePrefixPath + resourcePath;
            if (fileSystem->DirExists(dirPath))
            {
                directories.push_back(MakeShared<MountedDirectory>(context_, dirPath));
            }
        }
    }
    for (auto& dir : directories)
        Mount(dir);

    Mount(MakeShared<MountedDirectory>(context_, engine->GetAppPreferencesDir(), "conf"));
}

void VirtualFileSystem::Unmount(MountPoint* mountPoint)
{
    MutexLock lock(mountMutex_);

    const SharedPtr<MountPoint> pointPtr{mountPoint};
    const auto i = mountPoints_.find(pointPtr);
    if (i != mountPoints_.end())
    {
        // Erase the slow way because order of the mount points matters.
        mountPoints_.erase(i);
    }
}

void VirtualFileSystem::UnmountAll()
{
    MutexLock lock(mountMutex_);

    mountPoints_.clear();
}

MountPoint* VirtualFileSystem::GetMountPoint(unsigned index) const
{
    MutexLock lock(mountMutex_);

    return (index < mountPoints_.size()) ? mountPoints_[index].Get() : nullptr;
}

/// Open file in a virtual file system. Returns null if file not found.
AbstractFilePtr VirtualFileSystem::OpenFile(const FileIdentifier& fileName, FileMode mode) const
{
    MutexLock lock(mountMutex_);

    for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
    {
        if (AbstractFilePtr result = (*i)->OpenFile(fileName, mode))
            return result;
    }
    return AbstractFilePtr();
}

ea::string VirtualFileSystem::GetFileName(const FileIdentifier& fileName)
{
    MutexLock lock(mountMutex_);

    for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
    {
        ea::string result = (*i)->GetFileName(fileName);
        if (!result.empty())
            return result;
    }

    // Fallback to absolute path resolution, similar to ResouceCache behaviour.
    if (fileName.scheme_.empty())
    {
        const auto* fileSystem = GetSubsystem<FileSystem>();
        if (IsAbsolutePath(fileName.fileName_) && fileSystem->FileExists(fileName.fileName_))
            return fileName.fileName_;
    }

    return ea::string();
}

/// Check if a file exists in the virtual file system.
bool VirtualFileSystem::Exists(const FileIdentifier& fileName) const
{
    MutexLock lock(mountMutex_);

    for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
    {
        if ((*i)->Exists(fileName))
            return true;
    }
    return false;
}


} // namespace Urho3D
