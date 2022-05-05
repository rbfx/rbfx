//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Precompiled.h"

#include "../IO/FileSystem.h"
#include "../Utility/FileSystemReflection.h"

#include <EASTL/sort.h>
#include <EASTL/tuple.h>

#include "../DebugNew.h"

namespace Urho3D
{

bool FileSystemEntry::operator<(const FileSystemEntry& rhs) const
{
    //return ea::tie(depth_, resourceName_) < ea::tie(rhs.depth_, rhs.resourceName_);
    //return resourceName_ < rhs.resourceName_;

    const auto compare = [](char lhs, char rhs)
    {
        return ea::make_tuple(lhs != '/', lhs) < ea::make_tuple(rhs != '/', rhs);
    };
    return ea::lexicographical_compare(resourceName_.begin(), resourceName_.end(),
        rhs.resourceName_.begin(), rhs.resourceName_.end(), compare);
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
    for (const ea::string& resourceDir : directories_)
        ScanRootDirectory(resourceDir, entries);

    CollectAddedFiles(root_, entries);

    ea::sort(entries.begin(), entries.end());
    const ea::vector<FileSystemEntry> mergedEntries = MergeEntries(entries);

    FileSystemEntry root;
    for (const FileSystemEntry& entry : mergedEntries)
        AppendEntry(root, entry);

    root_ = ea::move(root);
    treeDirty_ = false;

    index_.clear();
    root_.ForEach([&](const FileSystemEntry& entry)
    {
        index_[entry.resourceName_] = &entry;
    });
}

void FileSystemReflection::ScanRootDirectory(const ea::string& resourceDir, ea::vector<FileSystemEntry>& entries) const
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector files;
    fs->ScanDir(files, resourceDir, "", SCAN_FILES | SCAN_DIRS, true);
    for (const ea::string& resourceName : files)
    {
        if (resourceName.ends_with(".") || resourceName.ends_with(".."))
            continue;

        FileSystemEntry entry;
        entry.absolutePath_ = resourceDir + resourceName;
        entry.resourceName_ = resourceName;
        entry.depth_ = ea::count(resourceName.begin(), resourceName.end(), '/');
        entry.isFile_ = fs->FileExists(entry.absolutePath_);
        entry.isDirectory_ = fs->DirExists(entry.absolutePath_);

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
