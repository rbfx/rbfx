// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/Scene/WorldPartition.h>
#include <Urho3D/WorldFabric/BuildGraph.h>
#include <Urho3D/WorldFabric/ProceduralWorld.h>
#include <Urho3D/WorldFabric/GameplayTestRunner.h>
#include <Urho3D/WorldFabric/WorldFabricCollaboration.h>
#include <Urho3D/WorldFabric/WorldFabricAccessibility.h>
#include <Urho3D/WorldFabric/WorldFabricLocalization.h>

using namespace Urho3D;

TEST_CASE("BuildGraph orders dependencies and reuses cache", "[buildgraph]")
{
    BuildGraph graph;
    unsigned executions = 0;

    BuildTask source;
    source.key = "source";
    source.kind = BuildTaskKind::ImportAsset;
    source.metadata["path"] = Variant(ea::string("Textures/hero.png"));
    REQUIRE(graph.AddTask(source, [&executions](const BuildTask&, const ea::vector<BuildTaskResult>&,
        BuildTaskResult& result, ea::string&)
    {
        ++executions;
        result.output = "hero.cooked";
        return true;
    }));

    BuildTask shader;
    shader.key = "shader";
    shader.kind = BuildTaskKind::CompileShader;
    REQUIRE(graph.AddTask(shader, [&executions](const BuildTask&, const ea::vector<BuildTaskResult>& dependencies,
        BuildTaskResult& result, ea::string&)
    {
        ++executions;
        REQUIRE(dependencies.size() == 1);
        result.output = "hero.shader.bin";
        return true;
    }));
    REQUIRE(graph.AddDependency("shader", "source"));
    REQUIRE(graph.Validate());

    ea::vector<ea::string> executed;
    REQUIRE(graph.Execute(&executed));
    REQUIRE(executions == 2);
    REQUIRE(executed == ea::vector<ea::string>({"source", "shader"}));

    REQUIRE(graph.Execute(&executed));
    REQUIRE(executions == 2);
    REQUIRE(graph.FindResult("shader"));
    REQUIRE(graph.FindResult("shader")->cacheHit);
}

