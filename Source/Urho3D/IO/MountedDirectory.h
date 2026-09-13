// Copyright (c) 2022-2023 the Urho3D project.
// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/FlagSet.h"
#include "Urho3D/IO/FileWatcher.h"
#include "Urho3D/IO/MountPoint.h"

namespace Urho3D
{

/// Stores files of a directory tree sequentially for convenient access.
class URHO3D_API MountedDirectory : public WatchableMountPoint
{
    URHO3D_OBJECT(MountedDirectory, WatchableMountPoint);

public:
    /// Construct and open.
    MountedDirectory(Context* context, const ea::string& directory, ea::string scheme = EMPTY_STRING);
    /// Destruct.
    ~MountedDirectory() override;

    /// Implement MountPoint.
    /// @{
    bool AcceptsScheme(const ea::string& scheme) const override;
    bool Exists(const FileIdentifier& fileName) const override;
    AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) override;
    ea::optional<FileTime> GetLastModifiedTime(
        const FileIdentifier& fileName, bool creationIsModification) const override;

    const ea::string& GetName() const override { return name_; }

    ea::string GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const override;
    FileIdentifier GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const override;

    void Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter,
        ScanFlags flags) const override;
    /// @}

    /// Get mounted directory path.
    const ea::string& GetDirectory() const { return directory_; }

protected:
    ea::string SanitizeDirName(const ea::string& name) const;

    /// Implement WatchableMountPoint.
    /// @{
    void StartWatching() override;
    void StopWatching() override;
    /// @}

private:
    void ProcessUpdates();

private:
    /// Expected file locator scheme.
    const ea::string scheme_;
    /// Target directory.
    const ea::string directory_;
    /// Name of the mount point.
    const ea::string name_;
    /// File watcher for resource directory, if automatic reloading enabled.
    SharedPtr<FileWatcher> fileWatcher_;
};

} // namespace Urho3D
