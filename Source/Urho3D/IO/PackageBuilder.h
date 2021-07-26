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
    bool Create(AbstractFile* buffer, bool compress);

    /// Append entry to package.
    bool Append(const ea::string& name, const SharedPtr<File>& file);

    /// Append entry to package.
    bool Append(const ea::string& name, MemoryBuffer& content);

    /// Append entry to package.
    bool Append(const ea::string& name, const ByteVector& content);

    /// Complete package.
    bool Build();
private:
    bool OpenImpl(AbstractFile* buffer, bool compress);

    /// Append entry to package.
    bool AppendImpl(const ea::string& name, AbstractFile* content);

    bool WriteHeader();
private:
    struct FileEntry
    {
        ea::string name_;
        unsigned offset_{};
        unsigned size_{};
        unsigned checksum_{};
    };

    bool compress_;

    unsigned headerPosition_;

    unsigned checksum_ = 0;

    unsigned long long fileListOffset_ = 0;

    AbstractFile* buffer_;

    SharedPtr<RefCounted> file_;

    ea::vector<FileEntry> entries_;
};

}
