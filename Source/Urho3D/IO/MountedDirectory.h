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

#pragma once

#include "FileWatcher.h"
#include "../IO/MountPoint.h"
#include "../Container/FlagSet.h"

namespace Urho3D
{

/// Stores files of a directory tree sequentially for convenient access.
class URHO3D_API MountedDirectory : public MountPoint
{
    URHO3D_OBJECT(MountedDirectory, MountPoint);

public:
    /// Construct.
    explicit MountedDirectory(Context* context);
    /// Construct and open.
    MountedDirectory(Context* context, const ea::string& directory, ea::string scheme = EMPTY_STRING);
    /// Destruct.
    ~MountedDirectory() override;

    /// Checks if mount point accepts scheme.
    bool AcceptsScheme(const ea::string& scheme) const override;

    /// Check if a file exists within the mount point.
    bool Exists(const FileIdentifier& fileName) const override;

    /// Open file in a virtual file system. Returns null if file not found.
    AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) override;

    /// Return full absolute file name of the file if possible, or empty if not found.
    ea::string GetFileName(const FileIdentifier& fileName) const override;

protected:
    ea::string SanitizeDirName(const ea::string& name) const;

private:
    /// Expected file locator scheme.
    ea::string scheme_;
    /// Target directory.
    ea::string directory_;
};

} // namespace Urho3D
