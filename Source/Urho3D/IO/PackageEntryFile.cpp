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

#include "../Core/Profiler.h"
#include "../IO/PackageEntryFile.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/PackageFile.h"

#ifdef __ANDROID__
#include <SDL_rwops.h>
#endif

#include <cstdio>
#include <LZ4/lz4.h>

#include "../DebugNew.h"

namespace Urho3D
{

#ifdef _WIN32
static const wchar_t* openMode[] =
{
    L"rb",
    L"wb",
    L"r+b",
    L"w+b"
};
#else
static const char* openMode[] =
{
    "rb",
    "wb",
    "r+b",
    "w+b"
};
#endif

#ifdef __ANDROID__
static const unsigned READ_BUFFER_SIZE = 32768;
#endif
static const unsigned SKIP_BUFFER_SIZE = 1024;

PackageEntryFile::PackageEntryFile(Context* context)
    : Object(context),
#ifdef __ANDROID__
    assetHandle_(0)
    ,
#endif
    readBufferOffset_(0)
    , readBufferSize_(0)
    , offset_(0)
    , checksum_(0)
    , compressed_(false)
    , readSyncNeeded_(false)
    , writeSyncNeeded_(false)
{
}

PackageEntryFile::PackageEntryFile(Context* context, PackageFile* package, const ea::string& fileName)
    :
    Object(context),
#ifdef __ANDROID__
    assetHandle_(0),
#endif
    readBufferOffset_(0),
    readBufferSize_(0),
    offset_(0),
    checksum_(0),
    compressed_(false),
    readSyncNeeded_(false),
    writeSyncNeeded_(false)
{
    Open(package, fileName);
}

PackageEntryFile::~PackageEntryFile()
{
    Close();
}

bool PackageEntryFile::Open(PackageFile* package, const ea::string& fileName)
{
    if (!package)
        return false;

    const PackageEntry* entry = package->GetEntry(fileName);
    if (!entry)
        return false;

    if (!OpenInternal(package, fileName))
    {
        URHO3D_LOGERROR("Could not open package file " + fileName);
        return false;
    }

    name_ = fileName;
    offset_ = entry->offset_;
    checksum_ = entry->checksum_;
    size_ = entry->size_;
    compressed_ = package->IsCompressed();

    // Seek to beginning of package entry's file data
    SeekInternal(offset_);
    return true;
}

unsigned PackageEntryFile::Read(void* dest, unsigned size)
{
    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    if (size + position_ > size_)
        size = size_ - position_;
    if (!size)
        return 0;

    if (compressed_)
    {
        unsigned sizeLeft = size;
        auto* destPtr = (unsigned char*)dest;

        while (sizeLeft)
        {
            if (!readBuffer_ || readBufferOffset_ >= readBufferSize_)
            {
                unsigned char blockHeaderBytes[4];
                ReadInternal(blockHeaderBytes, sizeof blockHeaderBytes);

                MemoryBuffer blockHeader(&blockHeaderBytes[0], sizeof blockHeaderBytes);
                unsigned unpackedSize = blockHeader.ReadUShort();
                unsigned packedSize = blockHeader.ReadUShort();

                if (!readBuffer_)
                {
                    readBuffer_ = new unsigned char[unpackedSize];
                    inputBuffer_ = new unsigned char[LZ4_compressBound(unpackedSize)];
                }

                /// \todo Handle errors
                ReadInternal(inputBuffer_.get(), packedSize);
                LZ4_decompress_fast((const char*)inputBuffer_.get(), (char*)readBuffer_.get(), unpackedSize);

                readBufferSize_ = unpackedSize;
                readBufferOffset_ = 0;
            }

            unsigned copySize = Min((readBufferSize_ - readBufferOffset_), sizeLeft);
            memcpy(destPtr, readBuffer_.get() + readBufferOffset_, copySize);
            destPtr += copySize;
            sizeLeft -= copySize;
            readBufferOffset_ += copySize;
            position_ += copySize;
        }

        return size;
    }

    // Need to reassign the position due to internal buffering when transitioning from writing to reading
    if (readSyncNeeded_)
    {
        SeekInternal(position_ + offset_);
        readSyncNeeded_ = false;
    }

    if (!ReadInternal(dest, size))
    {
        // Return to the position where the read began
        SeekInternal(position_ + offset_);
        URHO3D_LOGERROR("Error while reading from file " + GetName());
        return 0;
    }

    writeSyncNeeded_ = true;
    position_ += size;
    return size;
}

unsigned PackageEntryFile::Seek(unsigned position)
{
    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    // Allow sparse seeks if writing
    if (position > size_)
        position = size_;

    if (compressed_)
    {
        // Start over from the beginning
        if (position == 0)
        {
            position_ = 0;
            readBufferOffset_ = 0;
            readBufferSize_ = 0;
            SeekInternal(offset_);
        }
        // Skip bytes
        else if (position >= position_)
        {
            unsigned char skipBuffer[SKIP_BUFFER_SIZE];
            while (position > position_)
                Read(skipBuffer, Min(position - position_, SKIP_BUFFER_SIZE));
        }
        else
            URHO3D_LOGERROR("Seeking backward in a compressed file is not supported");

        return position_;
    }

    SeekInternal(position + offset_);
    position_ = position;
    readSyncNeeded_ = false;
    writeSyncNeeded_ = false;
    return position_;
}

unsigned PackageEntryFile::Write(const void* data, unsigned size)
{
    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    URHO3D_LOGERROR("File not opened for writing");
    return 0;
}

const ea::string& PackageEntryFile::GetAbsoluteName() const
{
    return absoluteFileName_;
}

unsigned PackageEntryFile::GetChecksum()
{
    if (offset_ || checksum_)
        return checksum_;

    if (!IsOpen())
        return 0;

    URHO3D_PROFILE("CalculateFileChecksum");

    unsigned oldPos = position_;
    checksum_ = 0;

    Seek(0);
    while (!IsEof())
    {
        unsigned char block[1024];
        unsigned readBytes = Read(block, 1024);
        for (unsigned i = 0; i < readBytes; ++i)
            checksum_ = SDBMHash(checksum_, block[i]);
    }

    Seek(oldPos);
    return checksum_;
}

void PackageEntryFile::Close()
{
    readBuffer_.reset();
    inputBuffer_.reset();

    if (sourceFile_)
    {
        sourceFile_ = nullptr;
        position_ = 0;
        size_ = 0;
        offset_ = 0;
        checksum_ = 0;
    }
}

void PackageEntryFile::Flush()
{
    sourceFile_->Flush();
}

bool PackageEntryFile::IsOpen() const
{
    return sourceFile_;
}

bool PackageEntryFile::OpenInternal(PackageFile* package, const ea::string& fileName)
{
    Close();

    compressed_ = false;
    readSyncNeeded_ = false;
    writeSyncNeeded_ = false;

    sourceFile_ = package->OpenPackageFile();
    if (fileName.empty())
    {
        URHO3D_LOGERROR("Could not open file with empty name");
        return false;
    }

    name_ = fileName;
    absoluteFileName_ = fileName;
    position_ = 0;
    checksum_ = 0;

    return true;
}

bool PackageEntryFile::ReadInternal(void* dest, unsigned size)
{
    return sourceFile_->Read(dest, size);
}

void PackageEntryFile::SeekInternal(unsigned newPosition)
{
    sourceFile_->Seek(newPosition);
}

void PackageEntryFile::ReadBinary(ea::vector<unsigned char>& buffer)
{
    buffer.clear();

    if (!size_)
        return;

    buffer.resize(size_);

    Read(static_cast<void*>(buffer.data()), size_);
}

void PackageEntryFile::ReadText(ea::string& text)
{
    text.clear();

    if (!size_)
        return;

    text.resize(size_);

    Read(static_cast<void*>(&text[0]), size_);
}

}
