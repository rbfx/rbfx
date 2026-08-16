// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/Blueprint/BlueprintRuntime.h>

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

}

TEST_CASE("Blueprint graph validates execution links", "[blueprint][graph]")
{
    BlueprintGraph graph("Validation");
    const BlueprintId eventNode = graph.AddNode("Event.OnStart", "On Start");
    const BlueprintId printNode = graph.AddNode("Flow.Print", "Print");
    AddPin(graph, eventNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("hello")));

    REQUIRE(graph.AddLink(eventNode, "then", printNode, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(graph.Validate().IsValid());
    REQUIRE(graph.GetNodes().size() == 2);
    REQUIRE(graph.GetLinks().size() == 1);
}

TEST_CASE("Blueprint graph round trips through JSON", "[blueprint][serialization]")
{
    BlueprintGraph source("RoundTrip");
    const BlueprintId nodeId = source.AddNode("Math.AddFloat", "Add", {120.0f, 80.0f}, BlueprintExecutionMode::Pure);
    AddPin(source, nodeId, "a", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(2.0f));
    AddPin(source, nodeId, "b", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(3.0f));
    AddPin(source, nodeId, "result", BlueprintPinKind::Output, BlueprintDataType::Float);

    BlueprintGraph restored;
    ea::string error;
    REQUIRE(restored.FromString(source.ToString(), &error));
    REQUIRE(error.empty());
    REQUIRE(restored.GetName() == "RoundTrip");
    REQUIRE(restored.GetNodes().size() == 1);
    REQUIRE(restored.GetNodes()[0].pins.size() == 3);
    REQUIRE(restored.GetNodes()[0].position == Vector2{120.0f, 80.0f});
}

TEST_CASE("Blueprint runtime evaluates builtin math node", "[blueprint][runtime]")
{
    BlueprintGraph graph("Math");
    const BlueprintId addNode = graph.AddNode("Math.AddFloat", "Add", {}, BlueprintExecutionMode::Pure);
    AddPin(graph, addNode, "a", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(2.0f));
    AddPin(graph, addNode, "b", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(3.5f));
    AddPin(graph, addNode, "result", BlueprintPinKind::Output, BlueprintDataType::Float);

    BlueprintRuntime runtime;
    REQUIRE(runtime.Execute(graph, addNode));
    REQUIRE_FALSE(runtime.HadRuntimeError());
    REQUIRE(runtime.GetValue(addNode, "result").GetFloat() == Catch::Approx(5.5f));
}

TEST_CASE("Blueprint runtime follows an event into Print", "[blueprint][runtime]")
{
    BlueprintGraph graph("Flow");
    const BlueprintId eventNode = graph.AddNode("Event.OnStart", "On Start");
    const BlueprintId printNode = graph.AddNode("Flow.Print", "Print");
    AddPin(graph, eventNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("runtime ok")));
    REQUIRE(graph.AddLink(eventNode, "then", printNode, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintRuntime runtime;
    REQUIRE(runtime.ExecuteEvent(graph, "Event.OnStart"));
    REQUIRE_FALSE(runtime.HadRuntimeError());
    REQUIRE(runtime.GetDiagnostics().empty());
}
