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

#include "../Precompiled.h"

#include "../IO/File.h"
#include "../IO/Log.h"
#include "../IO/PackageFile.h"
#include "../IO/FileSystem.h"

namespace Urho3D
{

PackageFile::PackageFile(Context* context) :
    MountPoint(context),
    totalSize_(0),
    totalDataSize_(0),
    checksum_(0),
    compressed_(false)
{
}

PackageFile::PackageFile(Context* context, const ea::string& fileName, unsigned startOffset) :
    MountPoint(context),
    totalSize_(0),
    totalDataSize_(0),
    checksum_(0),
    compressed_(false)
{
    Open(fileName, startOffset);
}

PackageFile::~PackageFile() = default;

bool PackageFile::Open(const ea::string& fileName, unsigned startOffset)
{
    auto file = MakeShared<File>(context_, fileName);
    if (!file->IsOpen())
        return false;

    // Check ID, then read the directory
    file->Seek(startOffset);
    ea::string id = file->ReadFileID();
    if (id != "UPAK" && id != "ULZ4" && id != "RPAK" && id != "RLZ4")
    {
        // If start offset has not been explicitly specified, also try to read package size from the end of file
        // to know how much we must rewind to find the package start
        if (!startOffset)
        {
            unsigned fileSize = file->GetSize();
            file->Seek((unsigned)(fileSize - sizeof(unsigned)));
            unsigned newStartOffset = fileSize - file->ReadUInt();
            if (newStartOffset < fileSize)
            {
                startOffset = newStartOffset;
                file->Seek(startOffset);
                id = file->ReadFileID();
            }
        }

        if (id != "UPAK" && id != "ULZ4" && id != "RPAK" && id != "RLZ4")
        {
            URHO3D_LOGERROR(fileName + " is not a valid package file");
            return false;
        }
    }

    fileName_ = fileName;
    nameHash_ = fileName_;
    totalSize_ = file->GetSize();
    compressed_ = id == "ULZ4" || id == "RLZ4";
    unsigned numFiles = file->ReadUInt();
    checksum_ = file->ReadUInt();

    if (id == "RPAK" || id == "RLZ4")
    {
        // New PAK file format includes two extra PAK header fields:
        // * Version. At this time this field is unused and is always 0. It will be used in the future if PAK format needs to be extended.
        // * File list offset. New format writes file list in the end of the file. This allows PAK creation without knowing entire file list
        //   beforehand.
        unsigned version = file->ReadUInt();                        // Reserved for future use.
        assert(version == 0);
        int64_t fileListOffset = file->ReadInt64();                 // New format has file list at the end of the file.
        file->Seek(fileListOffset);                                 // TODO: Serializer/Deserializer do not support files bigger than 4 GB
    }

    for (unsigned i = 0; i < numFiles; ++i)
    {
        ea::string entryName = file->ReadString();
        PackageEntry newEntry{};
        newEntry.offset_ = file->ReadUInt() + startOffset;
        totalDataSize_ += (newEntry.size_ = file->ReadUInt());
        newEntry.checksum_ = file->ReadUInt();
        if (!compressed_ && newEntry.offset_ + newEntry.size_ > totalSize_)
        {
            URHO3D_LOGERROR("File entry " + entryName + " outside package file");
            return false;
        }
        else
            entries_[entryName] = newEntry;
    }

    return true;
}

bool PackageFile::Exists(const ea::string& fileName) const
{
    bool found = entries_.find(fileName) != entries_.end();

#ifdef _WIN32
    // On Windows perform a fallback case-insensitive search
    if (!found)
    {
        for (auto i = entries_.begin(); i != entries_.end(); ++i)
        {
            if (!i->first.comparei(fileName))
            {
                found = true;
                break;
            }
        }
    }
#endif

    return found;
}

const PackageEntry* PackageFile::GetEntry(const ea::string& fileName) const
{
    auto i = entries_.find(fileName);
    if (i != entries_.end())
        return &i->second;

#ifdef _WIN32
    // On Windows perform a fallback case-insensitive search
    else
    {
        for (auto j = entries_.begin(); j != entries_.end(); ++j)
        {
            if (!j->first.comparei(fileName))
                return &j->second;
        }
    }
#endif

    return nullptr;
}

void PackageFile::Scan(
    ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, ScanFlags flags) const
{
    if (!flags.Test(SCAN_APPEND))
        result.clear();

    const bool recursive = flags.Test(SCAN_RECURSIVE);
    const ea::string filterExtension = GetExtensionFromFilter(filter);

    bool caseSensitive = true;
#ifdef _WIN32
    // On Windows ignore case in string comparisons
    caseSensitive = false;
#endif

    const StringVector& entryNames = GetEntryNames();
    for (const ea::string& entryName : entryNames)
    {
        if (MatchFileName(entryName, pathName, filterExtension, recursive, caseSensitive))
            result.push_back(TrimPathPrefix(entryName, pathName));
    }
}

/// Checks if mount point accepts scheme.
bool PackageFile::AcceptsScheme(const ea::string& scheme) const
{
    return scheme.empty() || scheme.comparei(GetName()) == 0;
}

/// Check if a file exists within the mount point.
bool PackageFile::Exists(const FileIdentifier& fileName) const
{
    // If scheme defined then it should match package name. Otherwise this package can't open the file.
    if (!AcceptsScheme(fileName.scheme_))
        return {};

    // Quit if file doesn't exists in the package.
    return Exists(fileName.fileName_);
}

/// Open package file. Returns null if file not found.
AbstractFilePtr PackageFile::OpenFile(const FileIdentifier& fileName, FileMode mode)
{
    // Package file can write files.
    if (mode != FILE_READ)
        return {};

    // If scheme defined it should match package name. Otherwise this package can't open the file.
    if (!fileName.scheme_.empty() && fileName.scheme_ != GetName())
        return {};

    // Quit if file doesn't exists in the package.
    if (!Exists(fileName.fileName_))
        return {};

    auto file = MakeShared<File>(context_, this, fileName.fileName_);
    file->SetName(fileName.ToUri());
    return file;
}

ea::optional<FileTime> PackageFile::GetLastModifiedTime(
    const FileIdentifier& fileName, bool creationIsModification) const
{
    if (!Exists(fileName))
        return ea::nullopt;

    auto fileSystem = GetSubsystem<FileSystem>();
    return fileSystem->GetLastModifiedTime(fileName_, creationIsModification);
}

}
