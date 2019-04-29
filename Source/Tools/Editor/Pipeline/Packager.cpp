//
// Copyright (c) 2019 Rokas Kupstys
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

#include <EASTL/unique_ptr.h>

#include <Urho3D/IO/Log.h>
#include <LZ4/lz4.h>
#include <LZ4/lz4hc.h>
#include "Packager.h"


namespace Urho3D
{

Packager::Packager(Context* context)
    : Object(context)
    , output_(context)
{
    SetCompress(true);
}

bool Packager::OpenPackage(const ea::string& path)
{
    return output_.Open(path, FILE_WRITE);
}

void Packager::AddFile(const ea::string& root, const ea::string& path)
{
    FileEntry entry{};
    entry.root_ = root;
    entry.name_ = path;
    entry.checksum_ = 0;
    entry.offset_ = 0;
    entry.size_ = File(context_, root + path).GetSize();
    if (entry.size_)
        entries_.push_back(entry);
}

void Packager::Write()
{
    WriteHeaders();

    unsigned totalDataSize = 0;
    unsigned lastOffset = 0;
    const unsigned blockSize_ = 32768;
    ea::vector<uint8_t> buffer;

    auto logger = Log::GetLogger("packager");
    // Write file data, calculate checksums & correct offsets
    for (FileEntry& entry : entries_)
    {
        lastOffset = entry.offset_ = output_.GetSize();
        ea::string fileFullPath = entry.root_ + "/" + entry.name_;

        File srcFile(context_, fileFullPath);
        if (!srcFile.IsOpen())
        {
            logger.Error("Could not open file {}. Skipped!", fileFullPath);
            continue;
        }

        unsigned dataSize = entry.size_;
        totalDataSize += dataSize;
        buffer.resize(dataSize);

        if (srcFile.Read(&buffer[0], dataSize) != dataSize)
        {
            logger.Error("Could not read file {}. Skipped!", fileFullPath);
            continue;
        }
        srcFile.Close();

        for (unsigned j = 0; j < dataSize; ++j)
        {
            header_.checksum_ = SDBMHash(header_.checksum_, buffer[j]);
            entry.checksum_ = SDBMHash(entry.checksum_, buffer[j]);
        }

        if (!compress_)
        {
            logger.Info("Added {} size {}", entry.name_, dataSize);
            output_.Write(&buffer[0], entry.size_);
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
                    logger.Error("LZ4 compression failed for file {} at offset {}.", entry.name_, pos);

                output_.WriteUShort((unsigned short)unpackedSize);
                output_.WriteUShort((unsigned short)packedSize);
                output_.Write(compressBuffer.get(), packedSize);

                pos += unpackedSize;
            }

            unsigned totalPackedBytes = output_.GetSize() - lastOffset;
            logger.Info("{} in: {} out: {} ratio: {}", entry.name_, dataSize, totalPackedBytes,
                totalPackedBytes ? 1.f * dataSize / totalPackedBytes : 0.f);
        }
    }

    // Write package size to the end of file to allow finding it linked to an executable file
    unsigned currentSize = output_.GetSize();
    output_.WriteUInt(currentSize + sizeof(unsigned));

    WriteHeaders();
}

void Packager::WriteHeaders()
{
    header_.numEntries_ = entries_.size();

    output_.Seek(0);
    output_.Write(&header_, sizeof(header_));

    for (const FileEntry& entry : entries_)
    {
        output_.WriteString(entry.name_);
        output_.WriteUInt(entry.offset_);
        output_.WriteUInt(entry.size_);
        output_.WriteUInt(entry.checksum_);
    }
}

void Packager::SetCompress(bool compress)
{
    compress_ = compress;

    if (compress)
        memcpy(&header_.id_, "ULZ4", 4);
    else
        memcpy(&header_.id_, "UPAK", 4);
}

}
