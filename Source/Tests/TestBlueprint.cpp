// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include "CommonUtils.h"

#include <Urho3D/Blueprint/BlueprintReflection.h>
#include <Urho3D/Blueprint/BlueprintResource.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/VectorBuffer.h>
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

TEST_CASE("Blueprint runtime transports function inputs and outputs", "[blueprint][functions]")
{
    BlueprintGraph body("ParameterizedBody");
    const BlueprintId entryNode = body.AddNode("Function.Entry", "Entry");
    const BlueprintId getNode = body.AddNode("Variable.Get", "Get Input");
    const BlueprintId setNode = body.AddNode("Variable.Set", "Set Result");
    const BlueprintId returnNode = body.AddNode("Function.Return", "Return");
    AddPin(body, entryNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, getNode, "value", BlueprintPinKind::Output, BlueprintDataType::Int);
    AddPin(body, setNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(body, setNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, setNode, "value", BlueprintPinKind::Input, BlueprintDataType::Int);
    AddPin(body, returnNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    body.GetNode(getNode)->properties["variableName"] = Variant(ea::string("input"));
    body.GetNode(setNode)->properties["variableName"] = Variant(ea::string("result"));
    REQUIRE(body.AddLink(entryNode, "then", setNode, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(body.AddLink(setNode, "then", returnNode, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(body.AddLink(getNode, "value", setNode, "value") != BLUEPRINT_INVALID_ID);

    BlueprintFunction function;
    function.name = "EchoInt";
    function.body = body.ToString();
    BlueprintPin input;
    input.name = "input";
    input.kind = BlueprintPinKind::Input;
    input.dataType = BlueprintDataType::Int;
    function.inputs.push_back(input);
    BlueprintPin output;
    output.name = "result";
    output.kind = BlueprintPinKind::Output;
    output.dataType = BlueprintDataType::Int;
    function.outputs.push_back(output);

    BlueprintGraph graph("FunctionParameters");
    REQUIRE(graph.AddFunction(function));
    const BlueprintId callNode = graph.AddNode("Function.Call", "Echo");
    graph.GetNode(callNode)->properties["functionName"] = Variant(ea::string("EchoInt"));
    AddPin(graph, callNode, "input", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(42));
    AddPin(graph, callNode, "result", BlueprintPinKind::Output, BlueprintDataType::Int);

    BlueprintRuntime runtime;
    REQUIRE(runtime.Execute(graph, callNode));
    REQUIRE_FALSE(runtime.HadRuntimeError());
    REQUIRE(runtime.GetValue(callNode, "result").GetInt() == 42);
}

TEST_CASE("Blueprint runtime supports input events and latent execution", "[blueprint][runtime][latent]")
{
    BlueprintGraph inputGraph("InputEvents");
    const BlueprintId keyNode = inputGraph.AddNode("Event.OnKeyPressed", "Key Pressed");
    AddPin(inputGraph, keyNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(inputGraph, keyNode, "key", BlueprintPinKind::Output, BlueprintDataType::Int);

    BlueprintRuntime inputRuntime;
    StringVariantMap inputVariables;
    inputVariables["__event.key"] = Variant(65);
    REQUIRE(inputRuntime.ExecuteEvent(inputGraph, "Event.OnKeyPressed", inputVariables));
    REQUIRE(inputRuntime.GetValue(keyNode, "key").GetInt() == 65);

    BlueprintGraph graph("LatentFlow");
    const BlueprintId eventNode = graph.AddNode("Event.OnStart", "Start");
    const BlueprintId delayNode = graph.AddNode("Flow.Delay", "Delay");
    const BlueprintId setNode = graph.AddNode("Variable.Set", "Complete");
    AddPin(graph, eventNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, delayNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, delayNode, "completed", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, delayNode, "duration", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(0.5f));
    AddPin(graph, setNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, setNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, setNode, "value", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(1));
    graph.GetNode(setNode)->properties["variableName"] = Variant(ea::string("completed"));
    REQUIRE(graph.AddLink(eventNode, "then", delayNode, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(graph.AddLink(delayNode, "completed", setNode, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintRuntime runtime;
    REQUIRE(runtime.ExecuteEvent(graph, "Event.OnStart"));
    REQUIRE(runtime.IsLatent());
    REQUIRE(runtime.Tick(0.25f));
    REQUIRE(runtime.IsLatent());
    REQUIRE(runtime.Tick(0.25f));
    REQUIRE_FALSE(runtime.IsLatent());
    REQUIRE(runtime.GetVariable("completed").GetInt() == 1);
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

TEST_CASE("Blueprint debugger continues to a breakpoint and exposes watches", "[blueprint][debug]")
{
    BlueprintGraph graph("BreakpointFlow");
    const BlueprintId eventNode = graph.AddNode("Event.OnStart", "Start");
    const BlueprintId printNode = graph.AddNode("Flow.Print", "Print");
    AddPin(graph, eventNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(graph, printNode, "message", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("watch")));
    REQUIRE(graph.AddLink(eventNode, "then", printNode, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintRuntime runtime;
    runtime.SetBreakpoint(printNode);
    REQUIRE(runtime.HasBreakpoint(printNode));
    REQUIRE(runtime.BeginDebug(graph, "Event.OnStart"));
    REQUIRE(runtime.ContinueDebug());
    REQUIRE(runtime.IsDebugActive());
    REQUIRE(runtime.GetDebugCurrentNode() == printNode);
    REQUIRE(runtime.GetBreakpoints().size() == 1);
    REQUIRE(runtime.ContinueDebug());
    REQUIRE_FALSE(runtime.IsDebugActive());
    runtime.SetBreakpoint(printNode, false);
    REQUIRE_FALSE(runtime.HasBreakpoint(printNode));
}

TEST_CASE("Blueprint graph supports structs and enums with JSON round-trip", "[blueprint][types][serialization]")
{
    BlueprintGraph source("UserTypes");
    BlueprintStructDef transform;
    transform.name = "Transform2D";
    transform.description = "A 2D transform value.";
    BlueprintStructField position;
    position.name = "position";
    position.dataType = BlueprintDataType::Vector2;
    position.defaultValue = Variant(Vector2::ZERO);
    transform.fields.push_back(position);
    BlueprintStructField scale;
    scale.name = "scale";
    scale.dataType = BlueprintDataType::Float;
    scale.defaultValue = Variant(1.0f);
    transform.fields.push_back(scale);
    REQUIRE(source.AddStruct(transform));
    REQUIRE(source.GetStruct("Transform2D") != nullptr);
    REQUIRE(source.GetStruct("Transform2D")->fields.size() == 2);
    BlueprintStructDef invalidStruct = transform;
    invalidStruct.fields.push_back(position);
    REQUIRE_FALSE(source.AddStruct(invalidStruct));

    BlueprintEnumDef direction;
    direction.name = "Direction";
    direction.description = "Cardinal directions.";
    direction.values.push_back({"North", 10});
    direction.values.push_back({"South", 20});
    REQUIRE(source.AddEnum(direction));
    REQUIRE(source.GetEnum("Direction") != nullptr);
    REQUIRE(source.GetEnum("Direction")->values[1].value == 20);
    BlueprintEnumDef invalidEnum = direction;
    invalidEnum.values.push_back({"North", 30});
    REQUIRE_FALSE(source.AddEnum(invalidEnum));

    BlueprintGraph restored;
    ea::string error;
    REQUIRE(restored.FromString(source.ToString(), &error));
    REQUIRE(error.empty());
    REQUIRE(restored.GetStructs().size() == 1);
    REQUIRE(restored.GetStruct("Transform2D")->fields[1].defaultValue.GetFloat() == Catch::Approx(1.0f));
    REQUIRE(restored.GetEnums().size() == 1);
    REQUIRE(restored.GetEnum("Direction")->values[0].value == 10);
    REQUIRE(source.ToString().find("\"schemaVersion\": 5") != ea::string::npos);
}

TEST_CASE("Blueprint runtime executes Array and Map nodes", "[blueprint][collections][runtime]")
{
    VariantVector initialArray;
    initialArray.push_back(Variant(11));
    initialArray.push_back(Variant(22));

    BlueprintGraph arrayGraph("ArrayNodes");
    const BlueprintId lengthNode = arrayGraph.AddNode("Array.Length", "Length");
    AddPin(arrayGraph, lengthNode, "array", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(initialArray));
    AddPin(arrayGraph, lengthNode, "length", BlueprintPinKind::Output, BlueprintDataType::Int);
    BlueprintRuntime arrayRuntime;
    REQUIRE(arrayRuntime.Execute(arrayGraph, lengthNode));
    REQUIRE(arrayRuntime.GetValue(lengthNode, "length").GetInt() == 2);

    const BlueprintId getNode = arrayGraph.AddNode("Array.Get", "Get");
    AddPin(arrayGraph, getNode, "array", BlueprintPinKind::Input, BlueprintDataType::Array, Variant(initialArray));
    AddPin(arrayGraph, getNode, "index", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(1));
    AddPin(arrayGraph, getNode, "value", BlueprintPinKind::Output, BlueprintDataType::Variant);
    AddPin(arrayGraph, getNode, "valid", BlueprintPinKind::Output, BlueprintDataType::Bool);
    REQUIRE(arrayRuntime.Execute(arrayGraph, getNode));
    REQUIRE(arrayRuntime.GetValue(getNode, "valid").GetBool());
    REQUIRE(arrayRuntime.GetValue(getNode, "value").GetInt() == 22);

    StringVariantMap initialMap;
    initialMap["health"] = Variant(100);
    BlueprintGraph mapGraph("MapNodes");
    const BlueprintId containsNode = mapGraph.AddNode("Map.Contains", "Contains");
    AddPin(mapGraph, containsNode, "map", BlueprintPinKind::Input, BlueprintDataType::Map, Variant(initialMap));
    AddPin(mapGraph, containsNode, "key", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("health")));
    AddPin(mapGraph, containsNode, "contains", BlueprintPinKind::Output, BlueprintDataType::Bool);
    BlueprintRuntime mapRuntime;
    REQUIRE(mapRuntime.Execute(mapGraph, containsNode));
    REQUIRE(mapRuntime.GetValue(containsNode, "contains").GetBool());

    const BlueprintId setNode = mapGraph.AddNode("Map.Set", "Set");
    AddPin(mapGraph, setNode, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(mapGraph, setNode, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(mapGraph, setNode, "map", BlueprintPinKind::Input, BlueprintDataType::Map, Variant(initialMap));
    AddPin(mapGraph, setNode, "key", BlueprintPinKind::Input, BlueprintDataType::String, Variant(ea::string("mana")));
    AddPin(mapGraph, setNode, "value", BlueprintPinKind::Input, BlueprintDataType::Variant, Variant(50));
    AddPin(mapGraph, setNode, "result", BlueprintPinKind::Output, BlueprintDataType::Map);
    REQUIRE(mapRuntime.Execute(mapGraph, setNode));
    const Variant updatedMap = mapRuntime.GetValue(setNode, "result");
    const StringVariantMap& updatedEntries = updatedMap.GetStringVariantMap();
    REQUIRE(updatedEntries.find("mana") != updatedEntries.end());
}

TEST_CASE("Blueprint delegates and signals round trip and execute", "[blueprint][events]")
{
    BlueprintGraph graph("Events");
    BlueprintDelegate delegate;
    delegate.name = "OnDamaged";
    delegate.description = "Called when damage is received";
    BlueprintPin amount;
    amount.name = "amount";
    amount.displayName = "amount";
    amount.kind = BlueprintPinKind::Input;
    amount.dataType = BlueprintDataType::Int;
    delegate.parameters.push_back(amount);
    REQUIRE(graph.AddDelegate(delegate));
    REQUIRE(graph.GetDelegate("OnDamaged") != nullptr);
    REQUIRE(graph.GetDelegates().size() == 1);
    REQUIRE_FALSE(graph.AddDelegate(BlueprintDelegate{"OnDamaged", "", {amount, amount}}));

    BlueprintGraph restored;
    ea::string error;
    REQUIRE(restored.FromString(graph.ToString(), &error));
    REQUIRE(error.empty());
    REQUIRE(restored.GetDelegates().size() == 1);
    REQUIRE(restored.GetDelegate("OnDamaged")->parameters.size() == 1);
    REQUIRE(restored.ToJSON()["schemaVersion"].GetUInt() == 5);

    BlueprintGraph body("DelegateBody");
    const BlueprintId entry = body.AddNode("Function.Entry", "Entry");
    const BlueprintId set = body.AddNode("Variable.Set", "Set Called");
    const BlueprintId result = body.AddNode("Function.Return", "Return");
    AddPin(body, entry, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, set, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(body, set, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(body, set, "value", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(9));
    AddPin(body, result, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    body.GetNode(set)->properties["variableName"] = Variant(ea::string("called"));
    REQUIRE(body.AddLink(entry, "then", set, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(body.AddLink(set, "then", result, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintFunction function;
    function.name = "HandleDamage";
    function.body = body.ToString();
    REQUIRE(graph.AddFunction(function));

    const BlueprintId bind = graph.AddNode("Delegate.Bind", "Bind Damage");
    graph.GetNode(bind)->properties["delegateName"] = Variant(ea::string("OnDamaged"));
    graph.GetNode(bind)->properties["functionName"] = Variant(ea::string("HandleDamage"));
    BlueprintRuntime runtime;
    REQUIRE(runtime.Execute(graph, bind));
    REQUIRE(runtime.IsDelegateBound("OnDamaged"));
    REQUIRE(runtime.InvokeDelegate(graph, "OnDamaged"));
    REQUIRE(runtime.GetVariable("called").GetInt() == 9);
    REQUIRE(runtime.UnbindDelegate("OnDamaged"));
    REQUIRE_FALSE(runtime.IsDelegateBound("OnDamaged"));
}

TEST_CASE("Blueprint timelines interpolate and macros execute", "[blueprint][timeline][macro]")
{
    BlueprintGraph graph("TimelineMacro");
    BlueprintTimeline timeline;
    timeline.name = "Progress";
    timeline.length = 2.0f;
    timeline.keyframes.push_back({0.0f, Variant(0.0f)});
    timeline.keyframes.push_back({2.0f, Variant(10.0f)});
    REQUIRE(graph.AddTimeline(timeline));
    REQUIRE(graph.GetTimeline("Progress") != nullptr);
    REQUIRE_FALSE(graph.AddTimeline(BlueprintTimeline{"Invalid", "", 1.0f, false, {{1.0f, Variant(1)}, {0.5f, Variant(2)}}}));

    BlueprintGraph macroBody("MacroBody");
    const BlueprintId entry = macroBody.AddNode("Function.Entry", "Entry");
    const BlueprintId set = macroBody.AddNode("Variable.Set", "Set Macro Called");
    const BlueprintId result = macroBody.AddNode("Function.Return", "Return");
    AddPin(macroBody, entry, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(macroBody, set, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    AddPin(macroBody, set, "then", BlueprintPinKind::ExecutionOutput, BlueprintDataType::Wildcard);
    AddPin(macroBody, set, "value", BlueprintPinKind::Input, BlueprintDataType::Int, Variant(17));
    AddPin(macroBody, result, "execute", BlueprintPinKind::ExecutionInput, BlueprintDataType::Wildcard);
    macroBody.GetNode(set)->properties["variableName"] = Variant(ea::string("macroCalled"));
    REQUIRE(macroBody.AddLink(entry, "then", set, "execute") != BLUEPRINT_INVALID_ID);
    REQUIRE(macroBody.AddLink(set, "then", result, "execute") != BLUEPRINT_INVALID_ID);

    BlueprintMacro macro;
    macro.name = "SetMacroCalled";
    macro.body = macroBody.ToString();
    REQUIRE(graph.AddMacro(macro));
    REQUIRE(graph.GetMacro("SetMacroCalled") != nullptr);

    BlueprintGraph restored;
    ea::string error;
    REQUIRE(restored.FromString(graph.ToString(), &error));
    REQUIRE(error.empty());
    REQUIRE(restored.GetTimelines().size() == 1);
    REQUIRE(restored.GetTimelines()[0].keyframes.size() == 2);
    REQUIRE(restored.GetMacros().size() == 1);

    BlueprintRuntime runtime;
    REQUIRE(runtime.PlayTimeline(graph, "Progress"));
    REQUIRE(runtime.IsTimelinePlaying("Progress"));
    REQUIRE(runtime.GetTimelineValue(graph, "Progress").GetFloat() == Catch::Approx(0.0f));
    REQUIRE(runtime.Tick(1.0f));
    REQUIRE(runtime.GetTimelineValue(graph, "Progress").GetFloat() == Catch::Approx(5.0f));
    REQUIRE(runtime.Tick(1.0f));
    REQUIRE_FALSE(runtime.IsTimelinePlaying("Progress"));
    REQUIRE(runtime.GetTimelineValue(graph, "Progress").GetFloat() == Catch::Approx(10.0f));

    const BlueprintId call = graph.AddNode("Macro.Call", "Call Macro");
    graph.GetNode(call)->properties["macroName"] = Variant(ea::string("SetMacroCalled"));
    REQUIRE(runtime.Execute(graph, call));
    REQUIRE(runtime.GetVariable("macroCalled").GetInt() == 17);
}

TEST_CASE("Blueprint resource round trips through an rbfx stream", "[blueprint][resource]")
{
    const auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    BlueprintGraph sourceGraph("NativeAsset");
    const BlueprintId nodeId = sourceGraph.AddNode("Math.AddFloat", "Add");
    AddPin(sourceGraph, nodeId, "a", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(1.0f));
    AddPin(sourceGraph, nodeId, "b", BlueprintPinKind::Input, BlueprintDataType::Float, Variant(2.0f));

    BlueprintResource source(context.Get());
    source.SetGraph(sourceGraph);
    VectorBuffer buffer;
    REQUIRE(source.Save(buffer));
    REQUIRE(buffer.GetSize() > 0);

    BlueprintResource restored(context.Get());
    REQUIRE(buffer.Seek(0) == 0);
    REQUIRE(restored.BeginLoad(buffer));
    REQUIRE(restored.GetGraph().GetName() == "NativeAsset");
    REQUIRE(restored.GetGraph().GetNodes().size() == 1);
    REQUIRE(restored.GetGraph().GetNodes()[0].pins.size() == 2);
}

TEST_CASE("Blueprint schema versioning migrates legacy graphs", "[blueprint][serialization][migration]")
{
    BlueprintGraph graph("SchemaTwo");
    const ea::string serialized = graph.ToString();
    REQUIRE(serialized.find("schemaVersion") != ea::string::npos);
    REQUIRE(serialized.find("\"format\": 5") != ea::string::npos);

    BlueprintGraph legacy;
    ea::string error;
    REQUIRE(legacy.FromString(R"({"format":2,"name":"Legacy","nodes":[{"id":1,"typeName":"Event.OnStart","title":"Start"}]})", &error));
    REQUIRE(error.empty());
    REQUIRE(legacy.GetName() == "Legacy");
    REQUIRE(legacy.GetNodes().size() == 1);
    REQUIRE(legacy.GetStructs().empty());
    REQUIRE(legacy.GetEnums().empty());
    REQUIRE(legacy.ToString().find("\"schemaVersion\": 5") != ea::string::npos);

    BlueprintGraph unsupported;
    REQUIRE_FALSE(unsupported.FromString(R"({"schemaVersion":999,"name":"Future"})", &error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("Blueprint builtin registry exposes complete node signatures", "[blueprint][registry]")
{
    BlueprintRuntime runtime;
    const BlueprintNodeDefinition* divide = runtime.GetRegistry().Find("Math.DivideFloat");
    REQUIRE(divide != nullptr);
    REQUIRE(divide->pins.size() == 3);
    REQUIRE(divide->pins[0].name == "a");
    REQUIRE(divide->pins[1].name == "b");
    REQUIRE(divide->pins[2].name == "result");

    const BlueprintNodeDefinition* branch = runtime.GetRegistry().Find("Flow.Branch");
    REQUIRE(branch != nullptr);
    REQUIRE(branch->pins.size() == 4);
    REQUIRE(runtime.GetRegistry().Find("Math.ClampFloat") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Event.OnMouseClick") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Delegate.Bind") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Delegate.Call") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Signal.Connect") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Signal.Emit") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Timeline.Play") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Timeline.GetValue") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Macro.Call") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Physics.ApplyForce") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Physics.ApplyImpulse") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Physics.SetVelocity") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Physics.GetVelocity") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Physics.RayCast") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Physics.SetGravity") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Animation.Play") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Animation.Stop") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Animation.SetSpeed") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Animation.IsPlaying") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Animation.GetTime") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Audio.Play") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Audio.Stop") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Audio.SetVolume") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Audio.SetPitch") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Audio.IsPlaying") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Camera.SetFOV") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Camera.GetFOV") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Camera.SetOrtho") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Camera.ScreenToWorld") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Camera.WorldToScreen") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Material.SetParameter") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Material.GetParameter") != nullptr);
    REQUIRE(runtime.GetRegistry().Find("Material.SetTexture") != nullptr);
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
