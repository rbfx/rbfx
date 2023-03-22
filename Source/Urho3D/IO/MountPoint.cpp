//
// Copyright (c) 2022-2023 the Urho3D project.
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
