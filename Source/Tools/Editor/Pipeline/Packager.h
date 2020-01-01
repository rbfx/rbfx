//
// Copyright (c) 2017-2020 the rbfx project.
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

class Asset;

struct FileEntry
{
    /// This is essentially a resource name.
    ea::string name_;
    /// Offset to the file data from the file start.
    unsigned offset_{};
    /// Size of file data.
    unsigned size_{};
    /// Checksum of file data.
    unsigned checksum_{};
};

///
/// rbfx uses modified Urho3D pak file format. File header is modified and extended. Version field was added to facilitate easy modification
/// of file structure in the future. Package entry list was moved to the end of the file (much like in a zip file) in order to allow
/// creation of package files without knowing full list of files before-hand.
///

/// %Packager is responsible for creating a package for specified flavor. Package will use new file format and have RPAK/RLZ4 file id.
class Packager : public Object
{
    URHO3D_OBJECT(Packager, Object);
public:
    /// Construct.
    explicit Packager(Context* context);
    /// Destruct.
    ~Packager() override;
    /// Opens pak file for writing.
    bool OpenPackage(const ea::string& path, Flavor* flavor, bool compress=true);
    /// Returns value between 0.0f and 1.0f.
    float GetProgress() const;
    /// Returns true if packager is still packaging files.
    bool IsCompleted() const;
    /// Queues asset for packaging.
    void AddAsset(Asset* asset);
    /// Begins packaging process and returns immediately. Object must remain alive until IsCompleted() returns true.
    void Start();
    /// Returns flavor packager is packaging.
    Flavor* GetFlavor() const { return flavor_; }

protected:
    /// Add a file to the package. This is a blocking operation.
    bool AddFile(const ea::string& root, const ea::string& path);
    /// Writes file headter to the start of the file.
    void WriteHeaders();
    /// A worker running in another thread that will handle writing the package.
    void WritePackage();

    /// Per-package logger.
    Logger logger_{};
    /// Full path to output package file.
    ea::string outputPath_{};
    /// Package file.
    File output_;
    /// List of files that will be present in the package.
    ea::vector<FileEntry> entries_{};
    /// Flavor that is being compressed.
    WeakPtr<Flavor> flavor_;
    /// A list of assets that are to be written into the package.
    ea::vector<SharedPtr<Asset>> queuedAssets_{};
    /// Flag indicating whether file content is compressed or not.
    bool compress_ = false;
    /// Checksum of all file data (uncompressed).
    unsigned checksum_ = 0;
    /// Offset to the list of file entries in this package.
    int64_t entriesOffset_ = 0;
    /// LZ4 block size for data compression.
    const int blockSize_ = 32768;
    /// Buffer that holds data that was read from file. It will be written to package or used in compression.
    ea::vector<uint8_t> buffer_{};
    /// Buffer that holds compressed file data.
    ea::vector<uint8_t> compressBuffer_{};
    /// Total number of assets to be processed. This number may be less than files written to the package as each asset may carry multiple byproducts.
    unsigned filesTotal_ = 0;
    /// A number of already completed written assets.
    std::atomic<uint32_t> filesDone_{0};
};


}
