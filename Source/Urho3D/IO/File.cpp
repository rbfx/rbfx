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

#include "../Core/Profiler.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/PackageFile.h"
#include "../IO/Compression.h"
#include "../IO/Encription.h"

#ifdef __ANDROID__
#include <SDL/SDL_rwops.h>
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

RawFile::RawFile():
    handle_(nullptr),
#ifdef __ANDROID__
    assetHandle_(0),
#endif
    offset_(0)
{
    
}

bool RawFile::IsOpen() const
{
#ifdef __ANDROID__
    return handle_ != 0 || assetHandle_ != 0;
#else
    return handle_ != nullptr;
#endif
}

/// Open file internally using either C standard IO functions or SDL RWops for Android asset files. Return true if successful.
bool RawFile::Open(const ea::string& fileName, FileMode mode, unsigned offset, unsigned maxSize)
{
#ifdef __ANDROID__
    if (URHO3D_IS_ASSET(fileName))
    {
        if (mode != FILE_READ)
        {
            URHO3D_LOGERROR("Only read mode is supported for Android asset files");
            return false;
        }

        assetHandle_ = SDL_RWFromFile(URHO3D_ASSET(fileName), "rb");
        if (!assetHandle_)
        {
            URHO3D_LOGERRORF("Could not open Android asset file %s", fileName.c_str());
            return false;
        }
        else
        {
            name_ = fileName;
            absoluteFileName_ = fileName;
            mode_ = mode;
            position_ = 0;
            offset_ = offset;
            size_ = ea::min(SDL_RWsize(assetHandle_)- offset_, maxSize);
            checksum_ = 0;
            if (offset_)
                Seek(0);
            return true;
        }
    }
#endif

#ifdef _WIN32
    handle_ = _wfopen(GetWideNativePath(fileName).c_str(), openMode[mode]);
#else
    handle_ = fopen(GetNativePath(fileName).c_str(), openMode[mode]);
#endif

    // If file did not exist in readwrite mode, retry with write-update mode
    if (mode == FILE_READWRITE && !handle_)
    {
#ifdef _WIN32
        handle_ = _wfopen(GetWideNativePath(fileName).c_str(), openMode[mode + 1]);
#else
        handle_ = fopen(GetNativePath(fileName).c_str(), openMode[mode + 1]);
#endif
    }

    if (!handle_)
    {
        URHO3D_LOGERRORF("Could not open file %s", fileName.c_str());
        return false;
    }

    offset_ = offset;

    fseek((FILE*)handle_, 0, SEEK_END);
    long size = ftell((FILE*)handle_);
    if (size < offset_)
    {
        URHO3D_LOGERRORF("Could not open file %s with offset beyond file size", fileName.c_str());
        Close();
        size_ = 0;
        return false;
    }
    if (size > M_MAX_UNSIGNED)
    {
        URHO3D_LOGERRORF("Could not open file %s which is larger than 4GB", fileName.c_str());
        Close();
        size_ = 0;
        return false;
    }
    size_ = ea::min<unsigned>(size - offset_, maxSize);
    Seek(0);

    return true;
}


unsigned RawFile::Read(void* dest, unsigned size)
{
    unsigned actualSize = ea::min(size, size_ - position_);
#ifdef __ANDROID__
    if (assetHandle_)
    {
        unsigned readed = SDL_RWread(assetHandle_, dest, 1, actualSize) == 1);
        position_ += readed;
        return readed;
    }
#endif
        unsigned readed = fread(dest, 1, actualSize, (FILE*)handle_);
        position_ += readed;
        return readed;
}

/// Set position from the beginning of the file.
unsigned RawFile::Seek(unsigned position)
{
    return SeekLong(position);
}


/// Set position from the beginning of the file.
unsigned RawFile::SeekLong(unsigned long position)
{
#ifdef __ANDROID__
    if (assetHandle_)
    {
        auto seekResult = SDL_RWseek(assetHandle_, position + offset_, SEEK_SET);
        if (seekResult < 0)
        {
            return position_;
        }
        return (position_ = seekResult - offset_);
    }
#endif
    if (!fseek((FILE*)handle_, position + offset_, SEEK_SET))
    {
        return (position_ = position);
    }
    return position_;
}

/// Write bytes to the file. Return number of bytes actually written.
unsigned RawFile::Write(const void* data, unsigned size)
{
    if (fwrite(data, size, 1, (FILE*)handle_) != 1)
        return 0;
    position_ += size;
    return size;
}

