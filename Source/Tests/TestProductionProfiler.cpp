#include <Urho3D/Blueprint/BlueprintRuntime.h>
#include <Urho3D/Profiler/ProductionProfiler.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

TEST_CASE("Production profiler captures hierarchical CPU scopes", "[profiler][cpu]")
{
    ProductionProfiler profiler(4);
    profiler.BeginFrame(7);
    profiler.RecordScope("Frame", 3.0, 0);
    profiler.RecordScope("Gameplay", 1.5, 1);
    profiler.RecordCounter("DrawCalls", 42.0);
    profiler.EndFrame();

    REQUIRE(profiler.GetHistory().size() == 1);
    CHECK(profiler.GetHistory().front().frameIndex == 7);
    CHECK(profiler.GetHistory().front().scopes.size() == 2);
    CHECK(profiler.GetHistory().front().scopes[1].depth == 1);
    CHECK(profiler.GetHistory().front().counters.at("DrawCalls") == 42.0);

    const ProfilerReport report = profiler.BuildReport();
    REQUIRE(report.frameCount == 1);
    CHECK(report.averageFrameMilliseconds >= 0.0);
    REQUIRE(report.scopes.size() == 2);
    CHECK(report.scopes[0].name == "Frame");
    CHECK(report.scopes[0].calls == 1);
    CHECK(report.scopes[0].totalMilliseconds == 3.0);
}

TEST_CASE("Production profiler bounds history and aggregates production metrics", "[profiler][report]")
{
    ProductionProfiler profiler(2);
    for (unsigned long long frame = 1; frame <= 3; ++frame)
    {
        profiler.BeginFrame(frame);
        profiler.RecordScope("Tick", static_cast<double>(frame), 0);
        profiler.RecordGpuPass("ToneMapping", 0.5);
        profiler.RecordScriptFunction("Player::Tick", 0.25);
        profiler.EndFrame();
    }

    profiler.TrackAllocation("Textures", 1024);
    profiler.TrackAllocation("Textures", 512);
    profiler.TrackFree("Textures", 256);
    profiler.RecordNetwork("Player1", 100, 200, 1, 34.0);
    profiler.RecordAudio("Master", 4, 48000, 1.25);
    profiler.RecordAudio("Master", 2, 24000, 0.75);

    REQUIRE(profiler.GetHistory().size() == 2);
    CHECK(profiler.GetHistory().front().frameIndex == 2);
    const ProfilerReport report = profiler.BuildReport();
    CHECK(report.frameCount == 2);
    REQUIRE(report.gpuPasses.size() == 1);
    CHECK(report.gpuPasses.front().calls == 3);
    REQUIRE(report.scriptFunctions.size() == 1);
    CHECK(report.scriptFunctions.front().calls == 3);
    REQUIRE(report.memory.size() == 1);
    CHECK(report.memory.front().currentBytes == 1280);
    CHECK(report.memory.front().peakBytes == 1536);
    REQUIRE(report.network.size() == 1);
    CHECK(report.network.front().bytesSent == 100);
    CHECK(report.network.front().roundTripMilliseconds == 34.0);
    REQUIRE(report.audio.size() == 1);
    CHECK(report.audio.front().bus == "Master");
    CHECK(report.audio.front().voices == 6);
    CHECK(report.audio.front().samples == 72000);
    CHECK(report.audio.front().cpuMilliseconds == 2.0);
}

TEST_CASE("Production profiler RAII scopes close active scopes", "[profiler][cpu]")
{
    ProductionProfiler profiler;
    profiler.BeginFrame(1);
    {
        ProfilerScope scope(&profiler, "RAII");
    }
    profiler.EndFrame();

    const ProfilerReport report = profiler.BuildReport();
    REQUIRE(report.scopes.size() == 1);
    CHECK(report.scopes.front().name == "RAII");
    CHECK(report.scopes.front().calls == 1);
}

TEST_CASE("Blueprint runtime exposes production profiler nodes", "[profiler][blueprint]")
{
    BlueprintRuntime runtime;
    ProductionProfiler profiler;
    runtime.SetProductionProfiler(&profiler);

    CHECK(runtime.GetRegistry().Find("Profiler.BeginScope") != nullptr);
    CHECK(runtime.GetRegistry().Find("Profiler.EndScope") != nullptr);
    CHECK(runtime.GetRegistry().Find("Profiler.GetFrameTime") != nullptr);
    CHECK(runtime.GetProductionProfiler() == &profiler);
}
