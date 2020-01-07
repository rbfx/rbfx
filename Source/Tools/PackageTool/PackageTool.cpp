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

#include <EASTL/sort.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/PackageFile.h>

#ifdef WIN32
#include <windows.h>
#endif

#include <EASTL/unique_ptr.h>
#include <LZ4/lz4.h>
#include <LZ4/lz4hc.h>

#include <Urho3D/DebugNew.h>


using namespace Urho3D;

static const unsigned COMPRESSED_BLOCK_SIZE = 32768;

struct FileEntry
{
    ea::string name_;
    unsigned offset_{};
    unsigned size_{};
    unsigned checksum_{};
};

Context* context_ = nullptr;
FileSystem* fileSystem_ = nullptr;
ea::string basePath_;
ea::vector<FileEntry> entries_;
unsigned checksum_ = 0;
bool compress_ = false;
bool quiet_ = false;
unsigned blockSize_ = COMPRESSED_BLOCK_SIZE;

ea::string ignoreExtensions_[] = {
    ".bak",
    ".rule"
};

int main(int argc, char** argv);
void Run(const ea::vector<ea::string>& arguments);
void ProcessFile(const ea::string& fileName, const ea::string& rootDir);
void WritePackageFile(const ea::string& fileName, const ea::string& rootDir);
void WriteHeader(File& dest);

int main(int argc, char** argv)
{
    SharedPtr<Context> context(new Context());
    SharedPtr<FileSystem> fileSystem(new FileSystem(context));
    ea::vector<ea::string> arguments;
    context_ = context;
    fileSystem_ = fileSystem;

    #ifdef WIN32
    arguments = ParseArguments(GetCommandLineW());
    #else
    arguments = ParseArguments(argc, argv);
    #endif

    Run(arguments);
    return 0;
}

void Run(const ea::vector<ea::string>& arguments)
{
    if (arguments.size() < 2)
        ErrorExit(
            "Usage: PackageTool <directory to process> <package name> [basepath] [options]\n"
            "\n"
            "Options:\n"
            "-c      Enable package file LZ4 compression\n"
            "-q      Enable quiet mode\n"
            "\n"
            "Basepath is an optional prefix that will be added to the file entries.\n\n"
            "Alternative output usage: PackageTool <output option> <package name>\n"
            "Output option:\n"
            "-i      Output package file information\n"
            "-l      Output file names (including their paths) contained in the package\n"
            "-L      Similar to -l but also output compression ratio (compressed package file only)\n"
        );

    const ea::string& dirName = arguments[0];
    const ea::string& packageName = arguments[1];
    bool isOutputMode = arguments[0].length() == 2 && arguments[0][0] == '-';
    if (arguments.size() > 2)
    {
        for (unsigned i = 2; i < arguments.size(); ++i)
        {
            if (arguments[i][0] != '-')
                basePath_ = AddTrailingSlash(arguments[i]);
            else
            {
                if (arguments[i].length() > 1)
                {
                    switch (arguments[i][1])
                    {
                    case 'c':
                        compress_ = true;
                        break;
                    case 'q':
                        quiet_ = true;
                        break;
                    default:
                        ErrorExit("Unrecognized option");
                    }
                }
            }
        }
    }

    if (!isOutputMode)
    {
        if (!quiet_)
            PrintLine("Scanning directory " + dirName + " for files");

        // Get the file list recursively
        ea::vector<ea::string> fileNames;
        fileSystem_->ScanDir(fileNames, dirName, "*", SCAN_FILES, true);
        if (!fileNames.size())
            ErrorExit("No files found");

        // Check for extensions to ignore
        for (unsigned i = fileNames.size() - 1; i < fileNames.size(); --i)
        {
            ea::string extension = GetExtension(fileNames[i]);
            for (unsigned j = 0; j < ignoreExtensions_[j].length(); ++j)
            {
                if (extension == ignoreExtensions_[j])
                {
                    fileNames.erase(fileNames.begin() + i);
                    break;
                }
            }
        }

        // Ensure entries are sorted
        ea::quick_sort(fileNames.begin(), fileNames.end());

        // Check if up to date
        if (fileSystem_->Exists(packageName))
        {
            unsigned packageTime = fileSystem_->GetLastModifiedTime(packageName);
            SharedPtr<PackageFile> packageFile(new PackageFile(context_, packageName));
            if (packageFile->GetNumFiles() == fileNames.size())
            {
                bool filesOutOfDate = false;
                for (const ea::string& fileName : fileNames)
                {
                    if (fileSystem_->GetLastModifiedTime(fileName) > packageTime)
                    {
                        filesOutOfDate = true;
                        break;
                    }
                }

                if (!filesOutOfDate)
                {
                    PrintLine("Package " + packageName + " is up to date.");
                    return;
                }
            }
        }

        for (unsigned i = 0; i < fileNames.size(); ++i)
            ProcessFile(fileNames[i], dirName);

        WritePackageFile(packageName, dirName);
    }
    else
    {
        SharedPtr<PackageFile> packageFile(new PackageFile(context_, packageName));
        bool outputCompressionRatio = false;
        switch (arguments[0][1])
        {
        case 'i':
            PrintLine("Number of files: " + ea::to_string(packageFile->GetNumFiles()));
            PrintLine("File data size: " + ea::to_string(packageFile->GetTotalDataSize()));
            PrintLine("Package size: " + ea::to_string(packageFile->GetTotalSize()));
            PrintLine("Checksum: " + ea::to_string(packageFile->GetChecksum()));
            PrintLine("Compressed: " + ea::string(packageFile->IsCompressed() ? "yes" : "no"));
            break;
        case 'L':
            if (!packageFile->IsCompressed())
                ErrorExit("Invalid output option: -L is applicable for compressed package file only");
            outputCompressionRatio = true;
            // Fallthrough
        case 'l':
            {
                const ea::unordered_map<ea::string, PackageEntry>& entries = packageFile->GetEntries();
                for (auto i = entries.begin(); i != entries.end();)
                {
                    auto current = i++;
                    ea::string fileEntry(current->first);
                    if (outputCompressionRatio)
                    {
                        unsigned compressedSize =
                            (i == entries.end() ? packageFile->GetTotalSize() - sizeof(unsigned) : i->second.offset_) -
                            current->second.offset_;
                        fileEntry.append_sprintf("\tin: %u\tout: %u\tratio: %f", current->second.size_, compressedSize,
                            compressedSize ? 1.f * current->second.size_ / compressedSize : 0.f);
                    }
                    PrintLine(fileEntry);
                }
            }
            break;
        default:
            ErrorExit("Unrecognized output option");
        }
    }
}

