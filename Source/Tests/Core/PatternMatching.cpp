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
#include <Urho3D/PatternMatching/PatternIndex.h>
#include <Urho3D/PatternMatching/PatternQuery.h>
#include <Urho3D/PatternMatching/PatternCollection.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/XMLArchive.h>

#include <EASTL/variant.h>

using namespace Urho3D;

TEST_CASE("PatternMatching query")
{
    PatternCollection patternCollection;
    PatternIndex index;
    index.Build(&patternCollection);

    PatternQuery query;
    CHECK(-1 == index.Query(query));

    auto expectedTwoKeys = patternCollection.BeginPattern();
    patternCollection.AddKey("TestKey");
    patternCollection.AddKey("MinMax", -2, 2);
    patternCollection.CommitPattern();
    auto expectedOneKey = patternCollection.BeginPattern();
    patternCollection.AddKey("TestKey");
    patternCollection.CommitPattern();
    index.Build(&patternCollection);

    query.SetKey("MinMax", 1.5f);
    query.Commit();
    CHECK(-1 == index.Query(query));

    query.SetKey("TestKey", 1.5f);
    query.Commit();
    CHECK(expectedTwoKeys == index.Query(query));
    query.SetKey("MinMax", 2.5f);
    query.Commit();
    CHECK(expectedOneKey == index.Query(query));

    query.RemoveKey("TestKey");
    query.Commit();
    CHECK(-1 == index.Query(query));

    auto expectedEmpty = patternCollection.BeginPattern();
    patternCollection.CommitPattern();
    index.Build(&patternCollection);
    CHECK(expectedEmpty == index.Query(query));

    query.Clear();
    query.Commit();
    CHECK(expectedEmpty == index.Query(query));
}

TEST_CASE("PatternMatching serilization")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    PatternCollection patternCollection;
    patternCollection.BeginPattern();
    patternCollection.AddKey("TestKey");
    patternCollection.AddKey("MinMax", -2, 2);
    {
        StringVariantMap args;
        args["PatternA"] = ResourceRef("Material", "Materials/DefaultMaterial.xml");
        patternCollection.AddEvent("MyEvent", args);
    }
    patternCollection.CommitPattern();
    patternCollection.BeginPattern();
    patternCollection.AddKey("TestKey");
    {
        StringVariantMap args;
        args["PatternB"] = 10;
        patternCollection.AddEvent("MyEvent", args);
    }
    patternCollection.CommitPattern();

    PatternQuery query;
    query.SetKey("TestKey");

    {
        VectorBuffer buf;
        auto xmlFile = MakeShared<XMLFile>(context);
        XMLElement root = xmlFile->CreateRoot("root");
        XMLOutputArchive xmlOutputArchive{context, root};
        auto block = xmlOutputArchive.OpenUnorderedBlock("root");
        patternCollection.SerializeInBlock(xmlOutputArchive);
        xmlFile->Save(buf);
        ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());
    }
}
TEST_CASE("PatternMatching deserilization")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto xml = R"(
<?xml version="1.0"?>
<root>
	<patterns>
		<pattern>
			<keys>
				<key key="TestKey" />
				<key key="MinMax" min="-2.000000" max="2.000000" />
			</keys>
			<events>
				<event name="MyEvent">
					<args>
						<element key="PatternA" type="ResourceRef" value="Material;Materials/DefaultMaterial.xml" />
					</args>
				</event>
			</events>
		</pattern>
		<pattern>
			<keys>
				<key key="TestKey" />
			</keys>
			<events>
				<event name="MyEvent">
					<args>
						<element key="PatternB" type="Int" value="10" />
					</args>
				</event>
			</events>
		</pattern>
	</patterns>
</root>
)";
    PatternCollection patternCollection;
    {
        XMLFile file(context);
        file.Load(MemoryBuffer(xml));
        XMLInputArchive archive(context, file.GetRoot());
        auto block = archive.OpenUnorderedBlock("root");
        patternCollection.SerializeInBlock(archive);
    }
    {
        VectorBuffer buf;
        auto xmlFile = MakeShared<XMLFile>(context);
        XMLElement root = xmlFile->CreateRoot("root");
        XMLOutputArchive xmlOutputArchive{context, root};
        auto block = xmlOutputArchive.OpenUnorderedBlock("root");
        patternCollection.SerializeInBlock(xmlOutputArchive);
        xmlFile->Save(buf);
        ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());
    }
}
