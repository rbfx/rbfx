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
#include "Urho3D/IO/VectorBuffer.h"

#include <Urho3D/IO/ConfigFile.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/JSONFile.h>

TEST_CASE("Load config file from JSON")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    JSONFile jsonFile(context);
    MemoryBuffer buffer(R"({"key0":3, "key2":6})");
    CHECK(jsonFile.Load(buffer));

    ConfigFile configFile(context);
    configFile.SetDefaultValue("key0", 1);
    configFile.SetDefaultValue("key1", 2);
    CHECK(configFile.LoadJSON(jsonFile.GetRoot()));

    CHECK(configFile.GetValue("key0").GetInt() == 3);
    CHECK(configFile.GetValue("key1").GetInt() == 2);
    CHECK(configFile.GetValue("key2").GetInt() == 0);
}

TEST_CASE("Save config file to JSON")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    ConfigFile configFile(context);
    configFile.SetDefaultValue("key0", 1);
    configFile.SetDefaultValue("key1", 2);
    CHECK(configFile.SetValue("key0", 3));
    CHECK(!configFile.SetValue("key2", 6));
    JSONValue value;
    CHECK(configFile.SaveJSON(value));

    configFile.Clear();
    CHECK(configFile.LoadJSON(value));
    CHECK(configFile.GetValue("key0").GetInt() == 3);
    CHECK(configFile.GetValue("key1").GetInt() == 2);
    CHECK(configFile.GetValue("key2").GetInt() == 0);
}


TEST_CASE("Load config file from XML")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    XMLFile xmlFile(context);
    MemoryBuffer buffer(R"(<Values key0="3" key2="6"/>)");
    CHECK(xmlFile.Load(buffer));

    ConfigFile configFile(context);
    configFile.SetDefaultValue("key0", 1);
    configFile.SetDefaultValue("key1", 2);
    CHECK(configFile.LoadXML(xmlFile.GetRoot()));

    CHECK(configFile.GetValue("key0").GetInt() == 3);
    CHECK(configFile.GetValue("key1").GetInt() == 2);
    CHECK(configFile.GetValue("key2").GetInt() == 0);
}

TEST_CASE("Save config file to XML")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    XMLFile xmlFile(context);

    ConfigFile configFile(context);
    configFile.SetDefaultValue("key0", 1);
    configFile.SetDefaultValue("key1", 2);
    CHECK(configFile.SetValue("key0", 3));
    CHECK(!configFile.SetValue("key2", 6));
    CHECK(configFile.SaveXML(xmlFile.CreateRoot("Settings")));

    configFile.Clear();
    CHECK(configFile.LoadXML(xmlFile.GetRoot()));
    CHECK(configFile.GetValue("key0").GetInt() == 3);
    CHECK(configFile.GetValue("key1").GetInt() == 2);
    CHECK(configFile.GetValue("key2").GetInt() == 0);
}

//TEST_CASE("Save config file to binarry")
//{
//    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
//
//    VectorBuffer buffer;
//
//    ConfigFile configFile(context);
//    configFile.SetDefaultValue("key0", 1);
//    configFile.SetDefaultValue("key1", 2);
//    CHECK(configFile.SetValue("key0", 3));
//    CHECK(!configFile.SetValue("key2", 6));
//    CHECK(configFile.Save(buffer));
//
//    configFile.Clear();
//    buffer.Seek(0);
//
//    CHECK(configFile.Load(buffer));
//    CHECK(configFile.GetValue("key0").GetInt() == 3);
//    CHECK(configFile.GetValue("key1").GetInt() == 2);
//    CHECK(configFile.GetValue("key2").GetInt() == 0);
//}
