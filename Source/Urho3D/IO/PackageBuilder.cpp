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
#include "../IO/MemoryBuffer.h"
#include "../IO/Log.h"
#include "../IO/PackageBuilder.h"
#include "../IO/FileSystem.h"
#include "LZ4/lz4.h"
#include "LZ4/lz4hc.h"

namespace Urho3D
{
    static const unsigned COMPRESSED_BLOCK_SIZE = 32768;

/// Constructor.
PackageBuilder::PackageBuilder():
    buffer_(nullptr)
{
}

bool PackageBuilder::WriteHeader()
{
    // Write ID, number of files & placeholder for checksum
    if (!compress_)
        buffer_->WriteFileID("RPAK");
    else
        buffer_->WriteFileID("RLZ4");

    // Num files
    if (!buffer_->WriteUInt(entries_.size()))
        return false;
    // Checksum
    if (!buffer_->WriteUInt(checksum_))
        return false;
    // Version
    if (!buffer_->WriteUInt(0))
        return false;
    // fileListOffset
    if (!buffer_->WriteUInt64(fileListOffset_))
        return false;

    return true;
}

bool PackageBuilder::Create(AbstractFile* dest, bool compress)
{
    if (buffer_)
    {
        URHO3D_LOGERROR("Target file already set");
        return false;
    }

    if (!dest->IsOpen())
    {
        URHO3D_LOGERROR("Can't open file " + dest->GetName());
        return false;
    }

    entries_.clear();
    headerPosition_ = dest->GetPosition();
    compress_ = compress;
    buffer_ = dest;
    file_ = dynamic_cast<RefCounted*>(dest);
    return WriteHeader();
}
/// Append entry to package.
bool PackageBuilder::Append(const ea::string& name, const SharedPtr<File>& file)
{
    return AppendImpl(name, file.Get());
}

/// Append entry to package.
bool PackageBuilder::Append(const ea::string& name, MemoryBuffer& content)
{
    return AppendImpl(name, &content);
}

/// Append entry to package.
bool  PackageBuilder::Append(const ea::string& name, const ByteVector& content)
{
    MemoryBuffer buffer(content);
    return Append(name, buffer);
}

/// Append entry to package.
bool PackageBuilder::AppendImpl(const ea::string& name, AbstractFile* content)
{
    if (!buffer_)
    {
        URHO3D_LOGERROR("PackageBuilder is not initialized");
        return false;
    }
    auto& entry = entries_.emplace_back();
    entry.name_ = name;
    entry.offset_ = buffer_->GetPosition();
    entry.checksum_ = 0;
    entry.size_ = content->GetSize();

    unsigned dataSize = content->GetSize();
    ea::unique_ptr<unsigned char[]> buffer(new unsigned char[dataSize]);

    if (content->Read(&buffer[0], dataSize) != dataSize)
    {
        URHO3D_LOGERROR("Could not read file " + buffer_->GetName());
        return false;
    }

    for (unsigned j = 0; j < dataSize; ++j)
    {
        checksum_ = SDBMHash(checksum_, buffer[j]);
        entry.checksum_ = SDBMHash(entry.checksum_, buffer[j]);
    }

    if (!compress_)
    {
        //if (!quiet_)
        //    PrintLine(entries_[i].name_ + " size " + ea::to_string(dataSize));
        buffer_->Write(&buffer[0], entry.size_);
    }
    else
    {
        constexpr auto blockSize_ = COMPRESSED_BLOCK_SIZE;
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
                URHO3D_LOGERROR("LZ4 compression failed for file " + buffer_->GetName() + " at offset " + ea::to_string(pos));
                return false;
            }

            buffer_->WriteUShort((unsigned short)unpackedSize);
            buffer_->WriteUShort((unsigned short)packedSize);
            buffer_->Write(compressBuffer.get(), packedSize);

            pos += unpackedSize;
        }
    }

    return true;
}

bool PackageBuilder::Build()
{
    if (!buffer_)
    {
        URHO3D_LOGERROR("PackageBuilder is not initialized");
        return false;
    }
    fileListOffset_ = buffer_->GetPosition();
    for (auto& entry: entries_)
    {
        buffer_->WriteString(entry.name_);
        buffer_->WriteUInt(entry.offset_ - headerPosition_);
        buffer_->WriteUInt(entry.size_);
        buffer_->WriteUInt(entry.checksum_);
    }
    buffer_->Seek(headerPosition_);
    WriteHeader();
    buffer_ = nullptr;
    file_.Reset();
    return true;
}

