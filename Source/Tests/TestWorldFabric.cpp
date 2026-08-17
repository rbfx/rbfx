#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/WorldFabric/WorldFabric.h>
#include <Urho3D/WorldFabric/WorldFabricReflection.h>
#include <Urho3D/RbScript/RbScriptType.h>
#include <Urho3D/Blueprint/BlueprintRuntime.h>
#include <Urho3D/Scene/WorldPartition.h>
#include <Urho3D/WorldFabric/WorldFabricWorldPartition.h>
#include <Urho3D/WorldFabric/HotReloadManager.h>
#include <Urho3D/WorldFabric/DeterministicSimulation.h>
#include <Urho3D/WorldFabric/WorldFabricCollaboration.h>
#include <Urho3D/WorldFabric/WorldFabricAccessibility.h>
#include <Urho3D/WorldFabric/WorldFabricLocalization.h>

using namespace Urho3D;

TEST_CASE("WorldFabric stable identifiers and node metadata", "[worldfabric]")
{
    WorldFabricGraph graph;
    StringVariantMap metadata;
    metadata["owner"] = "gameplay";

    const WorldFabricId first = graph.AddNode("scene/player", WorldFabricNodeKind::Entity, "Player", metadata);
    const WorldFabricId second = graph.AddNode("scene/player", WorldFabricNodeKind::Entity, "PlayerUpdated");

    REQUIRE(first != InvalidWorldFabricId);
    REQUIRE(first == second);
    REQUIRE(WorldFabricGraph::MakeStableId("scene/player") == first);
    REQUIRE(graph.GetNode(first));
    CHECK(graph.GetNode(first)->type == "PlayerUpdated");
    CHECK(graph.GetNode(first)->metadata.empty());
}

TEST_CASE("WorldFabric builds deterministic dependency order", "[worldfabric]")
{
    WorldFabricGraph graph;
    const WorldFabricId runtime = graph.AddNode("runtime", WorldFabricNodeKind::Entity);
    const WorldFabricId script = graph.AddNode("script/player", WorldFabricNodeKind::RbScript);
    const WorldFabricId asset = graph.AddNode("asset/player", WorldFabricNodeKind::Asset);

    REQUIRE(graph.AddDependency(runtime, script, WorldFabricDependencyKind::Requires));
    REQUIRE(graph.AddDependency(script, asset, WorldFabricDependencyKind::BuildsFrom));

    ea::vector<WorldFabricId> order;
    ea::string error;
    REQUIRE(graph.BuildOrder(order, &error));
    REQUIRE(error.empty());
    REQUIRE(order.size() == 3);
    CHECK(order[0] == asset);
    CHECK(order[1] == script);
    CHECK(order[2] == runtime);
    CHECK(graph.GetDependencies(script).size() == 1);
    CHECK(graph.GetDependents(asset).size() == 1);
    CHECK(graph.Validate(&error));
}

TEST_CASE("WorldFabric rejects cycles and duplicate edges", "[worldfabric]")
{
    WorldFabricGraph graph;
    const WorldFabricId a = graph.AddNode("a", WorldFabricNodeKind::Custom);
    const WorldFabricId b = graph.AddNode("b", WorldFabricNodeKind::Custom);
    const WorldFabricId c = graph.AddNode("c", WorldFabricNodeKind::Custom);

    REQUIRE(graph.AddDependency(a, b));
    REQUIRE(graph.AddDependency(b, c));
    CHECK_FALSE(graph.AddDependency(a, b));
    REQUIRE(graph.AddDependency(c, a));

    ea::string error;
    CHECK(graph.HasCycles(&error));
    CHECK(error == "WorldFabric dependency graph contains a cycle.");
    CHECK_FALSE(graph.Validate(&error));
    CHECK(graph.RemoveDependency(c, a));
    CHECK(graph.Validate(&error));
}

