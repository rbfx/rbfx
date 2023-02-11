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


#include "MountedDirectory.h"

#include "File.h"
#include "FileSystem.h"
#include "Log.h"
#include "Urho3D/Core/Context.h"

namespace Urho3D
{

MountedDirectory::MountedDirectory(Context* context)
    : MountPoint(context)
{
}

MountedDirectory::MountedDirectory(
    Context* context, const ea::string& directory, ea::string scheme)
    : MountPoint(context)
    , scheme_(std::move(scheme))
    , directory_(SanitizeDirName(directory))
{
}

MountedDirectory::~MountedDirectory()
{
}

ea::string MountedDirectory::SanitizeDirName(const ea::string& name) const
{
    ea::string fixedPath = AddTrailingSlash(name);
    if (!IsAbsolutePath(fixedPath))
        fixedPath = GetSubsystem<FileSystem>()->GetCurrentDir() + fixedPath;

    // Sanitize away /./ construct
    fixedPath.replace("/./", "/");

    fixedPath.trim();
    return fixedPath;
}

/// Checks if mount point accepts scheme.
bool MountedDirectory::AcceptsScheme(const ea::string& scheme) const
{
    return scheme.comparei(scheme_) == 0;
}

/// Check if a file exists within the mount point.
bool MountedDirectory::Exists(const FileIdentifier& fileName) const
{
    // File system directory only reacts on specific scheme.
    if (!AcceptsScheme(fileName.scheme_))
        return false;

    const auto fileSystem = context_->GetSubsystem<FileSystem>();

    return fileSystem->FileExists(directory_ + fileName.fileName_);
}

/// Open file in a virtual file system. Returns null if file not found.
AbstractFilePtr MountedDirectory::OpenFile(const FileIdentifier& fileName, FileMode mode)
{
    // File system directory only reacts on specific scheme.
    if (!AcceptsScheme(fileName.scheme_))
        return AbstractFilePtr();

    const auto fileSystem = context_->GetSubsystem<FileSystem>();
    auto fullPath = directory_ + fileName.fileName_;

    if (mode == FILE_WRITE)
    {
        const auto directory = GetPath(fullPath);
        if (!fileSystem->DirExists(directory))
            if (!fileSystem->CreateDir(directory))
                return AbstractFilePtr();
    }

    if (mode == FILE_READ && !fileSystem->FileExists(fullPath))
        return AbstractFilePtr();

    // Construct the file first with full path, then rename it to not contain the resource path,
    // so that the file's sanitized name can be used in further GetFile() calls (for example over the network)
    auto file(MakeShared<File>(context_, fullPath, mode));
    file->SetName(fileName.fileName_);
    if (!file->IsOpen())
        return AbstractFilePtr();

    return file;
}

/// Return full absolute file name of the file if possible, or empty if not found.
ea::string MountedDirectory::GetFileName(const FileIdentifier& fileName) const
{
    // File system directory only reacts on specific scheme.
    if (fileName.scheme_ != scheme_)
        return EMPTY_STRING;

    if (Exists(fileName))
        return directory_ + fileName.fileName_;

    return EMPTY_STRING;
}

} // namespace Urho3D