/*
/// Create package from existing files.
bool PackageBuilder::CreatePackage(Context* context, const ea::string& fileName, const ea::vector<PackageEntrySource>& files, bool compress_)
{
    SharedPtr<File> dest(new File(context));
    if (!dest->Open(fileName, FILE_WRITE))
    {
        URHO3D_LOGERROR("Could not open output file " + fileName);
        return false;
    }
    return CreatePackage(dest, files, compress_);
}

/// Create package from existing files.
bool PackageFile::CreatePackage(const SharedPtr<AbstractFile>& dest, const ea::vector<PackageEntrySource>& files, bool compress_)
{
    const int blockSize_ = 32768;

    ea::vector<FileEntry> entries_;
    unsigned checksum_ = 0;

    for (auto file : files)
    {
        FileEntry newEntry;
        newEntry.name_ = file.name_;
        newEntry.offset_ = 0; // Offset not yet known
        newEntry.size_ = file.file_->GetSize();
        newEntry.checksum_ = 0; // Will be calculated later
        entries_.push_back(newEntry);
    }

    // Write ID, number of files & placeholder for checksum
    if (!compress_)
        dest->WriteFileID("UPAK");
    else
        dest->WriteFileID("ULZ4");
    dest->WriteUInt(entries_.size());
    dest->WriteUInt(checksum_);

    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        // Write entry (correct offset is still unknown, will be filled in later)
        dest->WriteString(entries_[i].name_);
        dest->WriteUInt(entries_[i].offset_);
        dest->WriteUInt(entries_[i].size_);
        dest->WriteUInt(entries_[i].checksum_);
    }

    unsigned totalDataSize = 0;
    unsigned lastOffset;

    // Write file data, calculate checksums & correct offsets
    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        lastOffset = entries_[i].offset_ = dest->GetSize();
        const auto& srcFile = files[i];

        if (!srcFile.file_->IsOpen())
        {
            URHO3D_LOGERROR("Could not open file " + srcFile.file_->GetName());
            return false;
        }

        unsigned dataSize = entries_[i].size_;
        totalDataSize += dataSize;
        ea::unique_ptr<unsigned char[]> buffer(new unsigned char[dataSize]);

        if (srcFile.file_->Read(&buffer[0], dataSize) != dataSize)
        {
            URHO3D_LOGERROR("Could not read file " + srcFile.file_->GetName());
            return false;

        }
        srcFile.file_->Close();

        for (unsigned j = 0; j < dataSize; ++j)
        {
            checksum_ = SDBMHash(checksum_, buffer[j]);
            entries_[i].checksum_ = SDBMHash(entries_[i].checksum_, buffer[j]);
        }

        if (!compress_)
        {
            //if (!quiet_)
            //    PrintLine(entries_[i].name_ + " size " + ea::to_string(dataSize));
            dest->Write(&buffer[0], entries_[i].size_);
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

                dest->WriteUShort((unsigned short)unpackedSize);
                dest->WriteUShort((unsigned short)packedSize);
                dest->Write(compressBuffer.get(), packedSize);

                pos += unpackedSize;
            }

            //if (!quiet_)
            //{
            //    unsigned totalPackedBytes = dest->GetSize() - lastOffset;
            //    ea::string fileEntry(entries_[i].name_);
            //    fileEntry.append_sprintf("\tin: %u\tout: %u\tratio: %f", dataSize, totalPackedBytes,
            //        totalPackedBytes ? 1.f * dataSize / totalPackedBytes : 0.f);
            //    PrintLine(fileEntry);
            //}
        }
    }

    // Write package size to the end of file to allow finding it linked to an executable file
    unsigned currentSize = dest->GetSize();
    dest->WriteUInt(currentSize + sizeof(unsigned));

    // Write header again with correct offsets & checksums
    dest->Seek(8);
    // Write ID, number of files & placeholder for checksum
    dest->WriteUInt(checksum_);

    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        dest->WriteString(entries_[i].name_);
        dest->WriteUInt(entries_[i].offset_);
        dest->WriteUInt(entries_[i].size_);
        dest->WriteUInt(entries_[i].checksum_);
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
*/
}
