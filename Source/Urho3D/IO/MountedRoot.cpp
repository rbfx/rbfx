// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/IO/MountedRoot.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/FileSystem.h"

#include <utility>

namespace Urho3D
{

MountedRoot::MountedRoot(Context* context)
    : MountPoint(context)
{
}

bool MountedRoot::AcceptsScheme(const ea::string& scheme) const
{
    return scheme.comparei("file") == 0;
}

bool MountedRoot::Exists(const FileIdentifier& fileName) const
{
    if (!AcceptsScheme(fileName.scheme_))
        return false;

    auto fileSystem = GetSubsystem<FileSystem>();
    return IsAbsolutePath(fileName.fileName_) && fileSystem->Exists(fileName.fileName_);
}

AbstractFilePtr MountedRoot::OpenFile(const FileIdentifier& fileName, FileMode mode)
{
    if (!AcceptsScheme(fileName.scheme_))
        return nullptr;

    if (!IsAbsolutePath(fileName.fileName_))
        return nullptr;

    const auto fileSystem = context_->GetSubsystem<FileSystem>();

    const bool needRead = mode == FILE_READ || mode == FILE_READWRITE;
    const bool needWrite = mode == FILE_WRITE || mode == FILE_READWRITE;

    if (needRead && !fileSystem->FileExists(fileName.fileName_))
        return nullptr;

    if (needWrite)
    {
        const ea::string directory = GetPath(fileName.fileName_);
        if (!fileSystem->DirExists(directory))
        {
            if (!fileSystem->CreateDir(directory))
                return nullptr;
        }
    }

    auto file = MakeShared<File>(context_, fileName.fileName_, mode);
    if (!file->IsOpen())
        return nullptr;

    file->SetName(fileName.ToUri());
    return file;
}

const ea::string& MountedRoot::GetName() const
{
    static const ea::string name = "file://";
    return name;
}

ea::string MountedRoot::GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const
{
    if (!AcceptsScheme(fileName.scheme_))
        return EMPTY_STRING;

    const auto* fileSystem = GetSubsystem<FileSystem>();
    if (IsAbsolutePath(fileName.fileName_) && fileSystem->FileExists(fileName.fileName_))
        return fileName.fileName_;

    return EMPTY_STRING;
}

FileIdentifier MountedRoot::GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const
{
    return {"file", absoluteFileName};
}

void MountedRoot::Scan(
    ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, ScanFlags flags) const
{
    // Disable Scan for the root until we add scheme filtering.
}

} // namespace Urho3D
