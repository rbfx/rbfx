//
// Copyright (c) 2017-2021 the rbfx project.
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
#include "../SceneUtils.h"

#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/IO/MemoryBuffer.h>

TEST_CASE("Scene lookup")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto child0 = scene->CreateChild("Child_0");
    auto child00 = child0->CreateChild("Child_0_0");
    auto child000 = child00->CreateChild("Child_0_0_0");
    auto child01 = child0->CreateChild("Child_0_1");
    auto child1 = scene->CreateChild("Child_1");
    auto child2 = scene->CreateChild("Child_2");
    auto child20 = child2->CreateChild("Child_2_0");

    child20->CreateComponent<StaticModel>();

    CHECK(scene->GetChild("Child_0", true) == child0);
    CHECK(scene->GetChild("Child_2_0", true) == child20);

    CHECK(scene->FindChild("Child_0/Child_0_0/Child_0_0_0") == child000);
    CHECK(scene->FindChild("#0/#0/#0") == child000);
    CHECK(scene->FindChild("Child_0/Child_0_1") == child01);
    CHECK(scene->FindChild("#0/#1") == child01);
    CHECK(scene->FindChild("Child_2") == child2);
    CHECK(scene->FindChild("#2") == child2);
    CHECK(scene->FindChild("Child_2/Child_2_0") == child20);
    CHECK(scene->FindChild("#2/#0") == child20);

    CHECK(Tests::GetAttributeValue(child20->FindComponentAttribute("@/Name")) == Variant(child20->GetName()));
    CHECK(Tests::GetAttributeValue(child20->FindComponentAttribute("@StaticModel/LOD Bias")) == Variant(1.0f));
}

TEST_CASE("Scene LoadXML from SceneResource")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    MemoryBuffer xml{R"(<resource _id="1">
	<nodes>
		<node _id="3">
		</node>
	</nodes>
</resource>)"};

    auto scene = MakeShared<Scene>(context);
    REQUIRE(scene->LoadXML(xml));

    REQUIRE(1 == scene->GetNumChildren());
}

TEST_CASE("Scene LoadXML from old XML")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    MemoryBuffer xml{R"(<scene>
		<node>
		</node>
</scene>)"};

    auto scene = MakeShared<Scene>(context);
    REQUIRE(scene->LoadXML(xml));

    REQUIRE(1 == scene->GetNumChildren());
}

//TODO: Figure out how to make this test succeed
//TEST_CASE("Scene LoadXML from incorrect XML returns false")
//{
//    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
//    MemoryBuffer xml{R"(<unknown/>)"};
//
//    auto scene = MakeShared<Scene>(context);
//    REQUIRE(!scene->LoadXML(xml));
//}
//
//TEST_CASE("Scene LoadXML from incorrect resource returns false")
//{
//    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
//    MemoryBuffer xml{R"(<resource><resource></resource></resource>)"};
//
//    auto scene = MakeShared<Scene>(context);
//    REQUIRE(!scene->LoadXML(xml));
//}
