#include <Urho3D/Blueprint/BlueprintRuntime.h>
#include <Urho3D/Profiler/ProductionProfiler.h>
#include <Urho3D/WorldFabric/WorldFabric.h>
#include <Urho3D/WorldFabric/WorldFabricProfiler.h>
#include <Urho3D/Resource/PlatformExportAdapter.h>

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

TEST_CASE("World Fabric profiler correlates semantic nodes deterministically", "[profiler][worldfabric]")
{
    WorldFabricGraph graph;
    const WorldFabricId playerNode = graph.AddNode("gameplay/player", WorldFabricNodeKind::Blueprint, "Player");
    REQUIRE(playerNode != InvalidWorldFabricId);

    ProductionProfiler productionProfiler;
    WorldFabricProfiler profiler(&graph, &productionProfiler);
    productionProfiler.BeginFrame(10);
    ea::string error;
    REQUIRE(profiler.AnnotateNode(playerNode, 1.25, "CPU", &error));
    REQUIRE(profiler.AnnotateNode(playerNode, 2.75, "CPU", &error));
    REQUIRE(profiler.AnnotateNode(playerNode, 0.5, "GPU", &error));
    productionProfiler.EndFrame();

    WorldFabricNodeProfile cpu;
    REQUIRE(profiler.GetNodeStats(playerNode, "CPU", cpu));
    CHECK(cpu.calls == 2);
    CHECK(cpu.totalMilliseconds == Catch::Approx(4.0));
    CHECK(cpu.GetAverageMilliseconds() == Catch::Approx(2.0));
    CHECK(cpu.minimumMilliseconds == Catch::Approx(1.25));
    CHECK(cpu.maximumMilliseconds == Catch::Approx(2.75));
    CHECK(cpu.key == "gameplay/player");

    WorldFabricNodeProfile gpu;
    REQUIRE(profiler.GetNodeStats(playerNode, "GPU", gpu));
    CHECK(gpu.calls == 1);
    CHECK(gpu.totalMilliseconds == Catch::Approx(0.5));
    const ProfilerReport report = productionProfiler.BuildReport();
    CHECK(report.scopes.size() == 1);
    REQUIRE(report.gpuPasses.size() == 1);
    CHECK(report.gpuPasses.front().calls == 1);
    CHECK(profiler.ComputeDigest() != 0);

    const unsigned long long digest = profiler.ComputeDigest();
    profiler.Reset();
    CHECK(profiler.ComputeDigest() != digest);
    CHECK_FALSE(profiler.AnnotateNode(InvalidWorldFabricId, 1.0, "CPU", &error));
}

TEST_CASE("Blueprint exposes World Fabric profiler and platform export nodes", "[profiler][packaging][blueprint]")
{
    BlueprintRuntime runtime;
    runtime.RegisterBuiltinNodes();
    CHECK(runtime.GetRegistry().Find("Profiler.AnnotateNode") != nullptr);
    CHECK(runtime.GetRegistry().Find("Profiler.GetNodeStats") != nullptr);
    CHECK(runtime.GetRegistry().Find("Export.GetPlatform") != nullptr);
    CHECK(runtime.GetRegistry().Find("Export.GetCapabilities") != nullptr);
}

TEST_CASE("Platform export adapters describe supported targets", "[packaging][platform]")
{
    const PlatformExportAdapter* linuxAdapter = PlatformExportAdapter::Find(PackagePlatform::Linux);
    REQUIRE(linuxAdapter != nullptr);
    CHECK(linuxAdapter->GetName() == ea::string("Linux"));
    CHECK(linuxAdapter->GetCapabilities().supportsGpu);
    CHECK(linuxAdapter->GetCapabilities().architectures.size() == 2);

    bool supported = false;
    const StringVariantMap description = PlatformExportAdapter::Describe(PackagePlatform::WebAssembly, &supported);
    CHECK(supported);
    CHECK(description.at("platform").GetString() == "WebAssembly");
    CHECK_FALSE(description.at("supportsThreads").GetBool());
    CHECK(description.at("architectures").GetString() == "wasm32");

    CHECK(PlatformExportAdapter::Find(PackagePlatform::Android) == nullptr);
}