void RawFile::Close()
{
#ifdef __ANDROID__
    if (assetHandle_)
    {
        SDL_RWclose(assetHandle_);
        assetHandle_ = 0;
    }
#endif

    if (handle_)
    {
        fclose((FILE*)handle_);
        handle_ = nullptr;
    }
}

void RawFile::Flush()
{
    if (handle_)
        fflush((FILE*)handle_);
}

File::File(Context* context) :
    Object(context),
    mode_(FILE_READ),
#ifdef __ANDROID__
    readBufferOffset_(0),
    readBufferSize_(0),
#endif
    checksum_(0),
    decoder_(nullptr),
    readSyncNeeded_(false),
    writeSyncNeeded_(false)
{
}

File::File(Context* context, const ea::string& fileName, FileMode mode) :
    Object(context),
    mode_(FILE_READ),
#ifdef __ANDROID__
    readBufferOffset_(0),
    readBufferSize_(0),
#endif
    checksum_(0),
    decoder_(nullptr),
    readSyncNeeded_(false),
    writeSyncNeeded_(false)
{
    Open(fileName, mode);
}

File::File(Context* context, PackageFile* package, const ea::string& fileName, const EncryptionKey* key) :
    Object(context),
    mode_(FILE_READ),
#ifdef __ANDROID__
    assetHandle_(0),
    readBufferOffset_(0),
    readBufferSize_(0),
#endif
    checksum_(0),
    decoder_(nullptr),
    readSyncNeeded_(false),
    writeSyncNeeded_(false)
{
    Open(package, fileName, key);
}

File::~File()
{
    Close();
}

bool File::Open(const ea::string& fileName, FileMode mode)
{
    return OpenInternal(fileName, mode);
}

bool File::Open(PackageFile* package, const ea::string& fileName, const EncryptionKey* key)
{
    if (!package)
        return false;

    const PackageEntry* entry = package->GetEntry(fileName);
    if (!entry)
        return false;

    bool success = OpenInternal(package->GetName(), FILE_READ, entry, package->PackageEncoding() != PE_NONE);
    if (!success)
    {
        URHO3D_LOGERROR("Could not open package file " + fileName);
        return false;
    }

    name_ = fileName;
    checksum_ = entry->checksum_;
    size_ = entry->size_;

    switch (package->PackageEncoding())
    {
    case PE_NONE:
        break;
    case PE_LZ4:
        decoder_ = ea::make_unique<CompressedStreamDeserializer>(file_);
        break;
    case PE_TWEETNACL:
        if (!key)
        {
            URHO3D_LOGERROR("No encryption key provided");
            return false;
        }
        decoder_ = ea::make_unique<EncryptedStreamDeserializer>(file_, *key);
        break;
    case PE_LZ4_TWEETNACL:
        if (!key)
        {
            URHO3D_LOGERROR("No encryption key provided");
            return false;
        }
        decoder_ = ea::make_unique<EncryptedStreamDeserializer>(file_, *key);
        break;
    default:
        abort();
    }

    return true;
}

