// Copyright (c) 2022-2022 the Urho3D project.
// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IO/VirtualFileSystem.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MountPoint.h"
#include "Urho3D/IO/MountedDirectory.h"
#include "Urho3D/IO/MountedRoot.h"
#include "Urho3D/IO/PackageFile.h"

#include <EASTL/bonus/adaptors.h>

namespace Urho3D
{

VirtualFileSystem::VirtualFileSystem(Context* context)
    : Object(context)
{
}

VirtualFileSystem::~VirtualFileSystem() = default;

MountPoint* VirtualFileSystem::MountAliasRoot()
{
    MutexLock lock(mountMutex_);
    return GetOrCreateAliasRoot();
}

MountedAliasRoot* VirtualFileSystem::GetOrCreateAliasRoot()
{
    if (!aliasMountPoint_)
    {
        aliasMountPoint_ = MakeShared<MountedAliasRoot>(context_);
        mountPoints_.push_back(aliasMountPoint_);
    }

    return aliasMountPoint_;
}

MountPoint* VirtualFileSystem::MountRoot()
{
    const auto mountPoint = MakeShared<MountedRoot>(context_);
    Mount(mountPoint);
    return mountPoint;
}

MountPoint* VirtualFileSystem::MountDir(const ea::string& path)
{
    return MountDir(EMPTY_STRING, path);
}

MountPoint* VirtualFileSystem::MountDir(const ea::string& scheme, const ea::string& path)
{
    const auto mountPoint = MakeShared<MountedDirectory>(context_, path, scheme);
    Mount(mountPoint);
    return mountPoint;
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

MountPoint* VirtualFileSystem::MountPackageFile(const ea::string& path)
{
    const auto packageFile = MakeShared<PackageFile>(context_);
    if (packageFile->Open(path, 0u))
    {
        Mount(packageFile);
        return packageFile;
    }

    return nullptr;
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

    if (auto mountedAliasRoot = dynamic_cast<MountedAliasRoot*>(mountPoint))
    {
        if (aliasMountPoint_)
            URHO3D_LOGWARNING("Mounted alias root when one already exists, overwriting.");
        aliasMountPoint_ = mountedAliasRoot;
    }
}

void VirtualFileSystem::MountAlias(const ea::string& alias, MountPoint* mountPoint, const ea::string& scheme)
{
    MutexLock lock(mountMutex_);

    GetOrCreateAliasRoot()->AddAlias(alias, scheme, mountPoint);
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
            {
                MountPoint* mountPoint = MountPackageFile(packagePath);
                if (mountPoint)
                    MountAlias(Format("res:{}", relativePath), mountPoint);
            }
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
            {
                MountPoint* mountPoint = MountPackageFile(packagePath);
                if (mountPoint)
                    MountAlias(Format("res:{}", relativePath), mountPoint);
            }
            else if (fileSystem->DirExists(directoryPath))
            {
                MountPoint* mountPoint = MountDir(directoryPath);
                if (mountPoint)
                    MountAlias(Format("res:{}", relativePath), mountPoint);
            }
        }
    }
}

void VirtualFileSystem::Unmount(MountPoint* mountPoint)
{
    MutexLock lock(mountMutex_);

    if (aliasMountPoint_)
        aliasMountPoint_->RemoveAliases(mountPoint);

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
    aliasMountPoint_ = nullptr;
}

MountPoint* VirtualFileSystem::GetMountPoint(unsigned index) const
{
    MutexLock lock(mountMutex_);

    return (index < mountPoints_.size()) ? mountPoints_[index].Get() : nullptr;
}

AbstractFilePtr VirtualFileSystem::OpenFile(const FileIdentifier& fileName, FileMode mode) const
{
    if (!fileName)
        return nullptr;

    MutexLock lock(mountMutex_);

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        if (AbstractFilePtr result = mountPoint->OpenFile(fileName, mode))
            return result;
    }

    return nullptr;
}

ea::string VirtualFileSystem::ReadAllText(const FileIdentifier& fileName) const
{
    AbstractFilePtr file = OpenFile(fileName, FILE_READ);
    if (!file)
        return EMPTY_STRING;

    const unsigned dataSize = file->GetSize();

    ea::string buffer;
    buffer.resize(dataSize);
    if (file->Read(buffer.data(), dataSize) != dataSize)
        return buffer.substr(0, dataSize);

    return buffer;
}

bool VirtualFileSystem::WriteAllText(const FileIdentifier& fileName, const ea::string& text) const
{
    AbstractFilePtr file = OpenFile(fileName, FILE_WRITE);
    if (!file)
        return false;

    return file->Write(text.data(), text.length()) == text.length();
}

FileTime VirtualFileSystem::GetLastModifiedTime(const FileIdentifier& fileName, bool creationIsModification) const
{
    MutexLock lock(mountMutex_);

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        if (const auto result = mountPoint->GetLastModifiedTime(fileName, creationIsModification))
            return *result;
    }

    return 0;
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

    return EMPTY_STRING;
}

FileIdentifier VirtualFileSystem::GetCanonicalIdentifier(const FileIdentifier& fileName) const
{
    FileIdentifier result = fileName;

    // .. is not supported
    result.fileName_.replace("../", "");
    result.fileName_.replace("./", "");
    result.fileName_.trim();

    // Attempt to go from "file" scheme to local schemes
    if (result.scheme_ == "file")
    {
        if (const auto betterName = GetIdentifierFromAbsoluteName(result.fileName_))
            result = betterName;
    }

    return result;
}

FileIdentifier VirtualFileSystem::GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const
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
    const ea::string& scheme, const ea::string& absoluteFileName) const
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

void VirtualFileSystem::Scan(ea::vector<ea::string>& result, const ea::string& scheme, const ea::string& pathName,
    const ea::string& filter, ScanFlags flags) const
{
    MutexLock lock(mountMutex_);

    if (!flags.Test(SCAN_APPEND))
        result.clear();

    for (MountPoint* mountPoint : ea::reverse(mountPoints_))
    {
        if (mountPoint->AcceptsScheme(scheme))
            mountPoint->Scan(result, pathName, filter, flags | SCAN_APPEND);
    }
}

void VirtualFileSystem::Scan(ea::vector<ea::string>& result, const FileIdentifier& pathName, const ea::string& filter,
    ScanFlags flags) const
{
    Scan(result, pathName.scheme_, pathName.fileName_, filter, flags);
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

MountPointGuard::MountPointGuard(MountPoint* mountPoint)
    : mountPoint_(mountPoint)
{
    if (mountPoint_)
    {
        auto vfs = mountPoint_->GetContext()->GetSubsystem<VirtualFileSystem>();
        vfs->Mount(mountPoint_);
    }
}

MountPointGuard::~MountPointGuard()
{
    Release();
}

MountPointGuard::MountPointGuard(MountPointGuard&& other) noexcept
{
    *this = ea::move(other);
}

MountPointGuard& MountPointGuard::operator=(MountPointGuard&& other) noexcept
{
    Release();
    mountPoint_ = ea::move(other.mountPoint_);
    return *this;
}

void MountPointGuard::Release()
{
    if (mountPoint_)
    {
        auto vfs = mountPoint_->GetContext()->GetSubsystem<VirtualFileSystem>();
        vfs->Unmount(mountPoint_);
        mountPoint_ = nullptr;
    }
}

} // namespace Urho3D
