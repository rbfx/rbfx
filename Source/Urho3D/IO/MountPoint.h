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

#include "../Core/Object.h"
#include "../IO/AbstractFile.h"
#include "../IO/FileIdentifier.h"

namespace Urho3D
{

/// Access to engine file system mount point.
class URHO3D_API MountPoint : public Object
{
    URHO3D_OBJECT(MountPoint, Object);

public:
    /// Construct.
    explicit MountPoint(Context* context);
    /// Destruct.
    ~MountPoint() override;

    /// Checks if mount point accepts scheme.
    virtual bool AcceptsScheme(const ea::string& scheme) const = 0;

    /// Check if a file exists within the mount point.
    /// The file name may be be case-insensitive on Windows and case-sensitive on other platforms.
    virtual bool Exists(const FileIdentifier& fileName) const = 0;

    /// Open file in a virtual file system. Returns null if file not found.
    /// The file name may be be case-insensitive on Windows and case-sensitive on other platforms.
    virtual AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) = 0;

    /// Get full path to a file if it exists in a mount point.
    virtual ea::string GetFileName(const FileIdentifier& fileName) const = 0;
};

} // namespace Urho3D
