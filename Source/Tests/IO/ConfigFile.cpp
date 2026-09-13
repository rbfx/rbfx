// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Engine/ConfigFile.h>
#include <Urho3D/IO/MountedExternalMemory.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Resource/JSONFile.h>

namespace
{

class TestFileSystem
{
public:
    TestFileSystem(Context* context)
        : fileSystem_(context->GetSubsystem<VirtualFileSystem>())
        , mountPoint_(MakeShared<MountedExternalMemory>(context, "memory"))
    {
        fileSystem_->Mount(mountPoint_);
    }

    ~TestFileSystem()
    {
        fileSystem_->Unmount(mountPoint_);
    }

    void AddFile(ea::string_view fileName, MemoryBuffer memory)
    {
        mountPoint_->LinkMemory(fileName, memory);
    }

    void AddFile(ea::string_view fileName, const ea::string& content)
    {
        mountPoint_->LinkMemory(fileName, content);
    }

private:
    WeakPtr<VirtualFileSystem> fileSystem_;
    SharedPtr<MountedExternalMemory> mountPoint_;
};

const ea::string configDefaults = R"({
    "Default": [
        {
            "Flavor": [],
            "Variables": [
                {
                    "key": "FullScreen",
                    "type": "Bool",
                    "value": true
                },
                {
                    "key": "Plugins",
                    "type": "String",
                    "value": "SampleProject;TestPlugin"
                },
                {
                    "key": "MainPlugin",
                    "type": "String",
                    "value": "SampleProject"
                }
            ]
        }
    ]
})";

const ea::string configOverrides = R"({
    "FullScreen": {
        "type": "Bool",
        "value": false
    }
})";

}

TEST_CASE("ConfigFile is loaded from JSON with optional overrides")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    TestFileSystem fileSystem{context};

    fileSystem.AddFile("ConfigDefaults.json", configDefaults);
    fileSystem.AddFile("ConfigOverrides.json", configOverrides);

    ConfigFile configFile(context);
    configFile.DefineVariable("FullScreen", true).Overridable();

    CHECK(configFile.GetVariable("FullScreen") == Variant{true});
    CHECK(configFile.GetVariable("Plugins") == Variant::EMPTY);
    CHECK(configFile.GetVariable("MainPlugin") == Variant::EMPTY);

    REQUIRE(configFile.LoadDefaults("memory://ConfigDefaults.json", ApplicationFlavor::Universal));

    CHECK(configFile.GetVariable("FullScreen") == Variant{true});
    CHECK(configFile.GetVariable("Plugins") == Variant{"SampleProject;TestPlugin"});
    CHECK(configFile.GetVariable("MainPlugin") == Variant{"SampleProject"});

    REQUIRE(configFile.LoadOverrides("memory://ConfigOverrides.json"));

    CHECK(configFile.GetVariable("FullScreen") == Variant{false});
    CHECK(configFile.GetVariable("Plugins") == Variant{"SampleProject;TestPlugin"});
    CHECK(configFile.GetVariable("MainPlugin") == Variant{"SampleProject"});

    const auto overrides = configFile.GetChangedVariables(ApplicationFlavor::Universal);

    REQUIRE(overrides.size() == 1);
    CHECK(overrides.begin()->first == "FullScreen");
    CHECK(overrides.begin()->second == Variant{false});
}
