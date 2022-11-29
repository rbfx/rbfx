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

/// \file

#pragma once

#include <EASTL/shared_array.h>

#include "../Core/Object.h"
#include "../IO/FileSystemFile.h"

#ifdef __ANDROID__
struct SDL_RWops;
#endif

namespace Urho3D
{

class PackageFile;

/// %File opened either through the filesystem or from within a package file.
class URHO3D_API PackageEntryFile
    : public Object
    , public AbstractFile
{
    URHO3D_OBJECT(PackageEntryFile, Object);

public:
    /// Construct.
    explicit PackageEntryFile(Context* context);
    /// Construct and open from a package file.
    PackageEntryFile(Context* context, PackageFile* package, const ea::string& fileName);
    /// Destruct. Close the file if open.
    ~PackageEntryFile() override;

    /// Read bytes from the file. Return number of bytes actually read.
    unsigned Read(void* dest, unsigned size) override;
    /// Set position from the beginning of the file.
    unsigned Seek(unsigned position) override;
    /// Write bytes to the file. Return number of bytes actually written.
    unsigned Write(const void* data, unsigned size) override;

    /// Return absolute file name in file system.
    const ea::string& GetAbsoluteName() const override;

    /// Return a checksum of the file contents using the SDBM hash algorithm.
    unsigned GetChecksum() override;

    /// Open from within a package file. Return true if successful.
    bool Open(PackageFile* package, const ea::string& fileName);
    /// Close the file.
    void Close() override;
    /// Flush any buffered output to the file.
    void Flush();

    /// Return whether is open.
    /// @property
    bool IsOpen() const override;

    /// Reads a binary file to buffer.
    void ReadBinary(ea::vector<unsigned char>& buffer);

    /// Reads a binary file to buffer.
    ea::vector<unsigned char> ReadBinary() { ea::vector<unsigned char> retValue; ReadBinary(retValue); return retValue; }

    /// Reads a text file, ensuring data from file is 0 terminated
    virtual void ReadText(ea::string& text);

    /// Reads a text file, ensuring data from file is 0 terminated
    virtual ea::string ReadText() { ea::string retValue; ReadText(retValue); return retValue; }

private:
    /// Open file internally using either C standard IO functions or SDL RWops for Android asset files. Return true if successful.
    bool OpenInternal(PackageFile* package, const ea::string& fileName);
    /// Perform the file read internally using either C standard IO functions or SDL RWops for Android asset files. Return true if successful. This does not handle compressed package file reading.
    bool ReadInternal(void* dest, unsigned size);
    /// Seek in file internally using either C standard IO functions or SDL RWops for Android asset files.
    void SeekInternal(unsigned newPosition);

    /// Absolute file name.
    ea::string absoluteFileName_;
    /// Underlying package file.
    SharedPtr<FileSystemFile> sourceFile_;
    /// Read buffer for Android asset or compressed file loading.
    ea::shared_array<unsigned char> readBuffer_;
    /// Decompression input buffer for compressed file loading.
    ea::shared_array<unsigned char> inputBuffer_;
    /// Read buffer position.
    unsigned readBufferOffset_;
    /// Bytes in the current read buffer.
    unsigned readBufferSize_;
    /// Start position within a package file, 0 for regular files.
    unsigned offset_;
    /// Content checksum.
    unsigned checksum_;
    /// Compression flag.
    bool compressed_;
    /// Synchronization needed before read -flag.
    bool readSyncNeeded_;
    /// Synchronization needed before write -flag.
    bool writeSyncNeeded_;
};

}
