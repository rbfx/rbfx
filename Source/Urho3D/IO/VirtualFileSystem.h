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

#include "../Core/NonCopyable.h"
#include "../Core/Object.h"
#include "../IO/AbstractFile.h"
#include "../Utility/AssetPipeline.h"

#include <EASTL/hash_set.h>
#include <EASTL/list.h>

namespace Urho3D
{
/// Subsystem for virtual file system.
class URHO3D_API VirtualFileSystem : public Object
{
    URHO3D_OBJECT(VirtualFileSystem, Object);

public:
    /// Construct.
    explicit VirtualFileSystem(Context* context);
    /// Destruct.
    ~VirtualFileSystem() override;

    /// Mount virtual or real folder into file system.
    void Mount(MountPoint* mountPoint);
    /// Remove mount point from file system.
    void Unmount(MountPoint* mountPoint);
    /// Remove all mount points.
    void UnmountAll();
    /// Open file in a virtual file system. Returns null if file not found.
    AbstractFilePtr OpenFile(const ea::string& scheme, const ea::string& name, FileMode mode) const;
    /// Open file in a virtual file system. Returns null if file not found.
    AbstractFilePtr OpenFile(const ea::string& schemeAndName, FileMode mode) const;
    /// Check if a file exists in the virtual file system.
    bool Exists(const ea::string& schemeAndName) const;
    /// Check if a file exists in the virtual file system.
    bool Exists(const ea::string& scheme, const ea::string& name) const;

    void Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, unsigned flags, bool recursive) const;

    ea::string GetFileName(const ea::string& name);

    void SetWatching(bool enabled);
    bool IsWatching() const { return watching_; }

private:
    /// Mutex for thread-safe access to the mount points.
    mutable Mutex mountMutex_;
    /// File system mount points. It is expected to have small number of mount points.
    ea::vector<SharedPtr<MountPoint>> mountPoints_;

    bool watching_{};
};

}
