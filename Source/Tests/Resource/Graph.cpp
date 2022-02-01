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
TEST_CASE("Graph node id when added to graph")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    {
        auto node = MakeShared<GraphNode>(context);
        CHECK(node->GetID() == 0);
        CHECK(node->GetNumInputs() == 0);
        CHECK(!node->GetInput("x"));
        auto ref = node->GetOrAddInput("x");
        CHECK(ref);
        CHECK(node->GetNumInputs() == 1);
        CHECK(node->GetInput("x"));
        CHECK(node->GetInput(0));
        CHECK(!node->GetInput(1));
    }
    {
        auto node = MakeShared<GraphNode>(context);
        CHECK(node->GetID() == 0);
        CHECK(node->GetNumOutputs() == 0);
        CHECK(!node->GetOutput("x"));
        auto ref = node->GetOrAddOutput("x");
        CHECK(ref);
        CHECK(node->GetNumOutputs() == 1);
        CHECK(node->GetOutput("x"));
        CHECK(node->GetOutput(0));
        CHECK(!node->GetOutput(1));
    }
    {
        auto node = MakeShared<GraphNode>(context);
        CHECK(node->GetID() == 0);
        CHECK(node->GetNumEnters() == 0);
        CHECK(!node->GetEnter("x"));
        auto ref = node->GetOrAddEnter("x");
        CHECK(ref);
        CHECK(node->GetNumEnters() == 1);
        CHECK(node->GetEnter("x"));
        CHECK(node->GetEnter(0));
        CHECK(!node->GetEnter(1));
    }
    {
        auto node = MakeShared<GraphNode>(context);
        CHECK(node->GetID() == 0);
        CHECK(node->GetNumExits() == 0);
        CHECK(!node->GetExit("x"));
        auto ref = node->GetOrAddExit("x");
        CHECK(ref);
        CHECK(node->GetNumExits() == 1);
        CHECK(node->GetExit("x"));
        CHECK(node->GetExit(0));
        CHECK(!node->GetExit(1));
    }
}
TEST_CASE("Validate graph node id when added to graph")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

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
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto graph = MakeShared<Graph>(context);
    REQUIRE(graph->LoadXML(
        R"(
        <graph>
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
                <node id="4294967294" name="Test2">
                    <properties>
                        <property name="p" type="Vector2" value="1 2" />
                    </properties>
                    <in>
                        <pin type="Vector3" name="in3" node="42" pin="out" />
                    </in>
                </node>
            </nodes>
        </graph>
    )"));

    auto node42 = graph->GetNode(42);
    REQUIRE(node42);
    CHECK(node42->GetName() == "Test");

    CHECK(node42->GetNumInputs() == 3);
    CHECK(node42->GetNumOutputs() == 1);
    CHECK(node42->GetNumEnters() == 1);
    CHECK(node42->GetNumExits() == 1);

    CHECK(node42->GetInput("").GetPin()->GetType() == VAR_NONE);
    CHECK(node42->GetInput("in2").GetPin()->GetType() == VAR_VECTOR2);
    CHECK(node42->GetInput("in3").GetPin()->GetType() == VAR_VECTOR3);
    CHECK(node42->GetInput("in3").GetPin()->GetValue().GetVector3() == Vector3(1, 2, 3));
    CHECK(node42->GetOutput("out").GetPin()->GetType() == VAR_VECTOR3);
    CHECK(node42->GetEnter("enter"));
    CHECK(node42->GetExit("exit"));

    auto lastNode = graph->GetNode(4294967294);
    REQUIRE(lastNode);
    CHECK(lastNode->GetName() == "Test2");
    REQUIRE(lastNode->GetProperty("p"));
    CHECK(lastNode->GetProperty("p")->GetVector2() == Vector2(1,2));
    CHECK(lastNode->GetInput("in3").GetPin()->GetType() == VAR_VECTOR3);
    REQUIRE(lastNode->GetInput("in3").GetPin()->IsConnected());
    REQUIRE(lastNode->GetInput("in3").GetConnectedPin<GraphOutPin>());
    CHECK(lastNode->GetInput("in3").GetConnectedPin<GraphOutPin>().GetPin()->GetName() == "out");
}

TEST_CASE("Graph serialization roundtrip")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

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

    auto out = nodeA->GetOrAddOutput("out");
    auto enter = nodeC->GetOrAddEnter("enter");

    CHECK(nodeB->GetOrAddInput("in").GetPin()->ConnectTo(out));
    nodeB->GetOrAddOutput("out");
    nodeB->GetOrAddEnter("enter");
    CHECK(nodeB->GetOrAddExit("exit").GetPin()->ConnectTo(enter));

    // Save to xml
    VectorBuffer buf;
    {
        SharedPtr<XMLFile> xml(context->CreateObject<XMLFile>());
        REQUIRE(xml->SaveObject(*graph));
        REQUIRE(xml->Save(buf));
    }

    auto restoredGraph = MakeShared<Graph>(context);
    buf.Seek(0);
    {
        SharedPtr<XMLFile> xml(context->CreateObject<XMLFile>());
        REQUIRE(xml->Load(buf));
        REQUIRE(xml->LoadObject(*restoredGraph));
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
