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

#include <Urho3D/Particles/ParticleGraphLayer.h>
#include <Urho3D/Particles/ParticleGraphLayerInstance.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Particles/ParticleGraphEffect.h>
#include <Urho3D/Particles/All.h>
#include <Urho3D/Scene/Scene.h>
#include <EASTL/variant.h>

using namespace Urho3D;

TEST_CASE("Test particle graph serialization")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

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
        graph->SerializeInBlock(xmlOutputArchive);
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
        outGraph->SerializeInBlock(xmlOutputArchive);
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
        outGraph->SerializeInBlock(xmlOutputArchive);
        xmlFile->Save(buf);
        ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());
    }
}

TEST_CASE("Test simple particle graph")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto resourceCache = context->GetSubsystem<ResourceCache>();
    auto material = MakeShared<Material>(context);
    material->SetName("Materials/DefaultGrey.xml");
    resourceCache->AddManualResource(material);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    effect->SetNumLayers(1);
    auto layer = effect->GetLayer(0);
    {
        auto& initGraph = layer->GetInitGraph();

        auto c = MakeShared<ParticleGraphNodes::Constant>(context);
        c->SetValue(Vector3(1,2,3));
        auto constIndex = initGraph.Add(c);

        auto set = MakeShared<ParticleGraphNodes::SetAttribute>(context);
        set->SetAttributeName("pos");
        set->SetAttributeType(VAR_VECTOR3);
        set->SetPinSource(set->GetPinIndex(""), constIndex);
        auto setIndex = initGraph.Add(set);
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

        auto frame = MakeShared<ParticleGraphNodes::Constant>(context);
        frame->SetValue(0.0f);
        auto frameIndex = updateGraph.Add(frame);

        auto rotation = MakeShared<ParticleGraphNodes::Constant>(context);
        rotation->SetValue(0.0f);
        auto rotationIndex = updateGraph.Add(rotation);
        

        auto color = MakeShared<ParticleGraphNodes::Constant>(context);
        color->SetValue(Color(1.0f, 1.0f, 1.0f, 1.0f));
        auto colorIndex = updateGraph.Add(color);

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

        auto direction = MakeShared<ParticleGraphNodes::Constant>(context);
        direction->SetValue(Vector3::UP);
        auto directionIndex = updateGraph.Add(direction);

        auto render = MakeShared<ParticleGraphNodes::RenderBillboard>(context);
        render->SetMaterial(ResourceRef(Material::GetTypeNameStatic(), "Materials/DefaultGrey.xml"));
        render->SetPinSource(0, curveIndex, 1);
        render->SetPinSource(1, constIndex);
        render->SetPinSource(2, frameIndex);
        render->SetPinSource(3, colorIndex);
        render->SetPinSource(4, rotationIndex);
        render->SetPinSource(5, directionIndex);
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
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
    <layers>
	    <layer capacity="10">
		    <emit>
			    <nodes>
			    </nodes>
		    </emit>
		    <init>
			    <nodes>
				    <node id="1" name="Constant">
					    <properties>
						    <property name="Value" type="Vector3" value="1 2 3" />
					    </properties>
					    <out>
						    <pin type="Vector3" name="out" />
					    </out>
				    </node>
				    <node id="2" name="SetAttribute">
					    <in>
						    <pin type="Vector3" name="" node="1" pin="out" />
					    </in>
					    <out>
						    <pin type="Vector3" name="pos" />
					    </out>
				    </node>
			    </nodes>
		    </init>
		    <update>
			    <nodes>
			    </nodes>
		    </update>
	    </layer>
    </layers>
</particleGraphEffect>)";
    MemoryBuffer buffer(xml);
    REQUIRE(effect->Load(buffer));
    REQUIRE(effect->GetLayer(0)->GetInitGraph().GetNumNodes() >= 2);

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

TEST_CASE("Test Emit")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
    <layers>
	    <layer type="ParticleGraphLayer" capacity="10">
		    <emit>
			    <nodes>
    			    <node id="1" name="Emit">
					    <in>
						    <pin name="count" type="float" value="1" />
					    </in>
				    </node>
			    </nodes>
		    </emit>
		    <init>
			    <nodes>
			    </nodes>
		    </init>
		    <update>
			    <nodes>
			    </nodes>
		    </update>
	    </layer>
    </layers>
</particleGraphEffect>)";
    MemoryBuffer buffer(xml);
    REQUIRE(effect->Load(buffer));

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);

    Tests::RunFrame(context, 0.1f, 0.1f);

    CHECK(emitter->CheckActiveParticles());
}

