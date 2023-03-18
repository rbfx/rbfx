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

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/MountedDirectory.h"
#include "../IO/MountPoint.h"
#include "../IO/PackageFile.h"
#include "../IO/VirtualFileSystem.h"

#include <EASTL/bonus/adaptors.h>

namespace Urho3D
{

VirtualFileSystem::VirtualFileSystem(Context* context)
    : Object(context)
{
}

VirtualFileSystem::~VirtualFileSystem() = default;

void VirtualFileSystem::MountDir(const ea::string& path)
{
    MountDir(EMPTY_STRING, path);
}

void VirtualFileSystem::MountDir(const ea::string& scheme, const ea::string& path)
{
    Mount(MakeShared<MountedDirectory>(context_, path, scheme));
}

void VirtualFileSystem::AutomountDir(const ea::string& path)
{
    AutomountDir(EMPTY_STRING, path);
}

void VirtualFileSystem::AutomountDir(const ea::string& scheme, const ea::string& path)
{
    auto fileSystem = GetSubsystem<FileSystem>();
    if (!fileSystem->DirExists(path))
        return;

    // Add all the subdirs (non-recursive) as resource directory
    ea::vector<ea::string> subdirs;
    fileSystem->ScanDir(subdirs, path, "*", SCAN_DIRS);
    for (const ea::string& dir : subdirs)
    {
        if (dir.starts_with("."))
            continue;

        ea::string autoResourceDir = AddTrailingSlash(path) + dir;
        MountDir(scheme, autoResourceDir);
    }

    // Add all the found package files (non-recursive)
    ea::vector<ea::string> packageFiles;
    fileSystem->ScanDir(packageFiles, path, "*.pak", SCAN_FILES);
    for (const ea::string& packageFile : packageFiles)
    {
        if (packageFile.starts_with("."))
            continue;

        ea::string autoPackageName = AddTrailingSlash(path) + packageFile;
        MountPackageFile(autoPackageName);
    }
}

void VirtualFileSystem::MountPackageFile(const ea::string& path)
{
    const auto packageFile = MakeShared<PackageFile>(context_);
    if (packageFile->Open(path, 0u))
        Mount(packageFile);
}

void VirtualFileSystem::Mount(MountPoint* mountPoint)
{
    MutexLock lock(mountMutex_);

    const SharedPtr<MountPoint> pointPtr{mountPoint};
    if (mountPoints_.find(pointPtr) != mountPoints_.end())
    {
        return;
    }
    mountPoints_.push_back(pointPtr);

    mountPoint->SetWatching(isWatching_);
}

void VirtualFileSystem::MountExistingPackages(
    const StringVector& prefixPaths, const StringVector& relativePaths)
{
    const auto fileSystem = context_->GetSubsystem<FileSystem>();

    for (const ea::string& prefixPath : prefixPaths)
    {
        for (const ea::string& relativePath : relativePaths)
        {
            const ea::string packagePath = prefixPath + relativePath;
            if (fileSystem->FileExists(packagePath))
                MountPackageFile(packagePath);
        }
    }
}

void VirtualFileSystem::MountExistingDirectoriesOrPackages(
    const StringVector& prefixPaths, const StringVector& relativePaths)
{
    const auto fileSystem = context_->GetSubsystem<FileSystem>();

    for (const ea::string& prefixPath : prefixPaths)
    {
        for (const ea::string& relativePath : relativePaths)
        {
            const ea::string packagePath = prefixPath + relativePath + ".pak";
            const ea::string directoryPath = prefixPath + relativePath;
            if (fileSystem->FileExists(packagePath))
                MountPackageFile(packagePath);
            else if (fileSystem->DirExists(directoryPath))
                MountDir(directoryPath);
        }
    }
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

AbstractFilePtr VirtualFileSystem::OpenFile(const FileIdentifier& fileName, FileMode mode) const
{
    MutexLock lock(mountMutex_);

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        if (AbstractFilePtr result = mountPoint->OpenFile(fileName, mode))
            return result;
    }

    return AbstractFilePtr();
}

ea::string VirtualFileSystem::GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const
{
    MutexLock lock(mountMutex_);

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        const ea::string result = mountPoint->GetAbsoluteNameFromIdentifier(fileName);
        if (!result.empty())
            return result;
    }

    // TODO(vfs): This is a hack to support absolute paths, they should be handled by the mount points.
    // Fallback to absolute path resolution, similar to ResourceCache behaviour.
    if (fileName.scheme_.empty())
    {
        const auto* fileSystem = GetSubsystem<FileSystem>();
        if (IsAbsolutePath(fileName.fileName_) && fileSystem->FileExists(fileName.fileName_))
            return fileName.fileName_;
    }

    return EMPTY_STRING;
}

FileIdentifier VirtualFileSystem::GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName)
{
    MutexLock lock(mountMutex_);

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        const FileIdentifier result = mountPoint->GetIdentifierFromAbsoluteName(absoluteFileName);
        if (result)
            return result;
    }

    return FileIdentifier::Empty;
}

FileIdentifier VirtualFileSystem::GetIdentifierFromAbsoluteName(
    const ea::string& scheme, const ea::string& absoluteFileName)
{
    MutexLock lock(mountMutex_);

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        if (!mountPoint->AcceptsScheme(scheme))
            continue;

        const FileIdentifier result = mountPoint->GetIdentifierFromAbsoluteName(absoluteFileName);
        if (result)
            return result;
    }

    return FileIdentifier::Empty;
}

void VirtualFileSystem::SetWatching(bool enable)
{
    if (isWatching_ != enable)
    {
        MutexLock lock(mountMutex_);

        isWatching_ = enable;
        for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
        {
            (*i)->SetWatching(isWatching_);
        }
    }
}

void VirtualFileSystem::Scan(
    ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, ScanFlags flags) const
{
    MutexLock lock(mountMutex_);

    if (!flags.Test(SCAN_APPEND))
        result.clear();

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
        mountPoint->Scan(result, pathName, filter, flags | SCAN_APPEND);
}

bool VirtualFileSystem::Exists(const FileIdentifier& fileName) const
{
    MutexLock lock(mountMutex_);

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        if (mountPoint->Exists(fileName))
            return true;
    }
    return false;
}

} // namespace Urho3D
