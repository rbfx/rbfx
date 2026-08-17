// SPDX-License-Identifier: MIT

#include <Urho3D/Audio/AudioMixer.h>
#include <Urho3D/Blueprint/BlueprintRuntime.h>
#include <Urho3D/Particles/VFXGraph.h>
#include <Urho3D/Shader/ShaderGraph.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

TEST_CASE("ShaderGraph validates connections and generates GLSL and HLSL", "[shader-graph]")
{
    ShaderGraph graph;
    REQUIRE(graph.SetParameter({"Tint", ShaderGraphValueType::Color, Variant(Color::WHITE)}));
    const unsigned parameter = graph.AddNode("Tint", ShaderGraphNodeKind::Parameter, ShaderGraphValueType::Color, Variant(ea::string("Tint")));
    const unsigned output = graph.AddNode("Output", ShaderGraphNodeKind::Output, ShaderGraphValueType::Color);
    REQUIRE(graph.Connect(parameter, "value", output, "color"));
    REQUIRE(graph.SetOutputNode(output));
    ea::string error;
    REQUIRE(graph.Validate(&error));
    const ea::string glsl = graph.GenerateGLSL(&error);
    const ea::string hlsl = graph.GenerateHLSL(&error);
    CHECK(glsl.find("uniform vec4 u_Tint") != ea::string::npos);
    CHECK(glsl.find("fragColor") != ea::string::npos);
    CHECK(hlsl.find("float4 u_Tint") != ea::string::npos);
    CHECK(hlsl.find("SV_Target") != ea::string::npos);
}

TEST_CASE("ShaderGraph rejects cycles and duplicate input connections", "[shader-graph]")
{
    ShaderGraph graph;
    const unsigned add = graph.AddNode("Add", ShaderGraphNodeKind::Add, ShaderGraphValueType::Float);
    const unsigned output = graph.AddNode("Output", ShaderGraphNodeKind::Output, ShaderGraphValueType::Float);
    REQUIRE(graph.Connect(add, "value", output, "color"));
    CHECK_FALSE(graph.Connect(add, "other", output, "color"));
    CHECK_FALSE(graph.Connect(add, "a", add, "b"));
    REQUIRE(graph.SetOutputNode(output));
    ea::string error;
    CHECK(graph.Validate(&error));
}

TEST_CASE("VFXGraph compiles, emits bounded particles and ribbon points", "[vfx-graph]")
{
    VFXGraph graph;
    const unsigned output = graph.AddNode("Output", VFXNodeType::Output);
    REQUIRE(graph.SetOutputNode(output));
    graph.SetSimulationMode(VFXSimulationMode::GPU);
    graph.SetMaxParticles(4);
    graph.SetSpawnRate(20.0f);
    graph.SetParticleLifetime(2.0f);
    graph.SetInitialVelocity(Vector3(1.0f, 0.0f, 0.0f));
    graph.SetForce(Vector3(0.0f, -1.0f, 0.0f));
    graph.SetRibbonTrailLength(3);
    REQUIRE(graph.Play());
    graph.Update(0.25f);
    CHECK(graph.GetSimulationMode() == VFXSimulationMode::GPU);
    CHECK(graph.GetParticles().size() == 4);
    CHECK(graph.GetRibbonPoints().size() == 1);
    graph.Update(0.25f);
    CHECK(graph.GetParticles().size() <= 4);
    CHECK(graph.GetRibbonPoints().size() <= 3);
    REQUIRE(graph.Stop());
}

TEST_CASE("AudioMixer routes voices through hierarchical buses and meters", "[audio-mixer]")
{
    AudioMixer mixer;
    REQUIRE(mixer.AddBus({"Master", {}, 0.8f, false, false, {}}));
    REQUIRE(mixer.AddBus({"SFX", "Master", 0.5f, false, false, {}}));
    REQUIRE(mixer.AddEffect("SFX", {AudioDspType::Compressor, true, 0.75f, 2.0f, 0.1f}));
    REQUIRE(mixer.AddVoice({"shot", "SFX", 1.0f, 0.0f, 1.0f, 1.0f, true}));
    CHECK(mixer.GetEffectiveBusVolume("SFX") == Catch::Approx(0.4f));
    mixer.Update(0.016f);
    const AudioMeter meter = mixer.GetBusMeter("SFX");
    CHECK(meter.activeVoices == 1);
    CHECK(meter.peak == Catch::Approx(0.4f));
    REQUIRE(mixer.SetBusMuted("SFX", true));
    CHECK(mixer.GetEffectiveBusVolume("SFX") == 0.0f);
    REQUIRE(mixer.SetBusMuted("SFX", false));
    REQUIRE(mixer.SetBusSoloed("SFX", true));
    CHECK(mixer.GetEffectiveBusVolume("Master") == 0.0f);
}

TEST_CASE("Blueprint runtime exposes ShaderGraph, VFXGraph and AudioMixer nodes", "[production][blueprint]")
{
    BlueprintRuntime runtime;
    ShaderGraph shaderGraph;
    VFXGraph vfxGraph;
    AudioMixer audioMixer;
    runtime.SetShaderGraph(&shaderGraph);
    runtime.SetVFXGraph(&vfxGraph);
    runtime.SetAudioMixer(&audioMixer);
    CHECK(runtime.GetShaderGraph() == &shaderGraph);
    CHECK(runtime.GetVFXGraph() == &vfxGraph);
    CHECK(runtime.GetAudioMixer() == &audioMixer);
    CHECK(runtime.GetRegistry().Find("Shader.SetGraphParameter") != nullptr);
    CHECK(runtime.GetRegistry().Find("VFX.Play") != nullptr);
    CHECK(runtime.GetRegistry().Find("VFX.Stop") != nullptr);
    CHECK(runtime.GetRegistry().Find("Audio.SetBusVolume") != nullptr);
}
