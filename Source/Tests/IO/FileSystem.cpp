// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>

TEST_CASE("ResolvePath handles /")
{
    // Leading / preservation
    CHECK(ResolvePath("/bla") == "/bla");

    // Single / preservation
    CHECK(ResolvePath("/") == "/");

    // Consecutive / replaced with a single /
    CHECK(ResolvePath("root///bla") == "root/bla");
}

TEST_CASE("ResolvePath handles ..")
{
    // Keep old behaviour for ../ at root position
    CHECK(ResolvePath("../bla") == "bla");

    // Eliminate parent path if it is root
    CHECK(ResolvePath("root/../bla") == "bla");

    // Eliminate parent path
    CHECK(ResolvePath("root/sub/../bla") == "root/bla");

    // Eliminate parent paths when consecutive ..
    CHECK(ResolvePath("root/sub/sub2/../../bla") == "root/bla");

    // Eliminate parent paths when consecutive ..
    CHECK(ResolvePath("root/sub/../../bla") == "bla");

    // Eliminate trailing ..
    CHECK(ResolvePath("root/bla/..") == "root");
}

TEST_CASE("ResolvePath handles .")
{
    // Keep old behaviour for ./ at root position
    CHECK(ResolvePath("./bla") == "bla");

    // Eliminate parent path if it is root
    CHECK(ResolvePath("root/./bla") == "root/bla");

    // Eliminate trailing dot
    CHECK(ResolvePath("bla/.") == "bla");
}

namespace
{

SharedPtr<Context> CreateFileSystemContext()
{
    auto context = MakeShared<Context>();
    context->RegisterSubsystem(MakeShared<FileSystem>(context));
    return context;
}

ea::string GetCopyTestDirectory(FileSystem* fs)
{
    return fs->GetTemporaryDir() + "rbfx-file-copy-test-" + ea::to_string(GetCurrentProcessID());
}

} // namespace

TEST_CASE("FileSystem copies file contents and reports failures", "[file-copy]")
{
    auto context = Tests::GetOrCreateContext(CreateFileSystemContext);
    auto fs = context->GetSubsystem<FileSystem>();
    TemporaryDir directory(context, GetCopyTestDirectory(fs));
    REQUIRE(fs->DirExists(directory.GetPath()));
    const auto source = directory.GetPath() + u8"source-\u96ea.bin";
    const auto destination = directory.GetPath() + u8"destination-\u96ea.bin";
    ea::vector<unsigned char> contents(2 * 65536 + 17);
    for (unsigned i = 0; i < contents.size(); ++i)
        contents[i] = static_cast<unsigned char>(i % 251);
    const auto writeFile = [&](const ea::string& name, const ea::vector<unsigned char>& bytes)
    {
        File file(context, name, FILE_WRITE);
        REQUIRE(file.IsOpen());
        REQUIRE(file.Write(bytes.data(), bytes.size()) == bytes.size());
    };
    const auto readFile = [&](const ea::string& name)
    {
        File file(context, name, FILE_READ);
        REQUIRE(file.IsOpen());
        ea::vector<unsigned char> bytes(file.GetSize());
        REQUIRE(file.Read(bytes.data(), bytes.size()) == bytes.size());
        return bytes;
    };
    writeFile(source, contents);

    SECTION("Copy all bytes across buffer boundaries")
    {
        REQUIRE(fs->Copy(source, destination));
        CHECK(readFile(destination) == contents);
    }
    SECTION("Truncate an existing longer destination")
    {
        writeFile(destination, ea::vector<unsigned char>(contents.size() + 101, 0xff));
        REQUIRE(fs->Copy(source, destination));
        CHECK(readFile(destination) == contents);
    }
    SECTION("Copy an empty file and truncate the destination")
    {
        writeFile(source, {});
        writeFile(destination, contents);
        REQUIRE(fs->Copy(source, destination));
        CHECK(readFile(destination).empty());
    }
    SECTION("A missing source leaves the destination unchanged")
    {
        writeFile(destination, contents);
        CHECK_FALSE(fs->Copy(directory.GetPath() + "missing.bin", destination));
        CHECK(readFile(destination) == contents);
    }
    SECTION("A directory cannot be a destination")
    {
        CHECK_FALSE(fs->Copy(source, directory.GetPath()));
        CHECK(readFile(source) == contents);
    }
    SECTION("Reject a source outside the allowed path")
    {
        FileSystem restricted(context);
        const auto allowed = directory.GetPath() + "allowed/";
        REQUIRE(fs->CreateDir(allowed));
        restricted.RegisterPath(allowed);
        CHECK_FALSE(restricted.Copy(source, allowed + "copy.bin"));
        CHECK_FALSE(fs->FileExists(allowed + "copy.bin"));
    }
    SECTION("Reject a destination outside the allowed path")
    {
        FileSystem restricted(context);
        const auto allowed = directory.GetPath() + "allowed/";
        REQUIRE(fs->CreateDir(allowed));
        writeFile(allowed + "source.bin", contents);
        writeFile(destination, contents);
        restricted.RegisterPath(allowed);
        CHECK_FALSE(restricted.Copy(allowed + "source.bin", destination));
        CHECK(readFile(destination) == contents);
    }
#ifdef __linux__
    SECTION("Report errors flushing a buffered write")
    {
        REQUIRE(fs->FileExists("/dev/full"));
        writeFile(source, ea::vector<unsigned char>(15, 0x5a));
        CHECK_FALSE(fs->Copy(source, "/dev/full"));
    }
#endif
}

