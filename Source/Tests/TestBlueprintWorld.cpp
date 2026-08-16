#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/Blueprint/BlueprintRuntime.h>
#include <Urho3D/Scene/WorldPartition.h>

using namespace Urho3D;

namespace
{
void AddPin(BlueprintGraph& graph, BlueprintId nodeId, const char* name, BlueprintPinKind kind,
    BlueprintDataType type, const Variant& defaultValue = Variant())
{
    BlueprintPin pin;
    pin.name = name;
    pin.displayName = name;
    pin.kind = kind;
    pin.dataType = type;
    pin.defaultValue = defaultValue;
    REQUIRE(graph.AddPin(nodeId, pin));
}

StreamingCellDescriptor MakeCell(const ea::string& id)
{
    StreamingCellDescriptor descriptor;
    descriptor.id = id;
    descriptor.coordinates = IntVector2(0, 0);
    descriptor.center = Vector3::ZERO;
    descriptor.scenePath = id + ".scene";
    return descriptor;
}
}

TEST_CASE("Blueprint World.LoadCell queues a named cell", "[blueprint][streaming]")
{
    WorldPartition partition;
    REQUIRE(partition.AddCell(MakeCell("Gameplay")));

    BlueprintGraph graph("LoadCell");
    const BlueprintId eventNode = graph.AddNode("Event.OnStart", "Start");
    const BlueprintId loadNode = graph.AddNode("World.LoadCell", "Load Gameplay");
    AddPin(graph, eventNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, loadNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, loadNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, loadNode, "cell", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("Gameplay")));
    AddPin(graph, loadNode, "queued", BlueprintPinKind::Output, BlueprintDataType::Bool);
    REQUIRE(graph.AddLink(eventNode, "then", loadNode, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintRuntime runtime;
    runtime.SetWorldPartition(&partition);
    REQUIRE(runtime.ExecuteEvent(graph, "Event.OnStart"));
    REQUIRE_FALSE(runtime.HadRuntimeError());
    REQUIRE(runtime.GetValue(loadNode, "queued").GetBool());
    REQUIRE(partition.GetCell("Gameplay")->GetState() == StreamingCellState::Loading);
}

TEST_CASE("Blueprint World nodes report missing cells and change radius", "[blueprint][streaming]")
{
    WorldPartition partition;
    REQUIRE(partition.AddCell(MakeCell("Gameplay")));
    BlueprintRuntime runtime;
    runtime.SetWorldPartition(&partition);

    BlueprintGraph radiusGraph("Radius");
    const BlueprintId radiusNode = radiusGraph.AddNode("World.SetStreamingRadius", "Set Radius");
    AddPin(radiusGraph, radiusNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(radiusGraph, radiusNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(radiusGraph, radiusNode, "radius", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(42.0f));
    AddPin(radiusGraph, radiusNode, "value", BlueprintPinKind::Output, BlueprintDataType::Float);
    REQUIRE(runtime.Execute(radiusGraph, radiusNode));
    REQUIRE(runtime.GetValue(radiusNode, "value").GetFloat() == Catch::Approx(42.0f));
    REQUIRE(partition.GetStreamingRadius() == Catch::Approx(42.0f));

    BlueprintGraph unloadGraph("UnloadCell");
    const BlueprintId unloadNode = unloadGraph.AddNode("World.UnloadCell", "Unload Missing");
    AddPin(unloadGraph, unloadNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(unloadGraph, unloadNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(unloadGraph, unloadNode, "cell", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("Missing")));
    AddPin(unloadGraph, unloadNode, "queued", BlueprintPinKind::Output, BlueprintDataType::Bool);
    REQUIRE(runtime.Execute(unloadGraph, unloadNode));
    REQUIRE(runtime.HadRuntimeError());
    REQUIRE_FALSE(runtime.GetValue(unloadNode, "queued").GetBool());
}