void ProcessFile(const ea::string& fileName, const ea::string& rootDir)
{
    ea::string fullPath = rootDir + "/" + fileName;
    File file(context_);
    if (!file.Open(fullPath))
        ErrorExit("Could not open file " + fileName);

    FileEntry newEntry;
    newEntry.name_ = fileName;
    newEntry.offset_ = 0; // Offset not yet known
    newEntry.size_ = file.GetSize();
    newEntry.checksum_ = 0; // Will be calculated later
    entries_.push_back(newEntry);
}

void WritePackageFile(const ea::string& fileName, const ea::string& rootDir)
{
    if (!quiet_)
        PrintLine("Writing package");

    File dest(context_);
    if (!dest.Open(fileName, FILE_WRITE))
        ErrorExit("Could not open output file " + fileName);

    // Write ID, number of files & placeholder for checksum
    WriteHeader(dest);

    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        // Write entry (correct offset is still unknown, will be filled in later)
        dest.WriteString(basePath_ + entries_[i].name_);
        dest.WriteUInt(entries_[i].offset_);
        dest.WriteUInt(entries_[i].size_);
        dest.WriteUInt(entries_[i].checksum_);
    }

    unsigned totalDataSize = 0;
    unsigned lastOffset;

    // Write file data, calculate checksums & correct offsets
    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        lastOffset = entries_[i].offset_ = dest.GetSize();
        ea::string fileFullPath = rootDir + "/" + entries_[i].name_;

        File srcFile(context_, fileFullPath);
        if (!srcFile.IsOpen())
            ErrorExit("Could not open file " + fileFullPath);

        unsigned dataSize = entries_[i].size_;
        totalDataSize += dataSize;
        ea::unique_ptr<unsigned char[]> buffer(new unsigned char[dataSize]);

        if (srcFile.Read(&buffer[0], dataSize) != dataSize)
            ErrorExit("Could not read file " + fileFullPath);
        srcFile.Close();

        for (unsigned j = 0; j < dataSize; ++j)
        {
            checksum_ = SDBMHash(checksum_, buffer[j]);
            entries_[i].checksum_ = SDBMHash(entries_[i].checksum_, buffer[j]);
        }

        if (!compress_)
        {
            if (!quiet_)
                PrintLine(entries_[i].name_ + " size " + ea::to_string(dataSize));
            dest.Write(&buffer[0], entries_[i].size_);
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
                    ErrorExit("LZ4 compression failed for file " + entries_[i].name_ + " at offset " + ea::to_string(pos));

                dest.WriteUShort((unsigned short)unpackedSize);
                dest.WriteUShort((unsigned short)packedSize);
                dest.Write(compressBuffer.get(), packedSize);

                pos += unpackedSize;
            }

            if (!quiet_)
            {
                unsigned totalPackedBytes = dest.GetSize() - lastOffset;
                ea::string fileEntry(entries_[i].name_);
                fileEntry.append_sprintf("\tin: %u\tout: %u\tratio: %f", dataSize, totalPackedBytes,
                    totalPackedBytes ? 1.f * dataSize / totalPackedBytes : 0.f);
                PrintLine(fileEntry);
            }
        }
    }

    // Write package size to the end of file to allow finding it linked to an executable file
    unsigned currentSize = dest.GetSize();
    dest.WriteUInt(currentSize + sizeof(unsigned));

    // Write header again with correct offsets & checksums
    dest.Seek(0);
    WriteHeader(dest);

    for (unsigned i = 0; i < entries_.size(); ++i)
    {
        dest.WriteString(basePath_ + entries_[i].name_);
        dest.WriteUInt(entries_[i].offset_);
        dest.WriteUInt(entries_[i].size_);
        dest.WriteUInt(entries_[i].checksum_);
    }

    if (!quiet_)
    {
        PrintLine("Number of files: " + ea::to_string(entries_.size()));
        PrintLine("File data size: " + ea::to_string(totalDataSize));
        PrintLine("Package size: " + ea::to_string(dest.GetSize()));
        PrintLine("Checksum: " + ea::to_string(checksum_));
        PrintLine("Compressed: " + ea::string(compress_ ? "yes" : "no"));
    }
}

void WriteHeader(File& dest)
{
    if (!compress_)
        dest.WriteFileID("UPAK");
    else
        dest.WriteFileID("ULZ4");
    dest.WriteUInt(entries_.size());
    dest.WriteUInt(checksum_);
}
