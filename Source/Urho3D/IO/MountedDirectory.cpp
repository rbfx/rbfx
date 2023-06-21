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

#include "Urho3D/IO/MountedDirectory.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/CoreEvents.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Resource/ResourceEvents.h"

namespace Urho3D
{

MountedDirectory::MountedDirectory(Context* context, const ea::string& directory, ea::string scheme)
    : WatchableMountPoint(context)
    , scheme_(std::move(scheme))
    , directory_(SanitizeDirName(directory))
    , name_(scheme_.empty() ? directory_ : (scheme_ + "://" + directory_))
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

void MountedDirectory::StartWatching()
{
    if (!fileWatcher_)
        fileWatcher_ = MakeShared<FileWatcher>(context_);

    fileWatcher_->StartWatching(directory_, true);

    // Subscribe BeginFrame for handling directory watcher
    SubscribeToEvent(E_BEGINFRAME, &MountedDirectory::ProcessUpdates);
}

void MountedDirectory::StopWatching()
{
    if (fileWatcher_)
        fileWatcher_->StopWatching();

    UnsubscribeFromEvent(E_BEGINFRAME);
}

void MountedDirectory::ProcessUpdates()
{
    if (!fileWatcher_)
        return;

    FileChange change;
    while (fileWatcher_->GetNextChange(change))
    {
        using namespace FileChanged;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_FILENAME] = fileWatcher_->GetPath() + change.fileName_;
        eventData[P_RESOURCENAME] = FileIdentifier{scheme_, change.fileName_}.ToUri();
        SendEvent(E_FILECHANGED, eventData);
    }
}

bool MountedDirectory::AcceptsScheme(const ea::string& scheme) const
{
    return scheme.comparei(scheme_) == 0;
}

bool MountedDirectory::Exists(const FileIdentifier& fileName) const
{
    // File system directory only reacts on specific scheme.
    if (!AcceptsScheme(fileName.scheme_))
        return false;

    const auto fileSystem = context_->GetSubsystem<FileSystem>();

    return fileSystem->FileExists(directory_ + fileName.fileName_);
}

AbstractFilePtr MountedDirectory::OpenFile(const FileIdentifier& fileName, FileMode mode)
{
    // File system directory only reacts on specific scheme.
    if (!AcceptsScheme(fileName.scheme_))
        return nullptr;

    const auto fileSystem = context_->GetSubsystem<FileSystem>();

    const bool needRead = mode == FILE_READ || mode == FILE_READWRITE;
    const bool needWrite = mode == FILE_WRITE || mode == FILE_READWRITE;
    const ea::string fullPath = directory_ + fileName.fileName_;

    if (needRead && !fileSystem->FileExists(fullPath))
        return nullptr;

    if (needWrite)
    {
        const ea::string directory = GetPath(fullPath);
        if (!fileSystem->DirExists(directory))
        {
            if (!fileSystem->CreateDir(directory))
                return nullptr;
        }
    }

    auto file = MakeShared<File>(context_, fullPath, mode);
    if (!file->IsOpen())
        return nullptr;

    file->SetName(fileName.ToUri());
    return file;
}

ea::optional<FileTime> MountedDirectory::GetLastModifiedTime(
    const FileIdentifier& fileName, bool creationIsModification) const
{
    if (!Exists(fileName))
        return ea::nullopt;

    auto fileSystem = context_->GetSubsystem<FileSystem>();
    const ea::string fullPath = directory_ + fileName.fileName_;
    return fileSystem->GetLastModifiedTime(fullPath, creationIsModification);
}

ea::string MountedDirectory::GetAbsoluteNameFromIdentifier(const FileIdentifier& fileName) const
{
    if (Exists(fileName))
        return directory_ + fileName.fileName_;

    return EMPTY_STRING;
}

FileIdentifier MountedDirectory::GetIdentifierFromAbsoluteName(const ea::string& absoluteFileName) const
{
    if (absoluteFileName.starts_with(directory_))
        return FileIdentifier{scheme_, absoluteFileName.substr(directory_.length())};

    return FileIdentifier::Empty;
}

void MountedDirectory::Scan(
    ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, ScanFlags flags) const
{
    const auto fileSystem = context_->GetSubsystem<FileSystem>();
    fileSystem->ScanDir(result, directory_ + pathName, filter, flags);
}

} // namespace Urho3D
