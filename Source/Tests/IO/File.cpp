#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/PackageBuilder.h>
#include <Urho3D/IO/PackageFile.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

namespace
{
    SharedPtr<Context> CreateTestContext()
    {
        auto context = MakeShared<Context>();
        context->RegisterSubsystem<FileSystem>();
        return context;
    }

    struct TmpFile
    {
        TmpFile(Context* context)
        {
            fileSystem_ = context->GetSubsystem<FileSystem>();
            auto tmpDir = fileSystem_->GetTemporaryDir();
            fileName_ = tmpDir + "/tmp";
        }

        ~TmpFile()
        {
            if (fileSystem_->Exists(fileName_))
                fileSystem_->Delete(fileName_);
        }

        FileSystem* fileSystem_;
        ea::string fileName_;
    };

    const static ea::string LongMessage("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat.Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur.Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
}

TEST_CASE("RawFile seek and read")
{
    SharedPtr<Context> context = CreateTestContext();

    TmpFile tmpFile(context);

    const unsigned messageSize = LongMessage.length();
    
    unsigned start = GENERATE(0, 100, 400, 442);
    unsigned read = GENERATE(0, 200, 442);
    INFO("Read " << read << " starting from " << start);
    unsigned expectedSize = ea::min(read, messageSize-start);
    { // Create RawFile.
        RawFile file;
        file.Open(tmpFile.fileName_, FILE_WRITE);

        file.Write(LongMessage.data(), LongMessage.length());
        file.Close();
    }

    { // Read it back
        RawFile file;
        file.Open(tmpFile.fileName_, FILE_READ);
        REQUIRE(file.GetSize() == messageSize);
        REQUIRE(file.GetPosition() == 0);
        file.Seek(start);
        REQUIRE(file.GetPosition() == start);
        std::vector<char> buffer;
        buffer.resize(read);
        unsigned size = file.Read(buffer.data(),read);
        REQUIRE(expectedSize == size);
        REQUIRE(file.GetPosition() == start+expectedSize);
        ea::string message(buffer.data(), buffer.data() + size);
        REQUIRE(message == LongMessage.substr(start, size));
    }

}


TEST_CASE("RawFile slice seek and read")
{
    SharedPtr<Context> context = CreateTestContext();

    TmpFile tmpFile(context);

    const unsigned messageSize = 200;

    unsigned start = GENERATE(0, 50, 150, 200);
    unsigned read = GENERATE(0, 100, 300);
    INFO("Read " << read << " starting from " << start);
    unsigned expectedSize = ea::min(read, messageSize - start);
    { // Create RawFile.
        RawFile file;
        file.Open(tmpFile.fileName_, FILE_WRITE);

        file.Write(LongMessage.data(), LongMessage.length());
        file.Close();
    }

    { // Read it back
        RawFile file;
        file.Open(tmpFile.fileName_, FILE_READ, 100, messageSize);
        REQUIRE(file.GetSize() == messageSize);
        REQUIRE(file.GetPosition() == 0);
        file.Seek(start);
        REQUIRE(file.GetPosition() == start);
        std::vector<char> buffer;
        buffer.resize(read);
        unsigned size = file.Read(buffer.data(), read);
        REQUIRE(expectedSize == size);
        REQUIRE(file.GetPosition() == start + expectedSize);
        ea::string message(buffer.data(), buffer.data() + size);
        REQUIRE(message == LongMessage.substr(start+100, size));
    }

}


TEST_CASE("File seek and read")
{
    SharedPtr<Context> context = CreateTestContext();

    TmpFile tmpFile(context);

    const unsigned messageSize = LongMessage.length();

    unsigned start = GENERATE(0, 100, 400, 442);
    unsigned read = GENERATE(0, 200, 442);
    INFO("Read " << read << " starting from " << start);
    unsigned expectedSize = ea::min(read, messageSize - start);
    { // Create File.
        File file(context);
        file.Open(tmpFile.fileName_, FILE_WRITE);

        file.Write(LongMessage.data(), LongMessage.length());
        file.Close();
    }

    { // Read it back
        File file(context);
        file.Open(tmpFile.fileName_, FILE_READ);
        REQUIRE(file.GetSize() == messageSize);
        REQUIRE(file.GetPosition() == 0);
        file.Seek(start);
        REQUIRE(file.GetPosition() == start);
        std::vector<char> buffer;
        buffer.resize(read);
        unsigned size = file.Read(buffer.data(), read);
        REQUIRE(expectedSize == size);
        REQUIRE(file.GetPosition() == start + expectedSize);
        ea::string message(buffer.data(), buffer.data() + size);
        REQUIRE(message == LongMessage.substr(start, size));
    }

}

