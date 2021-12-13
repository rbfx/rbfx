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
#include "Urho3D/IO/MemoryBuffer.h"
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

TEST_CASE("Test particle graph serialization")
{
    auto context = Tests::CreateCompleteTestContext();

    const auto graph = MakeShared<Graph>(context);

    auto const1 = graph->Create("Constant")->WithProperty("Value", 1.0f)->WithOutput("out");
    auto add = graph->Create("Add")->
        WithInput("x", 0.5f)->
        WithInput("y", const1->GetOutput("out"))->WithOutput("out");
    auto setAttr = graph->Create("SetAttribute")->WithInput("", add->GetOutput("out"), VAR_FLOAT)->WithOutput("attr", VAR_FLOAT);
    auto getAttr = graph->Create("GetAttribute")->WithOutput("attr1", VAR_FLOAT);

    {
        VectorBuffer buf;
        auto xmlFile = MakeShared<XMLFile>(context);
        XMLElement root = xmlFile->CreateRoot("root");
        XMLOutputArchive xmlOutputArchive{context, root};
        graph->Serialize(xmlOutputArchive);
        xmlFile->Save(buf);
        ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());
    }

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

    const auto restoredGraph = MakeShared<ParticleGraph>(context);
    CHECK(restoredGraph->LoadGraph(*outGraph));
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
        set->SetAttributeName("pos");
        set->SetAttributeType(VAR_VECTOR3);
        set->SetPinSource(set->GetPinIndex(""), constIndex);
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

        auto ct = MakeShared<ParticleGraphNodes::Constant>(context);
        ct->SetValue(0.0f);
        auto constTIndex = updateGraph.Add(ct);

        auto curve = MakeShared<ParticleGraphNodes::Curve>(context);
        VariantCurve variantCurve;
        variantCurve.AddKeyFrame(VariantCurvePoint{0.0f, Vector3(0,1,2)});
        curve->SetCurve(variantCurve);
        curve->SetPinSource(0, constTIndex);
        auto curveIndex = updateGraph.Add(curve);

        auto render = MakeShared<ParticleGraphNodes::RenderBillboard>(context);
        render->SetMaterial(material);
        render->SetPinSource(0, curveIndex, 1);
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

TEST_CASE("Test const")
{
    auto context = Tests::CreateCompleteTestContext();

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
	<layer type="ParticleGraphLayer" capacity="10">
		<emit>
			<nodes>
				<node id="1" name="Constant">
					<properties>
						<property name="Value">
							<value type="Vector3" value="1 2 3" />
						</property>
					</properties>
					<pins>
						<pin direction="Out" type="Vector3" name="out" />
					</pins>
				</node>
				<node id="2" name="SetAttribute">
					<pins>
						<pin type="Vector3" name="" node="1" pin="out" />
						<pin direction="Out" type="Vector3" name="pos" />
					</pins>
				</node>
			</nodes>
		</emit>
		<update>
			<nodes>
			</nodes>
		</update>
	</layer>
</particleGraphEffect>)";
    REQUIRE(effect->Load(MemoryBuffer(xml)));

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);
    REQUIRE(emitter->EmitNewParticle(0));

    Tests::RunFrame(context, 0.1f, 0.1f);

    REQUIRE(emitter->GetLayer(0)->GetNumAttributes() > 0);
    auto attributeSpan = emitter->GetLayer(0)->GetAttributeValues<Vector3>(0);
    CHECK(attributeSpan[0] == Vector3(1, 2, 3));
}

TEST_CASE("Test Expire")
{
    auto context = Tests::CreateCompleteTestContext();

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
	<layer type="ParticleGraphLayer" capacity="10">
		<emit>
			<nodes>
			</nodes>
		</emit>
		<update>
			<nodes>
				<node id="1" name="Expire">
					<pins>
						<pin name="time" type="float" value="1" />
						<pin name="lifetime" type="float" value="1" />
					</pins>
				</node>
			</nodes>
		</update>
	</layer>
</particleGraphEffect>)";
    REQUIRE(effect->Load(MemoryBuffer(xml)));

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);
    REQUIRE(emitter->EmitNewParticle(0));
    REQUIRE(emitter->EmitNewParticle(0));
    REQUIRE(emitter->EmitNewParticle(0));
    CHECK(emitter->GetLayer(0)->CheckActiveParticles());

    Tests::RunFrame(context, 0.1f, 0.1f);

    CHECK(!emitter->GetLayer(0)->CheckActiveParticles());
}
