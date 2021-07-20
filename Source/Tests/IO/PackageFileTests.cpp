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

    bool AppendMessage(PackageBuilder& builder, const ea::string& entryName, const ea::string& value)
    {
        ByteVector message(value.begin(), value.end());
        if (!builder.Append("EntryName", message))
            return false;
        return true;
    }

    bool RetrieveMessage(AbstractFile& packageContent, const PackageEntry* entry, ea::string* value)
    {
        if (!entry)
            return false;
        if (!packageContent.Seek(entry->offset_))
            return false;
        value->resize(entry->size_);
        if (!packageContent.Read(value->data(), entry->size_))
            return false;
        return true;
    }

    bool RetrieveMessage(AbstractFile& packageContent, const PackageFile& package, const ea::string& entryName, ea::string* value)
    {
        return RetrieveMessage(packageContent, package.GetEntry(entryName), value);
    }
}

TEST_CASE("Missing PackageFile")
{
    SharedPtr<Context> context = CreateTestContext();
    PackageFile packageFile(context.Get());
    CHECK_FALSE(packageFile.Open("MissingFile"));

}

TEST_CASE("Empty PackageFile")
{
    SharedPtr<Context> context = CreateTestContext();

    ByteVector pakBuffer; pakBuffer.resize(1024);
    MemoryBuffer pakFile(pakBuffer);
    PackageBuilder builder;
    CHECK(builder.Create(&pakFile, GENERATE(false,true)));
    CHECK(builder.Build());

    PackageFile packageFile(static_cast<Context*>(context));
    pakFile.Seek(0);
    CHECK(packageFile.Open(&pakFile));
}

TEST_CASE("Single entry PackageFile")
{
    SharedPtr<Context> context = CreateTestContext();

    const ea::string testString = "Sample message";
    ByteVector pakBuffer; pakBuffer.resize(1024);
    MemoryBuffer pakFile(pakBuffer);
    PackageBuilder builder;
    CHECK(builder.Create(&pakFile, GENERATE(false)));
    CHECK(AppendMessage(builder, "EntryName", testString));
    CHECK(builder.Build());

    PackageFile packageFile(context.Get());
    pakFile.Seek(0);
    CHECK(packageFile.Open(&pakFile));

    ea::string messageValue;
    CHECK(RetrieveMessage(pakFile, packageFile, "EntryName", &messageValue));
    CHECK(messageValue == testString);
}
