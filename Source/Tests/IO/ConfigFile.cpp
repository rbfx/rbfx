//
// Copyright (c) 2022-2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

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
