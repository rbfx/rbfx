// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../IO/FileSystem.h"
#include "../Utility/FileSystemReflection.h"

#include <EASTL/sort.h>
#include <EASTL/tuple.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

bool CompareEntries(const ea::string& lhs, const ea::string& rhs, bool filesFirst)
{
    const auto compare = [filesFirst](char lhs, char rhs)
    {
        return ea::make_tuple((lhs != '/') != filesFirst, lhs)
            < ea::make_tuple((rhs != '/') != filesFirst, rhs);
    };
    return ea::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), compare);
}

}

bool FileSystemEntry::ComparePathFilesFirst(const ea::string& lhs, const ea::string& rhs)
{
    return CompareEntries(lhs, rhs, true);
}

bool FileSystemEntry::ComparePathDirectoriesFirst(const ea::string& lhs, const ea::string& rhs)
{
    return CompareEntries(lhs, rhs, false);
}

bool FileSystemEntry::CompareFilesFirst(const FileSystemEntry& lhs, const FileSystemEntry& rhs)
{
    return ComparePathFilesFirst(lhs.resourceName_, rhs.resourceName_);
}

bool FileSystemEntry::CompareDirectoriesFirst(const FileSystemEntry& lhs, const FileSystemEntry& rhs)
{
    return ComparePathDirectoriesFirst(lhs.resourceName_, rhs.resourceName_);
}

FileSystemReflection::FileSystemReflection(Context* context, const StringVector& directories)
    : Object(context)
    , directories_(directories)
    , fileWatcher_(MakeShared<MultiFileWatcher>(context_))
{
    for (const ea::string& dir : directories_)
        fileWatcher_->StartWatching(dir, true);
}

FileSystemReflection::~FileSystemReflection()
{
}

void FileSystemReflection::Update()
{
    FileChange change;
    while (fileWatcher_->GetNextChange(change))
    {
        updatedResources_.insert(change.fileName_);
        if (change.kind_ == FILECHANGE_ADDED || change.kind_ == FILECHANGE_REMOVED || change.kind_ == FILECHANGE_RENAMED)
            treeDirty_ = true;
    }

    if (treeDirty_)
        UpdateEntryTree();

    for (const ea::string& resourceName : updatedResources_)
    {
        if (const FileSystemEntry* entry = FindEntry(resourceName))
            OnResourceUpdated(this, *entry);
    }
    updatedResources_.clear();
}

const FileSystemEntry* FileSystemReflection::FindEntry(const ea::string& name) const
{
    const auto iter = index_.find(name);
    return iter != index_.end() ? iter->second : nullptr;
}

void FileSystemReflection::UpdateEntryTree()
{
    ea::vector<FileSystemEntry> entries;
    unsigned index = 0;
    for (const ea::string& resourceDir : directories_)
        ScanRootDirectory(resourceDir, entries, index++);

    CollectAddedFiles(root_, entries);

    ea::sort(entries.begin(), entries.end(), FileSystemEntry::CompareDirectoriesFirst);
    const ea::vector<FileSystemEntry> mergedEntries = MergeEntries(entries);

    FileSystemEntry root;
    root.owner_ = this;
    for (const FileSystemEntry& entry : mergedEntries)
        AppendEntry(root, entry);

    root_ = ea::move(root);
    root_.FillParents();
    treeDirty_ = false;

    index_.clear();
    root_.ForEach([&](const FileSystemEntry& entry)
    {
        index_[entry.resourceName_] = &entry;
    });

    OnListUpdated(this);
}

void FileSystemReflection::ScanRootDirectory(const ea::string& resourceDir,
    ea::vector<FileSystemEntry>& entries, unsigned index)
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector files;
    fs->ScanDir(files, resourceDir, "", SCAN_FILES | SCAN_DIRS | SCAN_RECURSIVE);
    for (const ea::string& resourceName : files)
    {
        if (resourceName.ends_with(".") || resourceName.ends_with(".."))
            continue;

        FileSystemEntry entry;
        entry.owner_ = this;
        entry.absolutePath_ = resourceDir + resourceName;
        entry.resourceName_ = resourceName;
        entry.isFile_ = fs->FileExists(entry.absolutePath_);
        entry.isDirectory_ = fs->DirExists(entry.absolutePath_);
        entry.directoryIndex_ = index;

        if (entry.isFile_ || entry.isDirectory_)
            entries.push_back(entry);
    }
}

void FileSystemReflection::CollectAddedFiles(const FileSystemEntry& oldRoot,
    const ea::vector<FileSystemEntry>& entries)
{
    ea::unordered_set<ea::string> previousResources;
    oldRoot.ForEach([&](const FileSystemEntry& entry)
    {
        previousResources.insert(entry.resourceName_);
    });

    for (const FileSystemEntry& entry : entries)
    {
        if (entry.isFile_ && !previousResources.contains(entry.resourceName_))
            updatedResources_.insert(entry.resourceName_);
    }
}

ea::vector<FileSystemEntry> FileSystemReflection::MergeEntries(ea::vector<FileSystemEntry>& entries) const
{
    ea::vector<FileSystemEntry> mergedEntries;
    for (const FileSystemEntry& entry : entries)
    {
        if (!mergedEntries.empty() && mergedEntries.back().resourceName_ == entry.resourceName_)
        {
            auto& exitsingEntry = mergedEntries.back();
            if (entry.isFile_ && exitsingEntry.isFile_)
                exitsingEntry.isFileAmbiguous_ = true;
            if (entry.isFile_)
                exitsingEntry.isFile_ = true;
            if (entry.isDirectory_)
                exitsingEntry.isDirectory_ = true;
            if (exitsingEntry.directoryIndex_ > entry.directoryIndex_)
            {
                exitsingEntry.directoryIndex_ = entry.directoryIndex_;
                exitsingEntry.absolutePath_ = entry.absolutePath_;
            }
        }
        else
            mergedEntries.push_back(entry);
    }
    return mergedEntries;
}

void FileSystemReflection::AppendEntry(FileSystemEntry& root, const FileSystemEntry& entry)
{
    const auto pathParts = entry.resourceName_.split('/');

    FileSystemEntry* currentEntry = &root;
    for (unsigned i = 0; i < pathParts.size(); ++i)
    {
        const ea::string localName = pathParts[i];
        if (currentEntry->children_.empty() || currentEntry->children_.back().localName_ != localName)
        {
            auto& entryCopy = currentEntry->children_.emplace_back(entry);
            entryCopy.localName_ = localName;
        }

        currentEntry = &currentEntry->children_.back();
    }
}

}
