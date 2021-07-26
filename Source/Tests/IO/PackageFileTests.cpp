#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/PackageBuilder.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/IO/VectorBuffer.h>

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

    bool AppendMessage(PackageBuilder& builder, const ea::string& entryName, const ea::string& value)
    {
        ByteVector message(value.begin(), value.end());
        if (!builder.Append("EntryName", message))
            return false;
        return true;
    }

    bool RetrieveMessage(Context* context, PackageFile* packageFile, const ea::string& fileName, ea::string* value)
    {
        SharedPtr<File> packageContent = MakeShared<File>(context, packageFile, fileName);
        auto* entry = packageFile->GetEntry(fileName);
        value->resize(entry->size_);
        if (!packageContent->Read(value->data(), entry->size_))
            return false;
        return true;
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
}

TEST_CASE("Missing PackageFile")
{
    SharedPtr<Context> context = CreateTestContext();
    PackageFile packageFile(context.Get());
    REQUIRE_FALSE(packageFile.Open("MissingFile"));

}

TEST_CASE("Empty PackageFile")
{
    SharedPtr<Context> context = CreateTestContext();

    TmpFile tmpFile(context);
    PackageBuilder builder;
    {
        SharedPtr<File> pakFile = MakeShared<File>(context.Get(), tmpFile.fileName_, FILE_WRITE);
        REQUIRE(builder.Create(pakFile, GENERATE(false, true)));
        REQUIRE(builder.Build());
    }
    PackageFile packageFile(static_cast<Context*>(context));
    REQUIRE(packageFile.Open(tmpFile.fileName_));
}

TEST_CASE("Single entry PackageFile")
{
    SharedPtr<Context> context = CreateTestContext();

    TmpFile tmpFile(context);

    const ea::string testString = "Sample message";
    {
        SharedPtr<File> pakFile = MakeShared<File>(context.Get(), tmpFile.fileName_, FILE_WRITE);
        PackageBuilder builder;
        REQUIRE(builder.Create(pakFile, GENERATE(false, true)));
        REQUIRE(AppendMessage(builder, "EntryName", testString));
        REQUIRE(builder.Build());
    }
    PackageFile packageFile(context.Get());
    REQUIRE(packageFile.Open(tmpFile.fileName_));

    ea::string messageValue;
    REQUIRE(RetrieveMessage(context, &packageFile, "EntryName", &messageValue));
    REQUIRE(messageValue == testString);
}