TEST_CASE("BuildGraph rejects dependency cycles", "[buildgraph]")
{
    BuildGraph graph;
    const BuildTaskExecutor executor = [](const BuildTask&, const ea::vector<BuildTaskResult>&,
        BuildTaskResult&, ea::string&) { return true; };
    BuildTask first;
    first.key = "first";
    BuildTask second;
    second.key = "second";
    REQUIRE(graph.AddTask(first, executor));
    REQUIRE(graph.AddTask(second, executor));
    REQUIRE(graph.AddDependency("first", "second"));
    ea::string error;
    REQUIRE_FALSE(graph.AddDependency("second", "first"));
    REQUIRE_FALSE(graph.Validate(&error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("ProceduralWorld generates stable idempotent partition cells", "[procedural]")
{
    ProceduralWorldSettings settings;
    settings.seed = 4242;
    settings.minX = -1;
    settings.maxX = 1;
    settings.minY = -1;
    settings.maxY = 1;
    settings.cellSize = 64.0f;
    settings.cellRadius = 48.0f;
    settings.lodLevels = 3;

    WorldPartition partition;
    ea::vector<ProceduralCellInfo> first;
    REQUIRE(ProceduralWorldGenerator::Generate(partition, settings, &first));
    REQUIRE(first.size() == 9);
    REQUIRE(partition.GetCellIds().size() == 9);
    REQUIRE(first.front().seed == ProceduralWorldGenerator::CellSeed(settings.seed, -1, -1));
    REQUIRE(ProceduralWorldGenerator::SelectLod(0.0f, settings.cellSize, 3) == 0);
    REQUIRE(ProceduralWorldGenerator::SelectLod(256.0f, settings.cellSize, 3) == 2);

    ea::vector<ProceduralCellInfo> second;
    REQUIRE(ProceduralWorldGenerator::Generate(partition, settings, &second));
    REQUIRE(second.size() == first.size());
    REQUIRE(second.front().seed == first.front().seed);
    REQUIRE(second.front().height == first.front().height);
}

TEST_CASE("ProceduralWorld rejects unsafe settings", "[procedural]")
{
    ProceduralWorldSettings settings;
    settings.cellSize = 0.0f;
    ea::string error;
    REQUIRE_FALSE(ProceduralWorldGenerator::ValidateSettings(settings, &error));
    REQUIRE_FALSE(error.empty());
}


TEST_CASE("GameplayTestRunner executes deterministic tagged cases", "[gameplay-tests]")
{
    GameplayTestRunner runner;
    GameplayTestCase alpha;
    alpha.name = "alpha";
    alpha.tags.push_back("smoke");
    alpha.callback = [](ea::string&) { return true; };
    GameplayTestCase beta;
    beta.name = "beta";
    beta.tags.push_back("slow");
    beta.callback = [](ea::string& error)
    {
        error = "intentional failure";
        return false;
    };
    REQUIRE(runner.Register(beta));
    REQUIRE(runner.Register(alpha));
    REQUIRE_FALSE(runner.Register(alpha));

    REQUIRE(runner.Run("smoke"));
    REQUIRE(runner.GetPassedCount() == 1);
    REQUIRE(runner.GetFailedCount() == 0);
    REQUIRE(runner.GetResults().front().name == "alpha");

    ea::string error;
    REQUIRE_FALSE(runner.Run("slow", &error));
    REQUIRE(runner.GetPassedCount() == 0);
    REQUIRE(runner.GetFailedCount() == 1);
    REQUIRE(error == "intentional failure");
}

TEST_CASE("WorldFabric collaboration applies locked versioned operations", "[collaboration]")
{
    WorldFabricGraph graph;
    const WorldFabricId node = graph.AddNode("hero", WorldFabricNodeKind::Entity, "Player");
    REQUIRE(node != InvalidWorldFabricId);
    WorldFabricCollaboration collaboration(&graph);
    REQUIRE(collaboration.AddClient("alice"));
    REQUIRE(collaboration.AddClient("bob"));
    REQUIRE(collaboration.Lock(node, "alice"));
    REQUIRE_FALSE(collaboration.Lock(node, "bob"));

    WorldFabricOperation operation;
    operation.clientId = "alice";
    operation.kind = WorldFabricOperationKind::SetMetadata;
    operation.node = node;
    operation.metadataKey = "owner";
    operation.metadataValue = Variant(ea::string("alice"));
    REQUIRE(collaboration.Submit(operation));
    REQUIRE(collaboration.GetRevision() == 1);
    REQUIRE(graph.GetNode(node)->metadata["owner"].GetString() == "alice");

    REQUIRE_FALSE(collaboration.Unlock(node, "bob"));
    REQUIRE(collaboration.Unlock(node, "alice"));
}

TEST_CASE("WorldFabric accessibility validates settings and digest", "[accessibility]")
{
    WorldFabricAccessibility accessibility;
    REQUIRE(accessibility.SetFeature(AccessibilityFeature::Subtitles, true));
    REQUIRE(accessibility.SetFeature(AccessibilityFeature::ReducedMotion, true));
    REQUIRE(accessibility.SetTextScale(1.5f));
    REQUIRE(accessibility.IsFeatureEnabled(AccessibilityFeature::Subtitles));
    REQUIRE(accessibility.IsFeatureEnabled(AccessibilityFeature::ReducedMotion));
    const unsigned long long firstDigest = accessibility.ComputeDigest();
    REQUIRE_FALSE(accessibility.SetTextScale(4.0f));
    REQUIRE(accessibility.ComputeDigest() == firstDigest);
}

TEST_CASE("WorldFabric localization resolves current and fallback locales", "[localization]")
{
    WorldFabricLocalization localization;
    REQUIRE(localization.AddLocale("en"));
    REQUIRE(localization.AddLocale("fr"));
    REQUIRE(localization.SetText("en", "menu.play", "Play"));
    REQUIRE(localization.SetText("fr", "menu.play", "Jouer"));
    REQUIRE(localization.SetText("en", "menu.quit", "Quit"));
    REQUIRE(localization.SetLocale("fr"));
    REQUIRE(localization.SetFallbackLocale("en"));
    REQUIRE(localization.Translate("menu.play") == "Jouer");
    REQUIRE(localization.Translate("menu.quit") == "Quit");
    REQUIRE(localization.Translate("missing", "Missing") == "Missing");
    REQUIRE(localization.GetLocales().size() == 2);
}