TEST_CASE("FileSystem copies files larger than 2 GiB", "[.][large-file]")
{
    auto context = Tests::GetOrCreateContext(CreateFileSystemContext);
    auto fs = context->GetSubsystem<FileSystem>();
    TemporaryDir directory(context, GetCopyTestDirectory(fs));
    REQUIRE(fs->DirExists(directory.GetPath()));
    const auto source = directory.GetPath() + "source.bin";
    const auto destination = directory.GetPath() + "destination.bin";
    const auto fileSize = GENERATE((std::uint64_t{1} << 31) + 65537, (std::uint64_t{1} << 32) + 65537);
    const std::array<std::uint64_t, 6> markerOffsets{
        0, 65535, 65536, (std::uint64_t{1} << 31) - 1, std::uint64_t{1} << 31, fileSize - 1};
    {
        std::ofstream file(source.c_str(), std::ios::binary);
        REQUIRE(file.is_open());
        for (unsigned i = 0; i < markerOffsets.size(); ++i)
        {
            file.seekp(static_cast<std::streamoff>(markerOffsets[i]));
            file.put(static_cast<char>(i + 1));
            REQUIRE(file.good());
        }
        file.close();
        REQUIRE(file.good());
    }
    REQUIRE(fs->Copy(source, destination));
    REQUIRE(std::filesystem::file_size(destination.c_str()) == fileSize);
    std::ifstream original(source.c_str(), std::ios::binary);
    std::ifstream copied(destination.c_str(), std::ios::binary);
    REQUIRE(original.is_open());
    REQUIRE(copied.is_open());
    std::array<char, 65536> originalBytes{};
    std::array<char, 65536> copiedBytes{};
    std::uint64_t compared = 0;
    while (compared < fileSize)
    {
        original.read(originalBytes.data(), originalBytes.size());
        copied.read(copiedBytes.data(), copiedBytes.size());
        REQUIRE(original.gcount() > 0);
        REQUIRE(copied.gcount() == original.gcount());
        REQUIRE(std::equal(originalBytes.begin(), originalBytes.begin() + original.gcount(), copiedBytes.begin()));
        compared += original.gcount();
    }
    CHECK(compared == fileSize);
    CHECK(copied.peek() == std::ifstream::traits_type::eof());
}