unsigned File::Read(void* dest, unsigned size)
{
    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    if (mode_ == FILE_WRITE)
    {
        URHO3D_LOGERROR("File not opened for reading");
        return 0;
    }

    if (size + position_ > size_)
        size = size_ - position_;
    if (!size)
        return 0;

    if (decoder_)
    {
        return decoder_->Read(dest, size);
    }

#ifdef __ANDROID__
    if (assetHandle_ && !compressed_)
    {
        // If not using a compressed package file, buffer file reads on Android for better performance
        if (!readBuffer_)
        {
            readBuffer_ = new unsigned char[READ_BUFFER_SIZE];
            readBufferOffset_ = 0;
            readBufferSize_ = 0;
        }

        unsigned sizeLeft = size;
        unsigned char* destPtr = (unsigned char*)dest;

        while (sizeLeft)
        {
            if (readBufferOffset_ >= readBufferSize_)
            {
                readBufferSize_ = Min(size_ - position_, READ_BUFFER_SIZE);
                readBufferOffset_ = 0;
                file.Read(readBuffer_.get(), readBufferSize_);
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
#endif

    // Need to reassign the position due to internal buffering when transitioning from writing to reading
    if (readSyncNeeded_)
    {
        SeekInternal(position_);
        readSyncNeeded_ = false;
    }

    if (file_.Read(dest, size) != size)
    {
        // Return to the position where the read began
        SeekInternal(position_);
        URHO3D_LOGERROR("Error while reading from file " + GetName());
        return 0;
    }

    writeSyncNeeded_ = true;
    position_ += size;
    return size;
}

unsigned File::Seek(unsigned position)
{
    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    // Allow sparse seeks if writing
    if (mode_ == FILE_READ && position > size_)
        position = size_;

    if (decoder_)
    {
        return decoder_->Seek(position);
    }

    SeekInternal(position);
    position_ = position;
    readSyncNeeded_ = false;
    writeSyncNeeded_ = false;
    return position_;
}

unsigned File::Write(const void* data, unsigned size)
{
    if (!IsOpen())
    {
        // If file not open, do not log the error further here to prevent spamming the stderr stream
        return 0;
    }

    if (mode_ == FILE_READ)
    {
        URHO3D_LOGERROR("File not opened for writing");
        return 0;
    }

    if (!size)
        return 0;

    // Need to reassign the position due to internal buffering when transitioning from reading to writing
    if (writeSyncNeeded_)
    {
        file_.SeekLong((long)position_);
        writeSyncNeeded_ = false;
    }

    if (file_.Write(data, size) != size)
    {
        // Return to the position where the write began
        file_.SeekLong((long)position_);
        URHO3D_LOGERROR("Error while writing to file " + GetName());
        return 0;
    }

    readSyncNeeded_ = true;
    position_ += size;
    if (position_ > size_)
        size_ = position_;

    return size;
}

unsigned File::GetChecksum()
{
    if (IsPackaged() || checksum_)
        return checksum_;
#ifdef __ANDROID__
    if ((!handle_ && !assetHandle_) || mode_ == FILE_WRITE)
#else
    if (!file_.GetHandle() || mode_ == FILE_WRITE)
#endif
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

void File::Close()
{
    file_.Close();

    position_ = 0;
    size_ = 0;
    checksum_ = 0;

#ifdef __ANDROID__
    readBuffer_.reset();
#endif

    decoder_ = nullptr;
}

void File::Flush()
{
    file_.Flush();
}

bool File::IsOpen() const
{
    return file_.IsOpen();
}

bool File::OpenInternal(const ea::string& fileName, FileMode mode, const PackageEntry* packageEntry, bool encoded)
{
    Close();

    decoder_ = nullptr;
    readSyncNeeded_ = false;
    writeSyncNeeded_ = false;

    auto* fileSystem = GetSubsystem<FileSystem>();
    if (fileSystem && !fileSystem->CheckAccess(GetPath(fileName)))
    {
        URHO3D_LOGERRORF("Access denied to %s", fileName.c_str());
        return false;
    }

    if (fileName.empty())
    {
        URHO3D_LOGERROR("Could not open file with empty name");
        return false;
    }

    // For an encoded chunked stream we don't know actual data size in the underlying file so let's not limit it.
    unsigned size = (packageEntry && !encoded) ? packageEntry->size_ : ea::numeric_limits<unsigned>::max();
    // Underlying file offset depends on package entry offset.
    unsigned offset = packageEntry ? packageEntry->offset_ : 0;

    if (!file_.Open(fileName, mode, offset, size))
    {
        URHO3D_LOGERROR("Could not open file "+ fileName);
        return false;
    }
    // If we read a package entry the decoded data size is defined in the entry, otherwise it matches entire file size.
    size_ = packageEntry ? packageEntry->size_ : file_.GetSize();

    name_ = fileName;
    absoluteFileName_ = fileName;
    mode_ = mode;
    position_ = 0;
    checksum_ = 0;

    return true;
}


/// Seek in file internally using either C standard IO functions or SDL RWops for Android asset files.
void File::SeekInternal(unsigned newPosition)
{
    file_.Seek(newPosition);
#ifdef __ANDROID__
    // Reset buffering after seek
    readBufferOffset_ = 0;
    readBufferSize_ = 0;
#endif
}

void File::ReadBinary(ea::vector<unsigned char>& buffer)
{
    buffer.clear();

    if (!size_)
        return;

    buffer.resize(size_);

    Read(static_cast<void*>(buffer.data()), size_);
}

void File::ReadText(ea::string& text)
{
    text.clear();

    if (!size_)
        return;

    text.resize(size_);

    Read(static_cast<void*>(&text[0]), size_);
}

bool File::Copy(File* srcFile)
{
    if (!srcFile || !srcFile->IsOpen() || srcFile->GetMode() != FILE_READ)
        return false;

    if (!IsOpen() || GetMode() != FILE_WRITE)
        return false;

    unsigned fileSize = srcFile->GetSize();
    ea::shared_array<unsigned char> buffer(new unsigned char[fileSize]);

    unsigned bytesRead = srcFile->Read(buffer.get(), fileSize);
    unsigned bytesWritten = Write(buffer.get(), fileSize);
    return bytesRead == fileSize && bytesWritten == fileSize;

}

}
