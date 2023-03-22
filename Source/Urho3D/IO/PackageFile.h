//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include "Urho3D/IO/MountPoint.h"
#include "Urho3D/IO/ScanFlags.h"

namespace Urho3D
{

/// %File entry within the package file.
struct PackageEntry
{
    /// Offset from the beginning.
    unsigned offset_;
    /// File size.
    unsigned size_;
    /// File checksum.
    unsigned checksum_;
};

/// Stores files of a directory tree sequentially for convenient access.
class URHO3D_API PackageFile : public MountPoint
{
    URHO3D_OBJECT(PackageFile, MountPoint);

public:
    /// Construct.
    explicit PackageFile(Context* context);
    /// Construct and open.
    PackageFile(Context* context, const ea::string& fileName, unsigned startOffset = 0);
    /// Destruct.
    ~PackageFile() override;

    /// Open the package file. Return true if successful.
    bool Open(const ea::string& fileName, unsigned startOffset = 0);
    /// Check if a file exists within the package file. This will be case-insensitive on Windows and case-sensitive on other platforms.
    bool Exists(const ea::string& fileName) const;
    /// Return the file entry corresponding to the name, or null if not found. This will be case-insensitive on Windows and case-sensitive on other platforms.
    const PackageEntry* GetEntry(const ea::string& fileName) const;

    /// Return all file entries.
    const ea::unordered_map<ea::string, PackageEntry>& GetEntries() const { return entries_; }

    /// Return hash of the package file name.
    StringHash GetNameHash() const { return nameHash_; }

    /// Return number of files.
    /// @property
    unsigned GetNumFiles() const { return entries_.size(); }

    /// Return total size of the package file.
    /// @property
    unsigned GetTotalSize() const { return totalSize_; }

    /// Return total data size from all the file entries in the package file.
    /// @property
    unsigned GetTotalDataSize() const { return totalDataSize_; }

    /// Return checksum of the package file contents.
    /// @property
    unsigned GetChecksum() const { return checksum_; }

    /// Return whether the files are compressed.
    /// @property
    bool IsCompressed() const { return compressed_; }

    /// Return list of file names in the package.
    const ea::vector<ea::string> GetEntryNames() const { return entries_.keys(); }

    /// Return a file name in the package at the specified index
    const ea::string& GetEntryName(unsigned index) const
    {
        unsigned nn = 0;
        for (auto j = entries_.begin(); j != entries_.end(); ++j)
        {
            if (nn == index) return j->first;
            nn++;
        }
        return EMPTY_STRING;
    }

    /// Implement MountPoint.
    /// @{
    bool AcceptsScheme(const ea::string& scheme) const override;
    bool Exists(const FileIdentifier& fileName) const override;
    AbstractFilePtr OpenFile(const FileIdentifier& fileName, FileMode mode) override;
    ea::optional<FileTime> GetLastModifiedTime(
        const FileIdentifier& fileName, bool creationIsModification) const override;

    const ea::string& GetName() const override { return fileName_; }

    void Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter,
        ScanFlags flags) const override;
    /// @}

private:
    /// File entries.
    ea::unordered_map<ea::string, PackageEntry> entries_;
    /// File name.
    ea::string fileName_;
    /// Package file name hash.
    StringHash nameHash_;
    /// Package file total size.
    unsigned totalSize_;
    /// Total data size in the package using each entry's actual size if it is a compressed package file.
    unsigned totalDataSize_;
    /// Package file checksum.
    unsigned checksum_;
    /// Compressed flag.
    bool compressed_;
};

}
