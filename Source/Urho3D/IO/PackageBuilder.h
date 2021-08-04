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

#pragma once

#include "../IO/Encription.h"
#include "../IO/PackageFile.h"

namespace Urho3D
{
class File;
class MemoryBuffer;

/// A helper class to create PackageFile.
class URHO3D_API PackageBuilder
{
public:
    /// Constructor.
    PackageBuilder();

    /// Build package.
    bool Create(AbstractFile* buffer, bool compress = false, const EncryptionKey* key = nullptr);

    /// Append entry to package.
    bool Append(const ea::string& name, AbstractFile* content);

    /// Append entry to package.
    bool Append(const ea::string& name, const ByteVector& content);

    /// Complete package.
    bool Build();
private:
    bool OpenImpl(AbstractFile* buffer, bool compress, bool encrypt);

    bool WriteHeader();
private:
    struct FileEntry
    {
        ea::string name_;
        unsigned offset_{};
        unsigned size_{};
        unsigned checksum_{};
    };

    /// Package encoding flags.
    PackageEncodingFlags encodingFlags_;

    unsigned headerPosition_;

    unsigned checksum_;

    unsigned long long fileListOffset_;

    AbstractFile* buffer_;

    ea::vector<FileEntry> entries_;

    EncryptionKey encryptionKey_;

    ea::unique_ptr<ChunkStreamSerializer> encoder_;
};

}
