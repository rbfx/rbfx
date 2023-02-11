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

    /// Mount real folder into virtual file system.
    void MountDir(const ea::string& path);
    /// Mount real folder into virtual file system under the scheme.
    void MountDir(const ea::string& scheme, const ea::string& path);
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
    /// Return full absolute file name of the file if possible, or empty if not found.
    ea::string GetFileName(const FileIdentifier& name);

private:
    /// Mutex for thread-safe access to the mount points.
    mutable Mutex mountMutex_;
    /// File system mount points. It is expected to have small number of mount points.
    ea::vector<SharedPtr<MountPoint>> mountPoints_;
};

}