TEST_CASE("WorldFabric digest and structural events are deterministic", "[worldfabric]")
{
    WorldFabricGraph graph;
    unsigned events = 0;
    unsigned lastEvent = 99;
    const unsigned subscription = graph.Subscribe([&](const WorldFabricEvent& event)
    {
        ++events;
        lastEvent = static_cast<unsigned>(event.type);
    });
    REQUIRE(subscription != 0);

    const WorldFabricId source = graph.AddNode("source", WorldFabricNodeKind::Asset);
    const WorldFabricId target = graph.AddNode("target", WorldFabricNodeKind::RenderResource);
    const unsigned long long before = graph.ComputeDigest();
    REQUIRE(graph.AddDependency(target, source, WorldFabricDependencyKind::Produces, "compiled"));
    const unsigned long long after = graph.ComputeDigest();

    CHECK(events == 3);
    CHECK(lastEvent == static_cast<unsigned>(WorldFabricEventType::DependencyAdded));
    CHECK(before != after);
    REQUIRE(graph.RemoveNode(source));
    CHECK(events == 4);
    graph.Reset();
    CHECK(events == 5);
    CHECK(lastEvent == static_cast<unsigned>(WorldFabricEventType::GraphReset));
    CHECK(graph.Unsubscribe(subscription));
}

TEST_CASE("WorldFabric projects rbscript reflection metadata", "[worldfabric][rbscript]")
{
    RbScriptTypeRegistry registry;
    RbScriptType inventory;
    inventory.kind = RbScriptTypeKind::User;
    inventory.name = "Inventory";
    registry.RegisterType(inventory);

    RbScriptFunctionSignature function;
    function.name = "Inventory::AddItem";
    function.returnType = registry.Resolve("void");
    function.parameterTypes.push_back(registry.Resolve("string"));
    function.blueprintCallable = true;
    registry.RegisterFunction(function);

    WorldFabricGraph graph;
    REQUIRE(WorldFabricReflection::RegisterRbScriptReflection(registry, graph) > 0);
    const WorldFabricNode* typeNode = graph.GetNode(WorldFabricGraph::MakeStableId("rbscript/type/Inventory"));
    const WorldFabricNode* functionNode = graph.GetNode(WorldFabricGraph::MakeStableId("rbscript/function/Inventory::AddItem"));
    REQUIRE(typeNode);
    REQUIRE(functionNode);
    CHECK(typeNode->kind == WorldFabricNodeKind::RbScript);
    const auto callable = functionNode->metadata.find("blueprintCallable");
    REQUIRE(callable != functionNode->metadata.end());
    CHECK(callable->second.GetBool());
    CHECK(graph.Validate());
}

TEST_CASE("WorldPartition cells synchronize with WorldFabric", "[worldfabric][worldpartition]")
{
    WorldPartition partition;
    StreamingCellDescriptor descriptor;
    descriptor.id = "cell_a";
    descriptor.coordinates = IntVector2(2, -1);
    descriptor.center = Vector3(10.0f, 0.0f, 20.0f);
    descriptor.radius = 64.0f;
    descriptor.scenePath = "Scenes/CellA.xml";
    descriptor.memoryCost = 4096;
    REQUIRE(partition.AddCell(descriptor));

    WorldFabricGraph graph;
    REQUIRE(WorldFabricWorldPartition::Synchronize(&partition, graph) == 1);
    const WorldFabricId cellId = WorldFabricGraph::MakeStableId("worldpartition/cell/cell_a");
    const WorldFabricId sceneId = WorldFabricGraph::MakeStableId("asset/scene/Scenes/CellA.xml");
    REQUIRE(graph.GetNode(cellId));
    REQUIRE(graph.GetNode(sceneId));
    const auto dependencies = graph.GetDependencies(cellId);
    REQUIRE(dependencies.size() == 1);
    CHECK(dependencies.front().dependency == sceneId);
    CHECK(dependencies.front().kind == WorldFabricDependencyKind::StreamsWith);
    CHECK(graph.Validate());
}

TEST_CASE("BlueprintRuntime synchronizes rbscript registry with WorldFabric", "[worldfabric][blueprint][rbscript]")
{
    WorldFabricGraph graph;
    RbScriptTypeRegistry registry;
    RbScriptFunctionSignature function;
    function.name = "world::current";
    function.returnType = registry.Resolve("Variant");
    registry.RegisterFunction(function);

    BlueprintRuntime runtime;
    runtime.SetWorldFabric(&graph);
    REQUIRE(runtime.RegisterRbScriptReflection(registry) > 0);
    CHECK(graph.GetNode(WorldFabricGraph::MakeStableId("rbscript/function/world::current")));
    CHECK(graph.Validate());
}



