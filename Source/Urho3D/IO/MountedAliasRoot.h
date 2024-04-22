// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/IO/MountPoint.h"

#include <EASTL/tuple.h>

namespace Urho3D
{

/// Mount point that provides named aliases to other mount points.
class URHO3D_API MountedAliasRoot : public MountPoint
{
    URHO3D_OBJECT(MountedAliasRoot, MountPoint);

public:
    explicit MountedAliasRoot(Context* context);
    ~MountedAliasRoot() override;

    /// Add alias to another mount point.
    void AddAlias(const ea::string& path, const ea::string& scheme, MountPoint* mountPoint);
    /// Remove all aliases to the mount point.
    void RemoveAliases(MountPoint* mountPoint);
    /// Find mount point and its alias for the specified file name.
    /// Returns mount point, alias and recommended scheme.
    ea::tuple<MountPoint*, ea::string_view, ea::string_view> FindMountPoint(ea::string_view fileName) const;

    /// Implement MountPoint.
    /// @{
    bool AcceptsScheme(const ea::string& scheme) const override;

    bool Exists(const FileIdentifier& fileName) const override;

    AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) override;

    ea::optional<FileTime> GetLastModifiedTime(
        const FileIdentifier& fileName, bool creationIsModification) const override;

    const ea::string& GetName() const override;

    ea::string GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const override;

    FileIdentifier GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const override;

    void Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter,
        ScanFlags flags) const override;
    /// @}

private:
    ea::unordered_map<ea::string, ea::pair<WeakPtr<MountPoint>, ea::string>> aliases_;
};

} // namespace Urho3D
