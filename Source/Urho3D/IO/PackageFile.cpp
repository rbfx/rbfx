//
// Copyright (c) 2008-2020 the Urho3D project.
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
#include "LZ4/lz4.h"
#include "LZ4/lz4hc.h"

namespace Urho3D
{

namespace 
{
    struct FileEntry
    {
        ea::string name_;
        unsigned offset_{};
        unsigned size_{};
        unsigned checksum_{};
    };
}

PackageFile::PackageFile(Context* context) :
    Object(context),
    totalSize_(0),
    totalDataSize_(0),
    checksum_(0),
    compressed_(false)
{
}

PackageFile::PackageFile(Context* context, const ea::string& fileName, unsigned startOffset) :
    Object(context),
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
    SharedPtr<File> file(new File(context_, fileName));
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

void PackageFile::Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, bool recursive) const
{
    result.clear();

    ea::string sanitizedPath = GetSanitizedPath(pathName);
    ea::string filterExtension;
    unsigned dotPos = filter.find_last_of('.');
    if (dotPos != ea::string::npos)
        filterExtension = filter.substr(dotPos);
    if (filterExtension.contains('*'))
        filterExtension.clear();

    bool caseSensitive = true;
#ifdef _WIN32
    // On Windows ignore case in string comparisons
    caseSensitive = false;
#endif

    const StringVector& entryNames = GetEntryNames();
    for (auto i = entryNames.begin(); i != entryNames.end(); ++i)
    {
        ea::string entryName = GetSanitizedPath(*i);
        if ((filterExtension.empty() || entryName.ends_with(filterExtension, caseSensitive)) &&
            entryName.starts_with(sanitizedPath, caseSensitive))
        {
            ea::string fileName = entryName.substr(sanitizedPath.length());
            if (fileName.starts_with("\\") || fileName.starts_with("/"))
                fileName = fileName.substr(1, fileName.length() - 1);
            if (!recursive && (fileName.contains("\\") || fileName.contains("/")))
                continue;

            result.push_back(fileName);
        }
    }
}

/// Create package from existing files.
bool PackageFile::CreatePackage(Context* context, const ea::string& fileName, const ea::vector<SharedPtr<File>>& files, bool compress_)
{
    const int blockSize_ = 32768;

    ea::vector<FileEntry> entries_;
    unsigned checksum_ = 0;

    for (auto file: files)
    {
        FileEntry newEntry;
        newEntry.name_ = file->GetName();
        newEntry.offset_ = 0; // Offset not yet known
        newEntry.size_ = file->GetSize();
        newEntry.checksum_ = 0; // Will be calculated later
        entries_.push_back(newEntry);
    }

    File dest(context);
    if (!dest.Open(fileName, FILE_WRITE))
    {
        URHO3D_LOGERROR("Could not open output file " + fileName);
        return false;
    }

    // Write ID, number of files & placeholder for checksum
    if (!compress_)
        dest.WriteFileID("UPAK");
    else
        dest.WriteFileID("ULZ4");
    dest.WriteUInt(entries_.size());
    dest.WriteUInt(checksum_);

    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        // Write entry (correct offset is still unknown, will be filled in later)
        dest.WriteString(entries_[i].name_);
        dest.WriteUInt(entries_[i].offset_);
        dest.WriteUInt(entries_[i].size_);
        dest.WriteUInt(entries_[i].checksum_);
    }

    unsigned totalDataSize = 0;
    unsigned lastOffset;

    // Write file data, calculate checksums & correct offsets
    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        lastOffset = entries_[i].offset_ = dest.GetSize();
        auto srcFile = files[i];

        if (!srcFile->IsOpen())
        {
            URHO3D_LOGERROR("Could not open file " + srcFile->GetName());
            return false;
        }

        unsigned dataSize = entries_[i].size_;
        totalDataSize += dataSize;
        ea::unique_ptr<unsigned char[]> buffer(new unsigned char[dataSize]);

        if (srcFile->Read(&buffer[0], dataSize) != dataSize)
        {
            URHO3D_LOGERROR("Could not read file " + srcFile->GetName());
            return false;

        }
        srcFile->Close();

        for (unsigned j = 0; j < dataSize; ++j)
        {
            checksum_ = SDBMHash(checksum_, buffer[j]);
            entries_[i].checksum_ = SDBMHash(entries_[i].checksum_, buffer[j]);
        }

        if (!compress_)
        {
            //if (!quiet_)
            //    PrintLine(entries_[i].name_ + " size " + ea::to_string(dataSize));
            dest.Write(&buffer[0], entries_[i].size_);
        }
        else
        {
            ea::unique_ptr<unsigned char[]> compressBuffer(new unsigned char[LZ4_compressBound(blockSize_)]);

            unsigned pos = 0;

            while (pos < dataSize)
            {
                unsigned unpackedSize = blockSize_;
                if (pos + unpackedSize > dataSize)
                    unpackedSize = dataSize - pos;

                auto packedSize = (unsigned)LZ4_compress_HC((const char*)&buffer[pos], (char*)compressBuffer.get(), unpackedSize, LZ4_compressBound(unpackedSize), 0);
                if (!packedSize)
                {
                    URHO3D_LOGERROR("LZ4 compression failed for file " + entries_[i].name_ + " at offset " + ea::to_string(pos));
                    return false;
                }

                dest.WriteUShort((unsigned short)unpackedSize);
                dest.WriteUShort((unsigned short)packedSize);
                dest.Write(compressBuffer.get(), packedSize);

                pos += unpackedSize;
            }

            //if (!quiet_)
            //{
            //    unsigned totalPackedBytes = dest.GetSize() - lastOffset;
            //    ea::string fileEntry(entries_[i].name_);
            //    fileEntry.append_sprintf("\tin: %u\tout: %u\tratio: %f", dataSize, totalPackedBytes,
            //        totalPackedBytes ? 1.f * dataSize / totalPackedBytes : 0.f);
            //    PrintLine(fileEntry);
            //}
        }
    }

    // Write package size to the end of file to allow finding it linked to an executable file
    unsigned currentSize = dest.GetSize();
    dest.WriteUInt(currentSize + sizeof(unsigned));

    // Write header again with correct offsets & checksums
    dest.Seek(8);
    // Write ID, number of files & placeholder for checksum
    dest.WriteUInt(checksum_);

    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        dest.WriteString(entries_[i].name_);
        dest.WriteUInt(entries_[i].offset_);
        dest.WriteUInt(entries_[i].size_);
        dest.WriteUInt(entries_[i].checksum_);
    }

    //if (!quiet_)
    //{
    //    PrintLine("Number of files: " + ea::to_string(entries_.size()));
    //    PrintLine("File data size: " + ea::to_string(totalDataSize));
    //    PrintLine("Package size: " + ea::to_string(dest.GetSize()));
    //    PrintLine("Checksum: " + ea::to_string(checksum_));
    //    PrintLine("Compressed: " + ea::string(compress_ ? "yes" : "no"));
    //}
    return true;
}

}
