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

#pragma once


#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/File.h>


namespace Urho3D
{

struct FileEntry
{
    ea::string root_;
    ea::string name_;
    unsigned offset_{};
    unsigned size_{};
    unsigned checksum_{};
};

class Packager : public Object
{
    URHO3D_OBJECT(Packager, Object);
public:
    ///
    explicit Packager(Context* context);
    ///
    bool OpenPackage(const ea::string& path);
    ///
    void AddFile(const ea::string& root, const ea::string& path);
    ///
    void Write();
    ///
    void SetCompress(bool compress);

protected:
    ///
    File output_;
    ///
    struct PakHeader
    {
        /// UPAK or ULZ4.
        unsigned id_;
        /// Number of file entries in the pak.
        unsigned numEntries_;
        /// Checksum of entire data section.
        unsigned checksum_;
    } header_{};
    ///
    ea::vector<FileEntry> entries_{};
    ///
    bool compress_ = false;

    ///
    void WriteHeaders();
};


}
