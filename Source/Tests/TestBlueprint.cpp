// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/Blueprint/BlueprintReflection.h>
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

TEST_CASE("Blueprint graph supports search, comments, functions and automatic layout", "[blueprint][graph][editor]")
{
    BlueprintGraph graph("AdvancedGraph");
    const BlueprintId eventNode = graph.AddNode("Event.OnStart", "On Start");
    const BlueprintId printNode = graph.AddNode("Flow.Print", "Print Message");
    graph.GetNode(printNode)->category = "Flow";
    AddPin(graph, eventNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("hello")));
    REQUIRE(graph.AddLink(eventNode, "then", printNode, "execute") != BLUEPRINT_INVALID_ID);

    const ea::vector<BlueprintId> searchResults = graph.SearchNodes("print message");
    REQUIRE(searchResults.size() == 1);
    REQUIRE(searchResults[0] == printNode);

    BlueprintComment comment;
    comment.text = "Flow entry";
    comment.position = Vector2{10.0f, 20.0f};
    comment.size = Vector2{400.0f, 180.0f};
    comment.color = 0xFF102030;
    REQUIRE(graph.AddComment(comment));
    REQUIRE(graph.GetComments().size() == 1);
    const BlueprintId commentId = graph.GetComments()[0].id;
    graph.GetNode(eventNode)->commentId = commentId;

    BlueprintFunction function;
    function.name = "FormatMessage";
    function.description = "Formats a message for display";
    function.body = "{\"name\":\"FormatMessage\"}";
    BlueprintPin functionInput;
    functionInput.name = "value";
    functionInput.dataType = BlueprintDataType::String;
    functionInput.kind = BlueprintPinKind::Input;
    function.inputs.push_back(functionInput);
    BlueprintPin functionOutput;
    functionOutput.name = "result";
    functionOutput.dataType = BlueprintDataType::String;
    functionOutput.kind = BlueprintPinKind::Output;
    function.outputs.push_back(functionOutput);
    REQUIRE(graph.AddFunction(function));

    graph.AutoLayout();
    REQUIRE(graph.GetNode(eventNode)->position == Vector2::ZERO);
    REQUIRE(graph.GetNode(printNode)->position == Vector2{320.0f, 0.0f});

    BlueprintGraph restored;
    ea::string error;
    REQUIRE(restored.FromString(graph.ToString(), &error));
    REQUIRE(error.empty());
    REQUIRE(restored.GetComments().size() == 1);
    REQUIRE(restored.GetComments()[0].text == "Flow entry");
    REQUIRE(restored.GetComments()[0].size == Vector2{400.0f, 180.0f});
    REQUIRE(restored.GetNode(eventNode)->commentId == commentId);
    REQUIRE(restored.GetFunctions().size() == 1);
    REQUIRE(restored.GetFunction("FormatMessage") != nullptr);
    REQUIRE(restored.GetFunction("FormatMessage")->inputs.size() == 1);
    REQUIRE(restored.GetFunction("FormatMessage")->body == function.body);
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

TEST_CASE("Blueprint runtime executes serialized Blueprint subgraphs", "[blueprint][functions]")
{
    BlueprintGraph body("FunctionBody");
    const BlueprintId entryNode = body.AddNode("Function.Entry", "Entry");
    const BlueprintId setNode = body.AddNode("Variable.Set", "Set Called");
    const BlueprintId returnNode = body.AddNode("Function.Return", "Return");
    AddPin(body, entryNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, setNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(body, setNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, setNode, "value", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(7));
    AddPin(body, returnNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    body.GetNode(setNode)->properties["variableName"] = Variant(ea::string("called"));
    REQUIRE(body.AddLink(entryNode, "then", setNode, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(body.AddLink(setNode, "then", returnNode, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintGraph graph("FunctionOwner");
    BlueprintFunction function;
    function.name = "MarkCalled";
    function.body = body.ToString();
    REQUIRE(graph.AddFunction(function));
    const BlueprintId callNode = graph.AddNode("Function.Call", "Call MarkCalled");
    graph.GetNode(callNode)->properties["functionName"] = Variant(ea::string("MarkCalled"));

    BlueprintRuntime runtime;
    REQUIRE(runtime.Execute(graph, callNode));
    REQUIRE_FALSE(runtime.HadRuntimeError());
    REQUIRE(runtime.GetVariable("called").GetInt() == 7);
}

TEST_CASE("Blueprint runtime supports step-by-step debugging", "[blueprint][debug]")
{
    BlueprintGraph graph("DebugFlow");
    const BlueprintId eventNode = graph.AddNode("Event.OnStart", "On Start");
    const BlueprintId printNode = graph.AddNode("Flow.Print", "Print");
    AddPin(graph, eventNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("debug ok")));
    REQUIRE(graph.AddLink(eventNode, "then", printNode, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintRuntime runtime;
    REQUIRE(runtime.BeginDebug(graph, "Event.OnStart"));
    REQUIRE(runtime.IsDebugActive());
    REQUIRE(runtime.GetDebugCurrentNode() == eventNode);
    REQUIRE(runtime.StepDebug());
    REQUIRE(runtime.IsDebugActive());
    REQUIRE(runtime.GetDebugCurrentNode() == printNode);
    REQUIRE(runtime.StepDebug());
    REQUIRE_FALSE(runtime.IsDebugActive());
    runtime.StopDebug();
    REQUIRE_FALSE(runtime.IsDebugActive());
}

TEST_CASE("Blueprint reflection maps rbfx variant types", "[blueprint][reflection]")
{
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_BOOL) == BlueprintDataType::Bool);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_INT) == BlueprintDataType::Int);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_INT64) == BlueprintDataType::Int64);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_FLOAT) == BlueprintDataType::Float);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_STRING) == BlueprintDataType::String);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_VECTOR2) == BlueprintDataType::Vector2);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_VECTOR3) == BlueprintDataType::Vector3);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_QUATERNION) == BlueprintDataType::Quaternion);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_VOIDPTR) == BlueprintDataType::Object);
    REQUIRE(BlueprintReflectionRegistry::MapVariantType(VAR_BUFFER) == BlueprintDataType::Variant);
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
