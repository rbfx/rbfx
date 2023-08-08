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

#include "Urho3D/Core/Object.h"
#include "Urho3D/IO/AbstractFile.h"
#include "Urho3D/IO/FileIdentifier.h"
#include "Urho3D/IO/FileSystem.h"

#include <EASTL/optional.h>

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

    /// Return modification time, or 0 if not supported.
    /// Return nullopt if file does not exist.
    virtual ea::optional<FileTime> GetLastModifiedTime(
        const FileIdentifier& fileName, bool creationIsModification) const;

    /// Returns human-readable name of the mount point.
    virtual const ea::string& GetName() const = 0;

    /// Return absolute file name for *existing* identifier in this mount point, if supported.
    virtual ea::string GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const;

    /// Return identifier in this mount point for absolute file name, if supported.
    /// Works even if the file does not exist.
    virtual FileIdentifier GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const;

    /// Enable or disable FileChanged events.
    virtual void SetWatching(bool enable);

    /// Returns true if FileChanged events are enabled.
    virtual bool IsWatching() const;

    /// Enumerate objects in the mount point. Only files enumeration is guaranteed to be supported.
    virtual void Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter,
        ScanFlags flags) const = 0;
};

/// Base implementation of watchable mount point.
class URHO3D_API WatchableMountPoint : public MountPoint
{
    URHO3D_OBJECT(WatchableMountPoint, MountPoint);

public:
    explicit WatchableMountPoint(Context* context);
    ~WatchableMountPoint() override;

    /// Implement MountPoint.
    /// @{
    void SetWatching(bool enable) override;
    bool IsWatching() const override;
    /// @}

protected:
    /// Start file watcher.
    virtual void StartWatching() = 0;
    /// Stop file watcher.
    virtual void StopWatching() = 0;

private:
    bool isWatching_{};
};

} // namespace Urho3D