TEST_CASE("Test Burst")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
    <layers>
	    <layer type="ParticleGraphLayer" capacity="10">
		    <emit>
			    <nodes>
    			    <node id="1" name="BurstTimer">
	    			    <properties>
    	    			    <property name="Delay" type="float" value="0.15" />
    	    			    <property name="Interval" type="float" value="1.0" />
                            <property name="Cycles" type="int" value="2" />
	    			    </properties>
					    <in>
						    <pin name="count" type="float" value="1" />
					    </in>
					    <out>
						    <pin name="out" type="float" />
					    </out>
				    </node>
    			    <node id="2" name="Emit">
					    <in>
						    <pin name="count" type="float" node="1" pin="out" />
					    </in>
				    </node>
			    </nodes>
		    </emit>
		    <init>
			    <nodes>
			    </nodes>
		    </init>
		    <update>
			    <nodes>
			    </nodes>
		    </update>
	    </layer>
    </layers>
</particleGraphEffect>)";
    MemoryBuffer buffer(xml);
    REQUIRE(effect->Load(buffer));

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);

    Tests::RunFrame(context, 0.1f, 0.1f);
    CHECK(!emitter->CheckActiveParticles());

    Tests::RunFrame(context, 0.1f, 0.1f);
    CHECK(emitter->CheckActiveParticles());
}

TEST_CASE("Test Expire")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
    <layers>
	    <layer type="ParticleGraphLayer" capacity="10">
		    <emit>
			    <nodes>
			    </nodes>
		    </emit>
		    <init>
			    <nodes>
			    </nodes>
		    </init>
		    <update>
			    <nodes>
				    <node id="1" name="Expire">
					    <in>
						    <pin name="time" type="float" value="1" />
						    <pin name="lifetime" type="float" value="1" />
					    </in>
				    </node>
			    </nodes>
		    </update>
	    </layer>
    </layers>
</particleGraphEffect>)";
    MemoryBuffer buffer(xml);
    REQUIRE(effect->Load(buffer));

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);
    REQUIRE(emitter->EmitNewParticle(0));
    REQUIRE(emitter->EmitNewParticle(0));
    REQUIRE(emitter->EmitNewParticle(0));
    CHECK(emitter->GetLayer(0)->GetNumActiveParticles());

    Tests::RunFrame(context, 0.1f, 0.1f);

    CHECK(!emitter->GetLayer(0)->GetNumActiveParticles());
}

TEST_CASE("Test Make")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
    <layers>
	    <layer type="ParticleGraphLayer" capacity="10">
		    <emit>
			    <nodes>
			    </nodes>
		    </emit>
		    <init>
			    <nodes>
				    <node id="1" name="Make">
					    <in>
						    <pin name="x" type="float" value="2" />
						    <pin name="y" type="float" value="3" />
					    </in>
					    <out>
						    <pin name="out" type="Vector2" />
					    </out>
				    </node>
				    <node id="2" name="SetAttribute">
					    <in>
						    <pin name="" type="Vector2" node="1" pin="out" />
					    </in>
					    <out>
						    <pin name="attr" type="Vector2" />
					    </out>
				    </node>
			    </nodes>
		    </init>
		    <update>
			    <nodes>
			    </nodes>
		    </update>
	    </layer>
    </layers>
</particleGraphEffect>)";
    MemoryBuffer buffer(xml);
    REQUIRE(effect->Load(buffer));

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);
    REQUIRE(emitter->EmitNewParticle(0));
    REQUIRE(emitter->GetLayer(0)->GetNumAttributes() > 0);
    auto attributeSpan = emitter->GetLayer(0)->GetAttributeValues<Vector2>(0);
    CHECK(attributeSpan[0] == Vector2(2, 3));
}

TEST_CASE("Test Make IntVector2")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    const auto effect = MakeShared<ParticleGraphEffect>(context);
    auto xml = R"(<particleGraphEffect>
    <layers>
	    <layer type="ParticleGraphLayer" capacity="10">
		    <emit>
			    <nodes>
			    </nodes>
		    </emit>
		    <init>
			    <nodes>
				    <node id="1" name="Make">
					    <in>
						    <pin name="x" type="int" value="2" />
						    <pin name="y" type="int" value="3" />
					    </in>
					    <out>
						    <pin name="out" type="IntVector2" />
					    </out>
				    </node>
				    <node id="2" name="SetAttribute">
					    <in>
						    <pin name="" type="IntVector2" node="1" pin="out" />
					    </in>
					    <out>
						    <pin name="attr" type="IntVector2" />
					    </out>
				    </node>
			    </nodes>
		    </init>
		    <update>
			    <nodes>
			    </nodes>
		    </update>
	    </layer>
    </layers>
</particleGraphEffect>)";
    MemoryBuffer buffer(xml);
    REQUIRE(effect->Load(buffer));

    const auto scene = MakeShared<Scene>(context);
    const auto node = scene->CreateChild();
    auto emitter = node->CreateComponent<ParticleGraphEmitter>();
    emitter->SetEffect(effect);
    REQUIRE(emitter->EmitNewParticle(0));

    REQUIRE(emitter->GetLayer(0)->GetNumAttributes() > 0);
    auto attributeSpan = emitter->GetLayer(0)->GetAttributeValues<IntVector2>(0);
    CHECK(attributeSpan[0] == IntVector2(2, 3));
}
