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
#include "../IO/AbstractFile.h"

#ifdef __ANDROID__
struct SDL_RWops;
#endif

namespace Urho3D
{

#ifdef __ANDROID__
static const char* APK = "/apk/";
// Macro for checking if a given pathname is inside APK's assets directory
#define URHO3D_IS_ASSET(p) p.starts_with(APK)
// Macro for truncating the APK prefix string from the asset pathname and at the same time patching the directory name components (see custom_rules.xml)
#ifdef ASSET_DIR_INDICATOR
#define URHO3D_ASSET(p) p.Substring(5).Replaced("/", ASSET_DIR_INDICATOR "/").c_str()
#else
#define URHO3D_ASSET(p) p.substr(5).c_str()
#endif
#else
static const char* APK = "";
#endif

class PackageFile;

/// %File opened either through the filesystem or from within a package file.
class URHO3D_API File : public Object, public AbstractFile
{
    URHO3D_OBJECT(File, Object);

public:
    /// Construct.
    explicit File(Context* context);
    /// Construct and open a filesystem file.
    File(Context* context, const ea::string& fileName, FileMode mode = FILE_READ);
    /// Construct and open from a package file.
    File(Context* context, PackageFile* package, const ea::string& fileName);
    /// Destruct. Close the file if open.
    ~File() override;

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

    /// Open a filesystem file. Return true if successful.
    bool Open(const ea::string& fileName, FileMode mode = FILE_READ);
    /// Open from within a package file. Return true if successful.
    bool Open(PackageFile* package, const ea::string& fileName);
    /// Close the file.
    void Close() override;
    /// Flush any buffered output to the file.
    void Flush();

    /// Return the open mode.
    /// @property
    FileMode GetMode() const { return mode_; }

    /// Return whether is open.
    /// @property
    bool IsOpen() const override;

    /// Return the file handle.
    void* GetHandle() const { return handle_; }

    /// Return whether the file originates from a package.
    /// @property
    bool IsPackaged() const { return offset_ != 0; }

    /// Reads a binary file to buffer.
    void ReadBinary(ea::vector<unsigned char>& buffer);

    /// Reads a binary file to buffer.
    ea::vector<unsigned char> ReadBinary() { ea::vector<unsigned char> retValue; ReadBinary(retValue); return retValue; }

    /// Reads a text file, ensuring data from file is 0 terminated
    virtual void ReadText(ea::string& text);

    /// Reads a text file, ensuring data from file is 0 terminated
    virtual ea::string ReadText() { ea::string retValue; ReadText(retValue); return retValue; }

    /// Copy a file from a source file, must be opened and FILE_WRITE
    /// Unlike FileSystem.Copy this copy works when the source file is in a package file
    bool Copy(File* srcFile);

private:
    /// Open file internally using either C standard IO functions or SDL RWops for Android asset files. Return true if successful.
    bool OpenInternal(const ea::string& fileName, FileMode mode, bool fromPackage = false);
    /// Perform the file read internally using either C standard IO functions or SDL RWops for Android asset files. Return true if successful. This does not handle compressed package file reading.
    bool ReadInternal(void* dest, unsigned size);
    /// Seek in file internally using either C standard IO functions or SDL RWops for Android asset files.
    void SeekInternal(unsigned newPosition);

    /// Absolute file name.
    ea::string absoluteFileName_;
    /// Open mode.
    FileMode mode_;
    /// File handle.
    void* handle_;
#ifdef __ANDROID__
    /// SDL RWops context for Android asset loading.
    SDL_RWops* assetHandle_;
#endif
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
