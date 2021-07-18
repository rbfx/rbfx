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
        auto packageFile = MakeShared<PackageFile>(context);
        CHECK_FALSE(packageFile->Open("MissingFile"));
    }

    SECTION("Empty uncompressed PAK")
    {
        TempFileHelper file(fileSystem, "empty.pak");
        ea::vector<SharedPtr<File>> files;
        PackageFile::CreatePackage(context, file.GetPath(), files);

        auto packageFile = MakeShared<PackageFile>(context);
        CHECK(packageFile->Open(file.GetPath()));
    }

    SECTION("Empty compressed PAK")
    {
        TempFileHelper file(fileSystem, "empty.pak");
        ea::vector<SharedPtr<File>> files;
        PackageFile::CreatePackage(context, file.GetPath(), files, true);

        auto packageFile = MakeShared<PackageFile>(context);
        CHECK(packageFile->Open(file.GetPath()));
    }

    SECTION("compressed PAK")
    {
        const ea::string testString = "Sample message";
        TempFileHelper file(fileSystem, "empty.pak");
        TempFileHelper message(fileSystem, "message");
        {
            auto messageFile = MakeShared<File>(context);
            messageFile->Open(message.GetPath(), FILE_WRITE);
            messageFile->WriteString(testString);
            messageFile->Close();
        }
        ea::vector<SharedPtr<File>> files;
        files.push_back(SharedPtr<File>(new File(context, message.GetPath())));
        
        PackageFile::CreatePackage(context, file.GetPath(), files, false);

        auto packageFile = MakeShared<PackageFile>(context);
        CHECK(packageFile->Open(file.GetPath()));
        {
            auto messageFile = MakeShared<File>(context);
            messageFile->Open(packageFile, message.GetPath());
            const ea::string messageFromFile = messageFile->ReadText();
            CHECK((messageFromFile == testString));
            messageFile->Close();
        }
    }
}
