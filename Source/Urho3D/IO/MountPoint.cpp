// Copyright (c) 2022-2023 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/IO/MountPoint.h"

#include "Urho3D/IO/FileSystem.h"

namespace Urho3D
{

MountPoint::MountPoint(Context* context)
    : Object(context)
{
}

MountPoint::~MountPoint() = default;

ea::optional<FileTime> MountPoint::GetLastModifiedTime(
    const FileIdentifier& fileName, bool creationIsModification) const
{
    if (Exists(fileName))
        return 0u;
    else
        return ea::nullopt;
}

ea::string MountPoint::GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const
{
    return EMPTY_STRING;
}

FileIdentifier MountPoint::GetIdentifierFromAbsoluteName(const ea::string& fileFullPath) const
{
    return FileIdentifier::Empty;
}

void MountPoint::SetWatching(bool enable)
{
}

bool MountPoint::IsWatching() const
{
    return false;
}

WatchableMountPoint::WatchableMountPoint(Context* context)
    : MountPoint(context)
{
}

WatchableMountPoint::~WatchableMountPoint() = default;

void WatchableMountPoint::SetWatching(bool enable)
{
    if (isWatching_ != enable)
    {
        isWatching_ = enable;
        if (isWatching_)
            StartWatching();
        else
            StopWatching();
    }
}

bool WatchableMountPoint::IsWatching() const
{
    return isWatching_;
}

} // namespace Urho3D
