//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include "../Core/Object.h"
#include "../IO/AbstractFile.h"
#include "../IO/MountPoint.h"

namespace Urho3D
{
/// Subsystem for virtual file system.
class URHO3D_API VirtualFileSystem : public Object
{
    URHO3D_OBJECT(VirtualFileSystem, Object)

public:
    /// Construct.
    explicit VirtualFileSystem(Context* context);
    /// Destruct.
    ~VirtualFileSystem() override;

    /// Mount file system root as file:// scheme.
    void MountRoot();
    /// Mount real folder into virtual file system.
    void MountDir(const ea::string& path);
    /// Mount real folder into virtual file system under the scheme.
    void MountDir(const ea::string& scheme, const ea::string& path);
    /// Mount subfolders and pak files from real folder into virtual file system.
    void AutomountDir(const ea::string& path);
    ///Mount subfolders and pak files from real folder into virtual file system under the scheme.
    void AutomountDir(const ea::string& scheme, const ea::string& path);
    /// Mount package file into virtual file system.
    void MountPackageFile(const ea::string& path);
    /// Mount virtual or real folder into virtual file system.
    void Mount(MountPoint* mountPoint);
    /// Remove mount point from the virtual file system.
    void Unmount(MountPoint* mountPoint);
    /// Remove all mount points.
    void UnmountAll();
    /// Get number of mount points.
    unsigned NumMountPoints() const { return mountPoints_.size(); }
    /// Get mount point by index.
    MountPoint* GetMountPoint(unsigned index) const;

    /// Mount all existing packages for each combination of prefix path and relative path.
    void MountExistingPackages(const StringVector& prefixPaths, const StringVector& relativePaths);
    /// Mount all existing directories and packages for each combination of prefix path and relative path.
    /// Package is preferred over directory if both exist.
    void MountExistingDirectoriesOrPackages(const StringVector& prefixPaths, const StringVector& relativePaths);

    /// Check if a file exists in the virtual file system.
    bool Exists(const FileIdentifier& fileName) const;
    /// Open file in the virtual file system. Returns null if file not found.
    AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) const;
    /// Return modification time. Return 0 if not supported or file doesn't exist.
    FileTime GetLastModifiedTime(const FileIdentifier& fileName, bool creationIsModification) const;
    /// Return absolute file name for *existing* identifier in this mount point, if supported.
    ea::string GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName, FileMode mode = FILE_READ) const;
    /// Return canonical file identifier, if possible.
    FileIdentifier GetCanonicalIdentifier(const FileIdentifier& fileName) const;
    /// Works even if the file does not exist.
    FileIdentifier GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const;
    /// Return relative file name of the file, or empty if not found.
    FileIdentifier GetIdentifierFromAbsoluteName(const ea::string& scheme, const ea::string& absoluteFileName) const;

    /// Enable or disable file watchers.
    void SetWatching(bool enable);

    /// Returns true if the file watchers are enabled.
    bool IsWatching() const { return isWatching_; }

    /// Scan for specified files.
    void Scan(ea::vector<ea::string>& result, const ea::string& scheme, const ea::string& pathName,
        const ea::string& filter, ScanFlags flags) const;
    void Scan(ea::vector<ea::string>& result, const FileIdentifier& pathName, const ea::string& filter,
        ScanFlags flags) const;

private:
    /// Mutex for thread-safe access to the mount points.
    mutable Mutex mountMutex_;
    /// File system mount points. It is expected to have small number of mount points.
    ea::vector<SharedPtr<MountPoint>> mountPoints_;
    /// Are file watchers enabled.
    bool isWatching_{};
};

/// Helper class to mount and unmount an object automatically.
class URHO3D_API MountPointGuard : public MovableNonCopyable
{
public:
    explicit MountPointGuard(MountPoint* mountPoint);
    ~MountPointGuard();

    MountPointGuard(MountPointGuard&& other) noexcept;
    MountPointGuard& operator=(MountPointGuard&& other) noexcept;

    void Release();
    MountPoint* Get() const { return mountPoint_; }

    template <class T>
    explicit MountPointGuard(const SharedPtr<T>& mountPoint)
        : MountPointGuard(mountPoint.Get())
    {
    }

private:
    SharedPtr<MountPoint> mountPoint_;
};

}
