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
#include "../ModelUtils.h"
#include "Urho3D/Resource/XMLArchive.h"

#include <Urho3D/Particles//ParticleGraphLayer.h>
#include <Urho3D/Particles/ParticleGraphLayerInstance.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Particles/ParticleGraphEffect.h>
#include <Urho3D/Particles/All.h>
#include <Urho3D/Scene/Scene.h>
#include <EASTL/span.h>

using namespace Urho3D;

//TEST_CASE("Test Add")
//{
//    auto context = Tests::CreateCompleteTestContext();
//
//    ParticleGraphNodes::AddFloat add;
//    auto& pin0 = add.GetPin(0);
//    pin0.containerType_ = ParticleGraphContainerType::Span;
//    pin0.offset_ = sizeof(float)*0;
//    pin0.size_ = sizeof(float);
//    auto& pin1 = add.GetPin(1);
//    pin1.containerType_ = ParticleGraphContainerType::Span;
//    pin1.offset_ = sizeof(float) * 1;
//    pin1.size_ = sizeof(float);
//    auto& pin2 = add.GetPin(2);
//    pin2.containerType_ = ParticleGraphContainerType::Span;
//    pin2.offset_ = sizeof(float) * 2;
//    pin2.size_ = sizeof(float);
//
//    ea::vector<uint8_t> buffer;
//    buffer.resize(add.EvalueInstanceSize());
//
//    UpdateContext updateContext;
//    ea::vector<unsigned> indices {0};
//    ea::vector<uint8_t> values;
//    values.resize(sizeof(float) * 3);
//    updateContext.indices_ = ea::span<unsigned>(indices.begin(), indices.end());
//    updateContext.timeStep_ = 0.0f;
//    updateContext.tempBuffer_ = ea::span<uint8_t>(values.begin(), values.end());
//
//    updateContext.GetSpan<float>(pin0)[0] = 40.0f;
//    updateContext.GetSpan<float>(pin1)[0] = 2.0f;
//
//    auto* instance = add.CreateInstanceAt(buffer.begin());
//    instance->Update(updateContext);
//
//    CHECK(updateContext.GetSpan<float>(pin2)[0] == Catch::Approx(42.0f));
//
//    instance->~ParticleGraphNodeInstance();
//}
TEST_CASE("Test particle graph serialization")
{
    auto context = Tests::CreateCompleteTestContext();

    const auto graph = MakeShared<Graph>(context);

    auto const1 = graph->Create("Constant")->WithProperty("value", 1.0f)->WithOutput("out");
    auto add = graph->Create("Add")->
        WithInput("x", 0.5f)->
        WithInput("y", const1->GetOutput("out"));

    const auto particleGraph = MakeShared<ParticleGraph>(context);
    CHECK(particleGraph->LoadGraph(*graph));

    const auto outGraph = MakeShared<Graph>(context);
    CHECK(particleGraph->SaveGraph(*outGraph));

    {
        VectorBuffer buf;
        auto xmlFile = MakeShared<XMLFile>(context);
        XMLElement root = xmlFile->CreateRoot("root");
        XMLOutputArchive xmlOutputArchive{context, root};
        outGraph->Serialize(xmlOutputArchive);
        xmlFile->Save(buf);
        ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());
    }
}

TEST_CASE("Test simple particle graph")
{
    auto context = Tests::CreateCompleteTestContext();
    auto resourceCache = context->GetSubsystem<ResourceCache>();
    auto material = MakeShared<Material>(context);
    material->SetName("Materials/DefaultGrey.xml");
    resourceCache->AddManualResource(material);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    effect->SetNumLayers(1);
    auto layer = effect->GetLayer(0);
    {
        auto& emitGraph = layer->GetEmitGraph();

        auto c = MakeShared<ParticleGraphNodes::Constant>(context);
        c->SetValue(Vector3(1,2,3));
        auto constIndex = emitGraph.Add(c);

        auto set = MakeShared<ParticleGraphNodes::SetAttribute>(context);
        set->SetPinSource(0, constIndex);
        set->SetAttributeName("pos");
        set->SetAttributeType(VAR_VECTOR3);
        auto setIndex = emitGraph.Add(set);
    }
    {
        auto& updateGraph = layer->GetUpdateGraph();
        auto get = MakeShared<ParticleGraphNodes::GetAttribute>(context);
        get->SetAttributeName("pos");
        get->SetAttributeType(VAR_VECTOR3);
        auto getIndex = updateGraph.Add(get);

        auto c = MakeShared<ParticleGraphNodes::Constant>(context);
        c->SetValue(Vector2(1, 2));
        auto constIndex = updateGraph.Add(c);


        auto log = MakeShared<ParticleGraphNodes::Print>(context);
        log->SetPinSource(0, getIndex);
        auto logIndex = updateGraph.Add(log);

        auto render = MakeShared<ParticleGraphNodes::RenderBillboard>(context);
        render->SetMaterial(material);
        render->SetPinSource(0 ,getIndex);
        render->SetPinSource(1, constIndex);
        auto renderIndex = updateGraph.Add(render);

    }

    VectorBuffer buf;
    effect->Save(buf);
    ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());


    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);
    CHECK(emitter->EmitNewParticle(0));

    Tests::RunFrame(context, 0.1f, 0.1f);
    //emitter->Tick(0.1f);

    auto l = emitter->GetLayer(0);
    //auto sizeMem = l->GetAttributeMemory(0);
    //CHECK(((float*)sizeMem.data())[0] == 40.0);
}
