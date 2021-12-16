//
// Copyright (c) 2021 the rbfx project.
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
#include <Urho3D/Graphics/AnimationTrack.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLArchive.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Resource/Graph.h>
#include <Urho3D/Resource/GraphNode.h>

using namespace Urho3D;

TEST_CASE("Validate graph node id when added to scene")
{
    auto context = Tests::CreateCompleteTestContext();

    auto graph = MakeShared<Graph>(context);
    auto node = MakeShared<GraphNode>(context);
    CHECK(node->GetID() == 0);
    CHECK(node->GetGraph() == 0);

    graph->Add(node);
    CHECK(node->GetID() == Graph::FIRST_ID);
    CHECK(node->GetGraph() == graph);

    graph->Remove(node);
    CHECK(node->GetID() == Graph::FIRST_ID);
    CHECK(node->GetGraph() == 0);

    graph->Add(node);
    CHECK(node->GetID() == Graph::FIRST_ID);
    CHECK(node->GetGraph() == graph);

    auto node2 = MakeShared<GraphNode>(context);
    graph->Add(node2);
    CHECK(node2->GetID() == Graph::FIRST_ID+1);
    CHECK(node2->GetGraph() == graph);

    graph->Remove(node);
    graph->Remove(node2);
    graph->Add(node2);
    graph->Add(node);
    CHECK(node->GetID() == Graph::FIRST_ID);
    CHECK(node->GetGraph() == graph);
    CHECK(node2->GetID() == Graph::FIRST_ID + 1);
    CHECK(node2->GetGraph() == graph);

    auto graph2 = MakeShared<Graph>(context);
    auto node3 = MakeShared<GraphNode>(context);
    graph2->Add(node3);
    graph->Add(node3);
    CHECK(node3->GetID() == Graph::FIRST_ID + 2);
    CHECK(node3->GetGraph() == graph);

    graph->Add(node);
}

TEST_CASE("Test pins deserialization")
{
    auto context = Tests::CreateCompleteTestContext();
    auto graph = MakeShared<Graph>(context);
    REQUIRE(graph->LoadXML(
        R"(
        <nodes>
            <node id="42" name="Test">
				<in>
					<pin />
					<pin type="Vector2" name="in2" />
					<pin type="Vector3" name="in3" value="1 2 3" />
				</in>
				<out>
					<pin type="Vector3" name="out" />
				</out>
				<enter>
					<pin name="enter" />
				</enter>
				<exit>
					<pin name="exit" />
				</exit>
            </node>
        </nodes>
    )"));

    auto node = graph->GetNode(42);
    REQUIRE(node);

    REQUIRE(node->GetInputs().size() == 3);
    REQUIRE(node->GetOutputs().size() == 1);
    REQUIRE(node->GetEnters().size() == 1);
    REQUIRE(node->GetExits().size() == 1);

    CHECK(node->GetInput("")->GetType() == VAR_NONE);
    CHECK(node->GetInput("in2")->GetType() == VAR_VECTOR2);
    CHECK(node->GetInput("in3")->GetType() == VAR_VECTOR3);
    CHECK(node->GetInput("in3")->GetValue().GetVector3() == Vector3(1,2,3));
    CHECK(node->GetOutput("out")->GetType() == VAR_VECTOR3);
    CHECK(node->GetEnter("enter"));
    CHECK(node->GetExit("exit"));
}

TEST_CASE("Graph serialization roundtrip")
{
    auto context = Tests::CreateCompleteTestContext();

    auto resourceCache = context->GetSubsystem<ResourceCache>();
    auto material = MakeShared<Material>(context);
    material->SetName("Materials/DefaultGrey.xml");
    resourceCache->AddManualResource(material);


    auto graph = MakeShared<Graph>(context);

    auto nodeA = graph->Create("A");
    auto nodeB = graph->Create("B");
    auto nodeC = graph->Create("C");

    nodeA->GetOrAddProperty("material") = Variant(ResourceRef(StringHash("Material"), material->GetName()));
    VariantCurve track;
    track.AddKeyFrame(VariantAnimationKeyFrame{0.0f, 0.5f});
    track.AddKeyFrame(VariantAnimationKeyFrame{1.0f, 1.0f});
    track.Commit();
    nodeC->GetOrAddProperty("spline") = track;

    auto& out = nodeA->GetOrAddOutput("out");
    auto& enter = nodeC->GetOrAddEnter("enter");

    CHECK(nodeB->GetOrAddInput("in").ConnectTo(out));
    nodeB->GetOrAddOutput("out");
    nodeB->GetOrAddEnter("enter");
    CHECK(nodeB->GetOrAddExit("exit").ConnectTo(enter));

    // Save to xml
    VectorBuffer buf;
    {
        SharedPtr<XMLFile> xml(context->CreateObject<XMLFile>());
        XMLElement graphElem = xml->CreateRoot("graph");
        XMLOutputArchive archive(xml);
        CHECK(graph->Serialize(archive));
        xml->Save(buf);
    }

    auto restoredGraph = MakeShared<Graph>(context);
    buf.Seek(0);
    {
        SharedPtr<XMLFile> xml(context->CreateObject<XMLFile>());
        CHECK(xml->Load(buf));
        XMLInputArchive archive(xml);
        CHECK(restoredGraph->Serialize(archive));
    }
    CHECK(restoredGraph->GetNumNodes() == graph->GetNumNodes());
    ea::vector<unsigned> ids;
    graph->GetNodeIds(ids);
    for (unsigned id: ids)
    {
        auto srcNode = graph->GetNode(id);
        CHECK(srcNode);
        auto dstNode = restoredGraph->GetNode(id);
        CHECK(dstNode);
        CHECK(srcNode->GetName() == dstNode->GetName());
    }
}