TEST_CASE("HotReloadManager preserves and migrates runtime state", "[worldfabric][hotreload]")
{
    HotReloadManager manager;
    StringVariantMap initial;
    initial["health"] = Variant(100);
    initial["name"] = Variant(ea::string("player"));
    REQUIRE(manager.Register("gameplay/player", HotReloadAssetKind::Blueprint, 1, initial,
        [](const HotReloadRequest&, const StringVariantMap& previous, StringVariantMap& next, ea::string&)
        {
            next = previous;
            next["health"] = Variant(previous.at("health").GetInt() + 25);
            return true;
        }));

    HotReloadRequest request;
    request.key = "gameplay/player";
    request.kind = HotReloadAssetKind::Blueprint;
    request.version = 2;
    request.preserveState = true;
    const HotReloadResult result = manager.Reload(request);
    REQUIRE(result.success);
    CHECK(result.stateRestored);
    CHECK(result.restoredValues == 2);
    REQUIRE(manager.Find("gameplay/player"));
    CHECK(manager.Find("gameplay/player")->version == 2);
    CHECK(manager.Find("gameplay/player")->state.at("health").GetInt() == 125);
}

TEST_CASE("DeterministicSimulation restores and replays fixed-step inputs", "[worldfabric][deterministic]")
{
    DeterministicSimulation simulation(16);
    StringVariantMap initial;
    initial["value"] = Variant(0);
    REQUIRE(simulation.Start(initial));

    const DeterministicStep step = [](unsigned, float, const StringVariantMap& input,
        const StringVariantMap& current, StringVariantMap& next)
    {
        next = current;
        next["value"] = Variant(current.at("value").GetInt() + input.at("delta").GetInt());
        return true;
    };

    StringVariantMap input;
    input["delta"] = Variant(2);
    REQUIRE(simulation.Advance(input, step));
    input["delta"] = Variant(3);
    REQUIRE(simulation.Advance(input, step));
    CHECK(simulation.GetCurrentFrame() == 2);
    CHECK(simulation.GetState().at("value").GetInt() == 5);

    REQUIRE(simulation.Restore(1));
    CHECK(simulation.GetState().at("value").GetInt() == 2);
    REQUIRE(simulation.ReplayTo(2, step));
    CHECK(simulation.GetState().at("value").GetInt() == 5);
    CHECK(simulation.FindSnapshot(2));
    CHECK(simulation.FindSnapshot(2)->digest == simulation.ComputeStateDigest());
}

TEST_CASE("Blueprint runtime exposes World Fabric Phase 5 nodes", "[worldfabric][blueprint]")
{
    BlueprintRuntime runtime;
    const auto& registry = runtime.GetRegistry();

    CHECK(registry.Find("Collab.SubmitOperation") != nullptr);
    CHECK(registry.Find("Collab.LockNode") != nullptr);
    CHECK(registry.Find("Collab.GetRevision") != nullptr);
    CHECK(registry.Find("Accessibility.SetFeature") != nullptr);
    CHECK(registry.Find("Accessibility.IsFeatureEnabled") != nullptr);
    CHECK(registry.Find("Accessibility.SetTextScale") != nullptr);
    CHECK(registry.Find("Localize.Translate") != nullptr);
    CHECK(registry.Find("Localize.SetLocale") != nullptr);

    const BlueprintNodeDefinition* submit = registry.Find("Collab.SubmitOperation");
    REQUIRE(submit != nullptr);
    REQUIRE(submit->pins.size() == 10);
    CHECK(submit->pins[2].name == "clientId");
    CHECK(submit->pins[7].name == "success");
    CHECK(submit->pins[8].dataType == BlueprintDataType::Int64);
}



TEST_CASE("Blueprint runtime exposes World Fabric Phase 5 service bindings", "[worldfabric][blueprint]")
{
    BlueprintRuntime runtime;
    WorldFabricGraph graph;
    WorldFabricCollaboration collaboration(&graph);
    WorldFabricAccessibility accessibility;
    WorldFabricLocalization localization;

    runtime.SetWorldFabric(&graph);
    runtime.SetWorldFabricCollaboration(&collaboration);
    runtime.SetWorldFabricAccessibility(&accessibility);
    runtime.SetWorldFabricLocalization(&localization);

    CHECK(runtime.GetWorldFabric() == &graph);
    CHECK(runtime.GetWorldFabricCollaboration() == &collaboration);
    CHECK(runtime.GetWorldFabricAccessibility() == &accessibility);
    CHECK(runtime.GetWorldFabricLocalization() == &localization);
}
