#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/PackageBuilder.h"

#include <Urho3D//IO/PackageFile.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>

#include <catch2/catch_amalgamated.hpp>
#include <utility>

using namespace Urho3D;

namespace
{
    SharedPtr<Context> CreateTestContext()
    {
        auto context = MakeShared<Context>();
        return context;
    }

    class TempFileHelper
    {
    public:
        TempFileHelper(SharedPtr<FileSystem> fileSystem, const ea::string& fileName):
            _fileSystem(std::move(fileSystem))
        {
            _fullPath = _fileSystem->GetTemporaryDir() + fileName;
        }
        ~TempFileHelper()
        {
            if (_fileSystem->Exists(_fullPath))
                _fileSystem->Delete(_fullPath);
        }
        const ea::string& GetPath() const { return _fullPath; };
    private:
        ea::string _fullPath;
        SharedPtr<FileSystem> _fileSystem;
    };
}

TEST_CASE("PackageFile")
{
    SharedPtr<Context> context = CreateTestContext();
    auto fileSystem = MakeShared<FileSystem>(context);
    auto temporaryDir = fileSystem->GetTemporaryDir();

    SECTION("Missing file returns false")
    {
        PackageFile packageFile(context.Get());
        CHECK_FALSE(packageFile.Open("MissingFile"));
    }

    SECTION("Empty uncompressed PAK")
    {
        ByteVector pakBuffer; pakBuffer.resize(1024);
        MemoryBuffer pakFile(pakBuffer);
        PackageBuilder builder;
        CHECK(builder.Create(pakFile, false));
        CHECK(builder.Build());

        CHECK(pakBuffer.size() > 0);

        PackageFile packageFile(static_cast<Context*>(context));
        pakFile.Seek(0);
        CHECK(packageFile.Open(pakFile));
    }

    SECTION("Empty compressed PAK")
    {
        ByteVector pakBuffer; pakBuffer.resize(1024);
        MemoryBuffer pakFile(pakBuffer);
        PackageBuilder builder;
        CHECK(builder.Create(pakFile, false));
        CHECK(builder.Build());

        CHECK(pakBuffer.size() > 0);

        PackageFile packageFile(static_cast<Context*>(context));
        pakFile.Seek(0);
        CHECK(packageFile.Open(pakFile));
    }

    SECTION("compressed PAK")
    {
        const ea::string testString = "Sample message";
        ByteVector message(testString.begin(), testString.end());
        ByteVector pakBuffer; pakBuffer.resize(1024);
        MemoryBuffer pakFile(pakBuffer);
        PackageBuilder builder;
        CHECK(builder.Create(pakFile, false));
        CHECK(builder.Append("EntryName", message));
        CHECK(builder.Build());

        PackageFile packageFile(context.Get());
        pakFile.Seek(0);
        CHECK(packageFile.Open(pakFile));
        File packageEntry(context.Get());
        const PackageEntry* entry = packageFile.GetEntry("EntryName");
        CHECK(entry);
        pakFile.Seek(entry->offset_);
        //pakFile.ReadBuffer();
        //CHECK(packageEntry.Open(&packageFile, "EntryName"));
        //ea::string messageValue = packageEntry.ReadText();
        //CHECK(messageValue == testString);
    }
}
