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


#include "ResourceFolder.h"

#include "File.h"
#include "FileSystem.h"
#include "Log.h"
#include "Urho3D/Core/Context.h"

namespace Urho3D
{

ResourceFolder::ResourceFolder(Context* context)
    : MountPoint(context)
{
}

ResourceFolder::ResourceFolder(Context* context, const ea::string& folder)
    : MountPoint(context)
{
    folder_ = SanitateResourceDirName(folder);
    fileWatcher_ = MakeShared<FileWatcher>(context);

    auto* fileSystem = GetSubsystem<FileSystem>();
    if (!fileSystem || !fileSystem->DirExists(folder_))
    {
        URHO3D_LOGERROR("Could not open directory " + folder_);
        return;
    }

    fileWatcher_->StartWatching(folder_, true);
}

ResourceFolder::~ResourceFolder()
{
    fileWatcher_->StopWatching();
}

ea::string ResourceFolder::SanitateResourceDirName(const ea::string& name) const
{
    ea::string fixedPath = AddTrailingSlash(name);
    if (!IsAbsolutePath(fixedPath))
        fixedPath = GetSubsystem<FileSystem>()->GetCurrentDir() + fixedPath;

    // Sanitate away /./ construct
    fixedPath.replace("/./", "/");

    fixedPath.trim();
    return fixedPath;
}


/// Check if a file exists within the mount point.
bool ResourceFolder::Exists(const ea::string& scheme, const ea::string& fileName) const
{
    if (!scheme.empty())
        return false;

    auto fileSystem = context_->GetSubsystem<FileSystem>();

    return fileSystem->FileExists(folder_ + fileName);
}

/// Open file in a virtual file system. Returns null if file not found.
AbstractFilePtr ResourceFolder::OpenFile(const ea::string& scheme, const ea::string& fileName, FileMode mode)
{
    auto fileSystem = context_->GetSubsystem<FileSystem>();
    auto fullPath = folder_ + fileName;

    if (!fileSystem->FileExists(fullPath))
        return {};

    // Construct the file first with full path, then rename it to not contain the resource path,
    // so that the file's sanitatedName can be used in further GetFile() calls (for example over the network)
    SharedPtr<File> file(MakeShared<File>(context_, fullPath));
    file->SetName(fileName);
    return AbstractFilePtr(file, file);
}

} // namespace Urho3D
