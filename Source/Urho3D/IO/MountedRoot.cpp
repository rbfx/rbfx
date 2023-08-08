//
// Copyright (c) 2023-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

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
    if (IsAbsolutePath(fileName.fileName_))
    {
        if (fileSystem->FileExists(fileName.fileName_))
            return fileName.fileName_;
    }

    return EMPTY_STRING;
}

ea::string MountedRoot::GetWritableAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const
{
    if (!AcceptsScheme(fileName.scheme_))
        return EMPTY_STRING;

    if (IsAbsolutePath(fileName.fileName_))
    {
        return fileName.fileName_;
    }

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
