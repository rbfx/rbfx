// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Object.h"
#include "../Core/Signal.h"
#include "../IO/MultiFileWatcher.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

class FileSystemReflection;

/// Description of file system entry (file or directory) with hierarchy information.
struct URHO3D_API FileSystemEntry
{
    FileSystemReflection* owner_{};
    FileSystemEntry* parent_{};

    ea::string resourceName_;
    ea::string absolutePath_;
    bool isDirectory_{};
    bool isFile_{};
    unsigned directoryIndex_{};

    ea::string localName_;
    bool isFileAmbiguous_{};

    ea::vector<FileSystemEntry> children_;

    static bool ComparePathFilesFirst(const ea::string& lhs, const ea::string& rhs);
    static bool ComparePathDirectoriesFirst(const ea::string& lhs, const ea::string& rhs);

    static bool CompareFilesFirst(const FileSystemEntry& lhs, const FileSystemEntry& rhs);
    static bool CompareDirectoriesFirst(const FileSystemEntry& lhs, const FileSystemEntry& rhs);

    template <class T>
    void ForEach(const T& callback) const
    {
        callback(*this);
        for (const FileSystemEntry& child : children_)
            child.ForEach(callback);
    }

    void FillParents()
    {
        for (FileSystemEntry& child : children_)
        {
            child.parent_ = this;
            child.FillParents();
        }
    }

    const FileSystemEntry* FindChild(const ea::string& name) const
    {
        for (const FileSystemEntry& child : children_)
        {
            if (child.localName_ == name)
                return &child;
        }
        return nullptr;
    }
};

/// Utility class that watches all files in ResourceCache.
class URHO3D_API FileSystemReflection : public Object
{
    URHO3D_OBJECT(FileSystemReflection, Object);

public:
    /// Called whenever resource file is modified or new resource is added.
    /// Can be used to invalidate whatever per-resource caches there are.
    Signal<void(const FileSystemEntry& entry)> OnResourceUpdated;
    Signal<void()> OnListUpdated;

    FileSystemReflection(Context* context, const StringVector& directories);
    ~FileSystemReflection() override;

    void Update();

    const FileSystemEntry& GetRoot() const { return root_; };
    const FileSystemEntry* FindEntry(const ea::string& name) const;

private:
    void UpdateEntryTree();
    void ScanRootDirectory(const ea::string& resourceDir, ea::vector<FileSystemEntry>& entries, unsigned index);
    void CollectAddedFiles(const FileSystemEntry& oldRoot, const ea::vector<FileSystemEntry>& entries);
    ea::vector<FileSystemEntry> MergeEntries(ea::vector<FileSystemEntry>& entries) const;
    void AppendEntry(FileSystemEntry& root, const FileSystemEntry& entry);

    template <class T>
    static void ForEach(const FileSystemEntry& entry, const T& callback)
    {
        callback(entry);
        for (const FileSystemEntry& child : entry.children_)
            ForEach(child, callback);
    }

    const StringVector directories_;
    SharedPtr<MultiFileWatcher> fileWatcher_;

    bool treeDirty_{true};
    ea::unordered_set<ea::string> updatedResources_;

    FileSystemEntry root_;
    ea::unordered_map<ea::string, const FileSystemEntry*> index_;
};

}
