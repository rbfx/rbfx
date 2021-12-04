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
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/IO/VectorBuffer.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLArchive.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Scene/ValueAnimation.h"

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


TEST_CASE("Graph serialization roundtrip")
{
    auto context = Tests::CreateCompleteTestContext();
    auto resourceCache = context->GetSubsystem<ResourceCache>();
    auto material = MakeShared<Material>(context);
    material->SetName("Materials/DefaultGrey.xml");
    resourceCache->AddManualResource(material);


    auto graph = MakeShared<Graph>(context);
    auto nodeA = MakeShared<GraphNode>(context);
    auto nodeB = MakeShared<GraphNode>(context);
    auto nodeC = MakeShared<GraphNode>(context);

    graph->Add(nodeA);
    graph->Add(nodeB);
    graph->Add(nodeC);

    nodeA->GetProperties()["material"] = Variant(ResourceRef(StringHash("Material"), material->GetName()));
    //SharedPtr<ValueAnimation> valueAnim = MakeShared<ValueAnimation>(context);
    //valueAnim->SetKeyFrame(0.0f, Variant(0.0f));
    //valueAnim->SetKeyFrame(0.5f, Variant(1.0f));
    //valueAnim->SetKeyFrame(1.5f, Variant(0.0f));
    //nodeB->GetProperties()["spline"] = Variant(valueAnim);

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
    ea::string xmlText((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());

    auto restoredGraph = MakeShared<Graph>(context);
    buf.Seek(0);
    {
        SharedPtr<XMLFile> xml(context->CreateObject<XMLFile>());
        CHECK(xml->Load(buf));
        XMLInputArchive archive(xml);
        CHECK(restoredGraph->Serialize(archive));
    }
    CHECK(restoredGraph->GetNumNodes() == graph->GetNumNodes());
}
